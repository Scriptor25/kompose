#pragma once

#include <toolkit/result.hxx>

namespace kompose
{
    struct Process
    {
        [[nodiscard]] toolkit::result<> operator()(std::string &out, std::string &err) const;

        std::vector<std::string> Args;
    };
}
