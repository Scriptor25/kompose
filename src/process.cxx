#include <unistd.h>
#include <sys/poll.h>
#include <sys/wait.h>

#include <process.hxx>

toolkit::result<> kompose::Process::operator()(std::string &out, std::string &err) const
{
    char *argv[Args.size() + 1];
    for (size_t i = 0; i < Args.size(); ++i)
    {
        auto &arg = Args[i];
        auto *ptr = new char[arg.size() + 1];
        std::ranges::copy(arg, ptr);
        ptr[arg.size()] = 0;
        argv[i] = ptr;
    }
    argv[Args.size()] = nullptr;

    int stdout_pipe[2];
    int stderr_pipe[2];

    if (pipe(stdout_pipe) < 0)
        return toolkit::make_error("failed to create stdout pipe");

    if (pipe(stderr_pipe) < 0)
        return toolkit::make_error("failed to create stderr pipe");

    const auto pid = fork();

    if (pid < 0)
        return toolkit::make_error("failed to fork off child process");

    if (pid == 0)
    {
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        execvp(argv[0], argv);

        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    pollfd fds[]
    {
        {
            .fd = stdout_pipe[0],
            .events = POLLIN,
        },
        {
            .fd = stderr_pipe[0],
            .events = POLLIN,
        },
    };

    char buffer[4096];

    auto open_pipes = 2;
    while (open_pipes > 0)
    {
        if (poll(fds, 2, -1) < 0)
            break;

        for (auto i = 0; i < 2; ++i)
        {
            if (fds[i].fd < 0)
                continue;

            if (fds[i].revents & (POLLIN | POLLHUP))
            {
                if (const auto n = read(fds[i].fd, buffer, sizeof(buffer)); n > 0)
                {
                    if (i == 0)
                        out.append(buffer, n);
                    else
                        err.append(buffer, n);
                }
                else if (n == 0)
                {
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    --open_pipes;
                }
            }
        }
    }

    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status))
        if (auto exit_status = WEXITSTATUS(status))
            return toolkit::make_error("process exited with status {}", exit_status);

    if (WIFSIGNALED(status))
        if (auto terminate_signal = WTERMSIG(status))
            return toolkit::make_error("process terminated on signal {}", terminate_signal);

    return {};
}
