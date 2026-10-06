#include <hash.hxx>

#include <array>
#include <iomanip>
#include <istream>

namespace
{
    constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ull;
    constexpr std::uint64_t FNV_PRIME = 1099511628211ull;

    enum class HashInput :std::uint8_t
    {
        String = 1,
        Stream = 2,
    };
}

kompose::Hash::Hash()
    : m_Hash(FNV_OFFSET)
{
}

void kompose::Hash::Push(const std::string_view value)
{
    Push(static_cast<std::uint8_t>(HashInput::String));
    Push(value.size());

    Push({ reinterpret_cast<const std::uint8_t *>(value.data()), value.size() });
}

void kompose::Hash::Push(std::istream &stream)
{
    std::array<char, 64 * 1024> buffer{};

    Push(static_cast<std::uint8_t>(HashInput::Stream));

    while (stream)
    {
        stream.read(buffer.data(), buffer.size());

        const auto count = stream.gcount();

        Push({ buffer.data(), static_cast<std::size_t>(count) });
    }
}

void kompose::Hash::Push(const std::uint64_t value)
{
    Push({ reinterpret_cast<const std::uint8_t *>(&value), sizeof(value) });
}

std::uint64_t kompose::Hash::operator()() const
{
    return m_Hash;
}

std::string &kompose::Hash::operator()(std::string &str) const
{
    std::ostringstream stream;

    stream
            << std::hex
            << std::setfill('0')
            << std::setw(16)
            << m_Hash;

    return str = stream.str();
}

void kompose::Hash::Push(const std::span<const uint8_t> value)
{
    for (const auto byte : value)
    {
        m_Hash ^= byte;
        m_Hash ^= FNV_PRIME;
    }
}
