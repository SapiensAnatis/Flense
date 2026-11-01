#include <gtest/gtest.h>

import Flense.Core.Http;
import Flense.Core.Tests.Support;
import std;

using namespace Flense::Core;
using namespace Flense::Core::Tests;

namespace
{
    // Captured from Docker 29.6.2 in response to GET /images/{name}/get.
    constexpr std::string_view SaveResponse = "HTTP/1.1 200 OK\r\n"
                                              "Api-Version: 1.55\r\n"
                                              "Content-Type: application/x-tar\r\n"
                                              "Date: Sat, 31 Oct 2026 11:24:14 GMT\r\n"
                                              "Docker-Experimental: false\r\n"
                                              "Ostype: linux\r\n"
                                              "Server: Docker/29.6.2 (linux)\r\n"
                                              "Connection: close\r\n"
                                              "Transfer-Encoding: chunked\r\n"
                                              "\r\n";

    HttpResponse Parse(const std::string_view raw,
                       const std::size_t maxReadSize = std::numeric_limits<std::size_t>::max())
    {
        return HttpResponse(std::make_unique<MemoryByteStream>(raw, maxReadSize));
    }

    std::string ReadBody(HttpResponse& response, const std::size_t bufferSize = 4096)
    {
        std::string body;
        std::vector<char> buffer(bufferSize);

        while (const std::size_t read = response.ReadSync(std::as_writable_bytes(std::span(buffer))))
        {
            body.append(buffer.data(), read);
        }

        return body;
    }

    std::string ReadBody(const std::string_view raw,
                         const std::size_t maxReadSize = std::numeric_limits<std::size_t>::max(),
                         const std::size_t bufferSize = 4096)
    {
        HttpResponse response = Parse(raw, maxReadSize);
        return ReadBody(response, bufferSize);
    }

    std::string Chunked(const std::string_view body)
    {
        return std::format("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n{}", body);
    }
} // namespace

TEST(HttpResponseTests, ParsesStatusCode)
{
    EXPECT_EQ(Parse(SaveResponse).StatusCode(), 200);
    EXPECT_EQ(Parse("HTTP/1.1 404 Not Found\r\n\r\n").StatusCode(), 404);
}

TEST(HttpResponseTests, ParsesHeaders)
{
    const HttpResponse response = Parse(SaveResponse);

    EXPECT_EQ(response.Header("Api-Version"), "1.55");
    EXPECT_EQ(response.Header("Content-Type"), "application/x-tar");
    EXPECT_EQ(response.Header("Date"), "Sat, 31 Oct 2026 11:24:14 GMT");
    EXPECT_EQ(response.Header("Server"), "Docker/29.6.2 (linux)");
    EXPECT_EQ(response.Header("Transfer-Encoding"), "chunked");
}

TEST(HttpResponseTests, HeaderLookupIgnoresCase)
{
    const HttpResponse response = Parse(SaveResponse);

    EXPECT_EQ(response.Header("content-type"), "application/x-tar");
    EXPECT_EQ(response.Header("TRANSFER-ENCODING"), "chunked");
}

TEST(HttpResponseTests, MissingHeaderIsEmpty)
{
    const HttpResponse response = Parse(SaveResponse);

    EXPECT_EQ(response.Header("Content-Length"), std::nullopt);
    EXPECT_EQ(response.ContentLength(), std::nullopt);
}

TEST(HttpResponseTests, ParsesStatusLineWithNoHeaders)
{
    const HttpResponse response = Parse("HTTP/1.1 204 No Content\r\n\r\n");

    EXPECT_EQ(response.StatusCode(), 204);
    EXPECT_EQ(response.Header(""), std::nullopt);
}

TEST(HttpResponseTests, ParsesContentLength)
{
    EXPECT_EQ(Parse("HTTP/1.1 200 OK\r\nContent-Length: 1234\r\n\r\n").ContentLength(), 1234U);
}

