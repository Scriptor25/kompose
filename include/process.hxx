#pragma once

#include <toolkit/result.hxx>

namespace kompose
{
    struct Process
    {
        [[nodiscard]] toolkit::result<> operator()(std::ostream &out, std::ostream &err) const;

        std::vector<std::string> Args;
    };
}
