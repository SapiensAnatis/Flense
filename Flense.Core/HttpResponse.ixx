module;

#include <cassert>

export module Flense.Core:HttpResponse;

import :DockerTransport;

import std;

export namespace Flense::Core
{
    struct HttpHeaderLess
    {
        using is_transparent = void;

        [[nodiscard]] bool operator()(const std::string_view left, const std::string_view right) const noexcept
        {
            return std::ranges::lexicographical_compare(left, right, std::less{}, ToLowerAscii, ToLowerAscii);
        }

      private:
        [[nodiscard]] static constexpr char ToLowerAscii(const char c) noexcept
        {
            assert(IsValidHeaderCharacter(c) && "received invalid header character");
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
        }

        [[nodiscard]] static bool IsValidHeaderCharacter(const char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
        }
    };

    class HttpResponse
    {
      public:
        explicit HttpResponse(std::unique_ptr<DockerConnection> connection);

        [[nodiscard]] int StatusCode() const;
        [[nodiscard]] std::optional<std::string_view> Header(std::string_view name) const;
        [[nodiscard]] std::optional<std::uint64_t> ContentLength() const;

        std::size_t ReadSync(std::span<std::byte> buffer);
        [[nodiscard]] std::uint64_t Position() const;

      private:
        std::unique_ptr<DockerConnection> m_connection;
        std::vector<std::byte> m_bodyBuffer;
        std::flat_map<std::string, std::string, HttpHeaderLess> m_headers;
    };
} // namespace Flense::Core