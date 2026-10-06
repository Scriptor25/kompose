#pragma once

#include <hash.hxx>

#include <toolkit/result.hxx>

namespace kompose
{
    struct Process
    {
        [[nodiscard]] toolkit::result<> operator()(std::ostream &out, std::ostream &err) const;

        void Push(Hash &builder) const;

        std::vector<std::string> Args;
    };
}
