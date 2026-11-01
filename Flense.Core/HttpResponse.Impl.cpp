module;

#include <picohttpparser.h>

module Flense.Core.Http:HttpResponse.Impl;

import Flense.Core.Shared;

import :HttpResponse;

import std;

namespace Flense::Core
{
    namespace
    {
        constexpr std::size_t MaxHeaderSize = 8192;
        constexpr std::size_t MaxHeaderCount = 64;

        // Return codes shared by phr_parse_response and phr_decode_chunked.
        constexpr int ParseFailed = -1;
        constexpr int ParseIncomplete = -2;

        std::uint64_t ParseContentLength(const std::string_view value)
        {
            std::uint64_t contentLength = 0;
            const char* const first = std::to_address(value.begin());
            const char* const last = std::to_address(value.end());
            const auto [end, error] = std::from_chars(first, last, contentLength);

            if (error != std::errc{} || end != last)
            {
                throw HttpError(std::format("Invalid Content-Length '{}'", value));
            }

            return contentLength;
        }
    } // namespace

    HttpResponse::HttpResponse(std::unique_ptr<ByteStream> connection) : m_connection(std::move(connection))
    {
        ReadHeaders();

        if (m_statusCode == 204 || m_statusCode == 304)
        {
            m_bodyComplete = true;
        }
        else if (const auto transferEncoding = Header("Transfer-Encoding"))
        {
            if (*transferEncoding != "chunked")
            {
                throw HttpError(std::format("Unsupported Transfer-Encoding '{}'", *transferEncoding));
            }

            m_chunked = true;
            m_chunkedDecoder.consume_trailer = 1;
        }
        else if (const auto contentLength = Header("Content-Length"))
        {
            m_contentLength = ParseContentLength(*contentLength);
            m_bodyBytesRemaining = *m_contentLength;
            m_bodyComplete = m_bodyBytesRemaining == 0;
        }
    }

    std::optional<std::string_view> HttpResponse::Header(const std::string_view name) const
    {
        const auto it = m_headers.find(name);
        if (it == m_headers.end())
        {
            return std::nullopt;
        }

        return it->second;
    }

    std::size_t HttpResponse::ReadSync(std::span<std::byte> buffer)
    {
        while (!m_bodyComplete && !buffer.empty())
        {
            if (m_contentLength.has_value())
            {
                buffer = buffer.first(
                    static_cast<std::size_t>(std::min<std::uint64_t>(buffer.size(), m_bodyBytesRemaining)));
            }

            std::size_t size = ReadRaw(buffer);
            if (size == 0)
            {
                // Chunked and Content-Length bodies mark their own end, so the connection closing first means data
                // was lost. Otherwise the server marks the end of the body by closing the connection.
                if (m_chunked || m_contentLength.has_value())
                {
                    throw HttpError("Connection closed before the response body ended");
                }

                m_bodyComplete = true;
                return 0;
            }

            if (m_contentLength.has_value())
            {
                m_bodyBytesRemaining -= size;
                m_bodyComplete = m_bodyBytesRemaining == 0;
            }

            if (!m_chunked)
            {
                return size;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) - picohttpparser decodes chars in place
            const auto result = phr_decode_chunked(&m_chunkedDecoder, reinterpret_cast<char*>(buffer.data()), &size);
            if (result == ParseFailed)
            {
                throw HttpError("Malformed chunked response body");
            }

            m_bodyComplete = result != ParseIncomplete;
            if (size > 0)
            {
                return size;
            }
        }

        return 0;
    }

    void HttpResponse::ReadHeaders()
    {
        std::vector<char> buffer(MaxHeaderSize);
        std::size_t filled = 0;
        std::array<phr_header, MaxHeaderCount> headers{};
        std::size_t headerCount = 0;
        int headerLength = ParseIncomplete;

        while (headerLength == ParseIncomplete)
        {
            if (filled == buffer.size())
            {
                throw HttpError(std::format("Response headers are larger than {} bytes", MaxHeaderSize));
            }

            const std::size_t previousFilled = filled;
            const std::size_t read = m_connection->ReadSync(std::as_writable_bytes(std::span(buffer).subspan(filled)));
            filled += read;

            if (read == 0)
            {
                throw HttpError("Connection closed before the response headers ended");
            }

            int minorVersion = 0;
            const char* message = nullptr;
            std::size_t messageLength = 0;

            // phr_parse_response reads this as the capacity of headers and overwrites it with the number parsed.
            headerCount = headers.size();

            headerLength = phr_parse_response(buffer.data(), filled, &minorVersion, &m_statusCode, &message,
                                              &messageLength, headers.data(), &headerCount, previousFilled);
        }

        if (headerLength == ParseFailed)
        {
            throw HttpError("Malformed response headers");
        }

        for (const phr_header& header : std::span(headers).first(headerCount))
        {
            if (header.name == nullptr)
            {
                throw HttpError("Folded response headers are not supported");
            }

            const std::string_view name(header.name, header.name_len);
            const std::string_view value(header.value, header.value_len);

            if (const auto [it, inserted] = m_headers.try_emplace(std::string(name), value); !inserted)
            {
                it->second.append(",").append(value);
            }
        }

        const auto leftover =
            std::as_bytes(std::span(buffer).first(filled).subspan(static_cast<std::size_t>(headerLength)));
        m_leftover.assign(leftover.begin(), leftover.end());
    }

    std::size_t HttpResponse::ReadRaw(const std::span<std::byte> buffer)
    {
        if (m_leftoverPosition < m_leftover.size())
        {
            const auto available = std::span(m_leftover).subspan(m_leftoverPosition);
            const std::size_t count = std::min(buffer.size(), available.size());

            std::ranges::copy(available.first(count), buffer.begin());
            m_leftoverPosition += count;
            return count;
        }

        return m_connection->ReadSync(buffer);
    }
} // namespace Flense::Core