TEST(HttpResponseTests, InvalidContentLengthThrows)
{
    EXPECT_THROW(Parse("HTTP/1.1 200 OK\r\nContent-Length: 12ab\r\n\r\n"), HttpError);
}

TEST(HttpResponseTests, CombinesRepeatedHeaders)
{
    const HttpResponse response = Parse("HTTP/1.1 200 OK\r\nVary: Accept\r\nvary: Origin\r\n\r\n");

    EXPECT_EQ(response.Header("Vary"), "Accept,Origin");
}

TEST(HttpResponseTests, ParsesResponseDeliveredInPieces)
{
    for (const std::size_t readSize : {1, 2, 3, 5, 255, 256, 257})
    {
        SCOPED_TRACE(std::format("read size {}", readSize));

        const HttpResponse response = Parse(SaveResponse, readSize);

        EXPECT_EQ(response.StatusCode(), 200);
        EXPECT_EQ(response.Header("Transfer-Encoding"), "chunked");
    }
}

TEST(HttpResponseTests, EmptyResponseThrows)
{
    EXPECT_THROW(Parse(""), HttpError);
}

TEST(HttpResponseTests, ResponseEndingBeforeHeadersFinishThrows)
{
    EXPECT_THROW(Parse(SaveResponse.substr(0, SaveResponse.size() - 2)), HttpError);
}

TEST(HttpResponseTests, OversizedHeadersThrow)
{
    const std::string response = std::format("HTTP/1.1 200 OK\r\nX-Padding: {}\r\n\r\n", std::string(9000, 'a'));

    EXPECT_THROW(Parse(response), HttpError);
}

TEST(HttpResponseTests, TooManyHeadersThrow)
{
    std::string response = "HTTP/1.1 200 OK\r\n";
    for (int i = 0; i < 100; ++i)
    {
        response += std::format("X-{}: v\r\n", i);
    }
    response += "\r\n";

    EXPECT_THROW(Parse(response), HttpError);
}

TEST(HttpResponseTests, HeadersExactlyAtLimitParse)
{
    // The status line, the header's name and CRLF, and the final empty line take up 32 bytes between them.
    const auto withPadding = [](const std::size_t padding) {
        return std::format("HTTP/1.1 200 OK\r\nX-Padding: {}\r\n\r\n", std::string(padding, 'a'));
    };

    ASSERT_EQ(withPadding(8160).size(), 8192U);

    EXPECT_EQ(Parse(withPadding(8160)).Header("X-Padding")->size(), 8160U);
    EXPECT_THROW(Parse(withPadding(8161)), HttpError);
}

TEST(HttpResponseTests, MalformedStatusLineThrows)
{
    for (const std::string_view statusLine : {"garbage", "HTTP/1.1 20", "HTTP/1.1 2x0 OK"})
    {
        SCOPED_TRACE(statusLine);

        EXPECT_THROW(Parse(std::format("{}\r\nServer: Docker\r\n\r\n", statusLine)), HttpError);
    }
}

TEST(HttpResponseTests, HeaderLineWithoutSeparatorThrows)
{
    EXPECT_THROW(Parse("HTTP/1.1 200 OK\r\nNotAHeader\r\n\r\n"), HttpError);
}

TEST(HttpResponseTests, UnsupportedTransferEncodingThrows)
{
    EXPECT_THROW(Parse("HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n"), HttpError);
}

TEST(HttpResponseTests, ReadsContentLengthBody)
{
    EXPECT_EQ(ReadBody("HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello"), "hello");
}

TEST(HttpResponseTests, StopsReadingAtContentLength)
{
    EXPECT_EQ(ReadBody("HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello, world"), "hello");
}

TEST(HttpResponseTests, ReadsBodyToEndWithoutContentLength)
{
    EXPECT_EQ(ReadBody("HTTP/1.1 200 OK\r\n\r\nhello, world"), "hello, world");
}

