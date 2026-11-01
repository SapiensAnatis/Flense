#include <gtest/gtest.h>

import Flense.Core.Shared;
import Flense.Core.Tests.Support;
import std;

using namespace Flense::Core;
using namespace Flense::Core::Tests;

static_assert(std::ranges::input_range<ByteStreamRange>);

namespace
{
    std::string ReadAll(ByteStream& stream)
    {
        ByteStreamRange range(stream);

        std::string result;
        for (const char c : range)
        {
            result += c;
        }

        return result;
    }

    std::string Pattern(const std::size_t size)
    {
        std::string pattern(size, '\0');
        for (std::size_t i = 0; i < size; ++i)
        {
            pattern[i] = static_cast<char>('a' + (i % 26));
        }

        return pattern;
    }
} // namespace

TEST(ByteStreamRangeTests, EmptyStreamIsEmptyRange)
{
    MemoryByteStream stream("");
    ByteStreamRange range(stream);

    EXPECT_TRUE(range.begin() == range.end());
}

TEST(ByteStreamRangeTests, ReadsWholeStream)
{
    MemoryByteStream stream("hello, world");

    EXPECT_EQ(ReadAll(stream), "hello, world");
}

TEST(ByteStreamRangeTests, ReadsStreamLargerThanBuffer)
{
    const std::string contents = Pattern(200'000);
    MemoryByteStream stream(contents);

    EXPECT_EQ(ReadAll(stream), contents);
}

TEST(ByteStreamRangeTests, ReadsStreamDeliveredInPieces)
{
    const std::string contents = Pattern(1000);

    for (const std::size_t readSize : {1, 2, 7, 999})
    {
        SCOPED_TRACE(std::format("read size {}", readSize));

        MemoryByteStream stream(contents, readSize);
        EXPECT_EQ(ReadAll(stream), contents);
    }
}
