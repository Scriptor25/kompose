#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace kompose
{
    class Hash
    {
    public:
        Hash();

        void Push(std::string_view value);
        void Push(std::istream &stream);
        void Push(std::uint64_t value);

        [[nodiscard]] std::uint64_t operator()() const;
        std::string &operator()(std::string &str) const;

    private:
        void Push(std::span<const uint8_t> value);

        std::uint64_t m_Hash;
    };
}