TEST(HttpResponseTests, NoContentResponseHasNoBody)
{
    EXPECT_EQ(ReadBody("HTTP/1.1 204 No Content\r\n\r\nunexpected"), "");
}

TEST(HttpResponseTests, ContentLengthBodyEndingEarlyThrows)
{
    EXPECT_THROW(ReadBody("HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nhello"), HttpError);
}

TEST(HttpResponseTests, ReadsChunkedBody)
{
    EXPECT_EQ(ReadBody(Chunked("5\r\nhello\r\n7\r\n, world\r\n0\r\n\r\n")), "hello, world");
}

TEST(HttpResponseTests, ReadsChunkedBodyContainingCrlf)
{
    EXPECT_EQ(ReadBody(Chunked("4\r\na\r\nb\r\n2\r\n\r\n\r\n0\r\n\r\n")), "a\r\nb\r\n");
}

TEST(HttpResponseTests, ReadsChunkSizesInEitherCase)
{
    const std::string data(0xAB, 'x');

    EXPECT_EQ(ReadBody(Chunked(std::format("AB\r\n{0}\r\nab\r\n{0}\r\n0\r\n\r\n", data))), data + data);
}

TEST(HttpResponseTests, IgnoresChunkExtensions)
{
    EXPECT_EQ(ReadBody(Chunked("5;name=value\r\nhello\r\n5;other\r\nworld\r\n0;last\r\n\r\n")), "helloworld");
}

TEST(HttpResponseTests, SkipsChunkedTrailers)
{
    EXPECT_EQ(ReadBody(Chunked("5\r\nhello\r\n0\r\nX-Trailer: one\r\nX-Other: two\r\n\r\n")), "hello");
}

TEST(HttpResponseTests, ReadsEmptyChunkedBody)
{
    EXPECT_EQ(ReadBody(Chunked("0\r\n\r\n")), "");
}

TEST(HttpResponseTests, ReadsChunkedBodyAtEveryReadSizeAndBufferSize)
{
    const std::string large(1000, 'L');
    const std::string raw =
        Chunked(std::format("3\r\nabc\r\n{:x}\r\n{}\r\n1\r\n\n\r\n0\r\nX-T: v\r\n\r\n", large.size(), large));
    const std::string expected = "abc" + large + "\n";

    for (std::size_t readSize = 1; readSize <= raw.size(); ++readSize)
    {
        for (const std::size_t bufferSize : {1, 2, 7, 4096})
        {
            SCOPED_TRACE(std::format("read size {}, buffer size {}", readSize, bufferSize));

            ASSERT_EQ(ReadBody(raw, readSize, bufferSize), expected);
        }
    }
}

TEST(HttpResponseTests, KeepsReturningZeroAfterChunkedBodyEnds)
{
    HttpResponse response = Parse(Chunked("5\r\nhello\r\n0\r\n\r\n"));

    EXPECT_EQ(ReadBody(response), "hello");

    std::array<std::byte, 16> buffer{};
    EXPECT_EQ(response.ReadSync(buffer), 0U);
}

TEST(HttpResponseTests, ChunkedBodyEndingEarlyThrows)
{
    for (const std::string_view body : {"", "5\r\nhel", "5\r\nhello", "5\r\nhello\r\n", "5\r\nhello\r\n0\r\n"})
    {
        SCOPED_TRACE(body);

        EXPECT_THROW(ReadBody(Chunked(body)), HttpError);
    }
}

TEST(HttpResponseTests, MalformedChunkedBodyThrows)
{
    for (const std::string_view body : {"\r\nhello\r\n0\r\n\r\n", "zz\r\nhello\r\n0\r\n\r\n",
                                        "-5\r\nhello\r\n0\r\n\r\n", "5\r\nhelloXX0\r\n\r\n",
                                        "10000000000000000\r\n"})
    {
        SCOPED_TRACE(body);

        EXPECT_THROW(ReadBody(Chunked(body)), HttpError);
    }
}
