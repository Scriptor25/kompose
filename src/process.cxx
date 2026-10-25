#include <process.hxx>

#include <unistd.h>
#include <sys/poll.h>
#include <sys/wait.h>

#include <iostream>
#include <ostream>

toolkit::result<> kompose::Process::operator()(
    std::ostream &out,
    std::ostream &err) const
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

        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0)
        {
            std::cerr << "failed to attach stdout" << std::endl;
            _exit(127);
        }
        if (dup2(stderr_pipe[1], STDERR_FILENO) < 0)
        {
            std::cerr << "failed to attach stderr" << std::endl;
            _exit(127);
        }

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);

        if (!WorkingDirectory.empty())
        {
            const auto wd = WorkingDirectory.string();
            if (chdir(wd.c_str()))
            {
                std::cerr << "failed to change to working directory" << std::endl;
                _exit(127);
            }
        }

        execvp(argv[0], argv);
        std::cerr << "failed to execute with args" << std::endl;
        _exit(127);
    }

    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    std::array fds
    {
        pollfd
        {
            .fd = stdout_pipe[0],
            .events = POLLIN,
        },
        pollfd
        {
            .fd = stderr_pipe[0],
            .events = POLLIN,
        },
    };

    std::array<char, 4096> buffer{};

    auto open_pipes = fds.size();
    while (open_pipes > 0)
    {
        if (poll(fds.data(), fds.size(), -1) < 0)
        {
            if (errno == EINTR)
                continue;
            break;
        }

        for (auto &p : fds)
        {
            if (p.fd < 0)
                continue;

            if (p.revents & (POLLIN | POLLHUP))
            {
                if (const auto n = read(p.fd, buffer.data(), buffer.size()); n > 0)
                {
                    if (p.fd == stdout_pipe[0])
                        out.write(buffer.data(), n);
                    else
                        err.write(buffer.data(), n);
                }
                else if (n == 0 || (errno != EINTR && errno != EAGAIN))
                {
                    close(p.fd);
                    p.fd = -1;
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

void kompose::Process::Push(Hash &builder) const
{
    for (const auto &arg : Args)
        builder.Push(arg);
}
