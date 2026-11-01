module;

#include <picohttpparser.h>

export module Flense.Core.Http:HttpResponse;

import Flense.Core.Shared;

import std;

namespace Flense::Core
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
            return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
        }
    };
} // namespace Flense::Core

export namespace Flense::Core
{
    /// <summary>
    /// Thrown when a server sends a response that is malformed, too large, or ends early.
    /// </summary>
    class HttpError : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    /// <summary>
    /// An HTTP/1.1 response read from a connection. The status line and headers are read up front, and the body is
    /// then read as a stream, with any chunked transfer encoding removed.
    /// </summary>
    class HttpResponse : public ByteStream
    {
      public:
        /// <summary>
        /// Reads the status line and headers from the connection.
        /// </summary>
        /// <exception cref="HttpError">The headers are malformed, too large, or the connection ends before them.
        /// </exception>
        explicit HttpResponse(std::unique_ptr<ByteStream> connection);

        [[nodiscard]] int StatusCode() const
        {
            return m_statusCode;
        }

        /// <summary>
        /// Looks up a header by name, ignoring case. Repeated headers are combined into one comma-separated value.
        /// </summary>
        [[nodiscard]] std::optional<std::string_view> Header(std::string_view name) const;

        [[nodiscard]] std::optional<std::uint64_t> ContentLength() const
        {
            return m_contentLength;
        }

        /// <summary>
        /// Reads up to buffer.size() bytes of the body.
        /// </summary>
        /// <exception cref="HttpError">The body is malformed or the connection ends before it does.</exception>
        std::size_t ReadSync(std::span<std::byte> buffer) override;

      private:
        void ReadHeaders();
        std::size_t ReadRaw(std::span<std::byte> buffer);

        std::unique_ptr<ByteStream> m_connection;
        int m_statusCode = 0;
        std::flat_map<std::string, std::string, HttpHeaderLess> m_headers;
        std::optional<std::uint64_t> m_contentLength;
        std::uint64_t m_bodyBytesRemaining = 0;
        bool m_chunked = false;
        bool m_bodyComplete = false;
        phr_chunked_decoder m_chunkedDecoder{};

        // Body bytes that arrived in the same reads as the headers.
        std::vector<std::byte> m_leftover;
        std::size_t m_leftoverPosition = 0;
    };
} // namespace Flense::Core
