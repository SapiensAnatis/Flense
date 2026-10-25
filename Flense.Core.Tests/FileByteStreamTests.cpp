#include <gtest/gtest.h>

import Flense.Core;
import Flense.Core.Tests.Support;
import std;

using namespace Flense::Core;
using namespace Flense::Core::Tests;

namespace
{
    const std::filesystem::path ValidArchive = TestDataPath("ErrorCases/valid.tar");

    std::vector<std::byte> ReadToEnd(FileByteStream& stream)
    {
        std::vector<std::byte> contents;
        std::array<std::byte, 4096> chunk{};

        while (const std::size_t bytesRead = stream.ReadSync(chunk))
        {
            contents.insert(contents.end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(bytesRead));
        }

        return contents;
    }

    /// <summary>
    /// A temporary file spanning several of FileByteStream's chunks, with an unaligned size so the final chunk is
    /// partial.
    /// </summary>
    class LargeTempFile
    {
      public:
        LargeTempFile()
            : m_path(std::filesystem::temp_directory_path() /
                     std::format("flense-filebytestream-{}.bin", std::random_device{}()))
        {
            constexpr std::size_t Size = (std::size_t{7} * 512 * 1024) + 123;

            m_contents.resize(Size);
            for (std::size_t i = 0; i < Size; ++i)
            {
                m_contents[i] = static_cast<std::byte>((i * 31) + (i / 4096));
            }

            std::ofstream file{m_path, std::ios::binary};
            file.write(
                reinterpret_cast<const char*>(m_contents.data()), // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
                static_cast<std::streamsize>(m_contents.size()));
        }

        ~LargeTempFile()
        {
            std::error_code ignored;
            std::filesystem::remove(m_path, ignored);
        }

        LargeTempFile(const LargeTempFile&) = delete;
        LargeTempFile& operator=(const LargeTempFile&) = delete;
        LargeTempFile(LargeTempFile&&) = delete;
        LargeTempFile& operator=(LargeTempFile&&) = delete;

        [[nodiscard]] const std::filesystem::path& Path() const
        {
            return m_path;
        }

        [[nodiscard]] std::span<const std::byte> Contents() const
        {
            return m_contents;
        }

      private:
        std::filesystem::path m_path;
        std::vector<std::byte> m_contents;
    };
} // namespace

TEST(FileByteStreamTests, ReportsSizeAndStartsAtBeginning)
{
    const FileByteStream stream{ValidArchive};

    EXPECT_EQ(stream.Size(), std::filesystem::file_size(ValidArchive));
    EXPECT_EQ(stream.Position(), 0);
}

TEST(FileByteStreamTests, ReadsWholeFile)
{
    FileByteStream stream{ValidArchive};

    const std::vector<std::byte> contents = ReadToEnd(stream);

    EXPECT_EQ(contents.size(), stream.Size());
    EXPECT_EQ(stream.Position(), stream.Size());

    std::array<std::byte, 16> buffer{};
    EXPECT_EQ(stream.ReadSync(buffer), 0);
    EXPECT_EQ(stream.Skip(1), 0);
}

TEST(FileByteStreamTests, ReadsFromSkippedPosition)
{
    FileByteStream whole{ValidArchive};
    const std::vector<std::byte> contents = ReadToEnd(whole);
    ASSERT_GT(contents.size(), 1024);

    FileByteStream stream{ValidArchive};
    ASSERT_EQ(stream.Skip(1000), 1000);

    std::array<std::byte, 24> buffer{};
    ASSERT_EQ(stream.ReadSync(buffer), buffer.size());

    EXPECT_TRUE(std::ranges::equal(buffer, std::span{contents}.subspan(1000, buffer.size())));
    EXPECT_EQ(stream.Position(), 1000 + buffer.size());
}

TEST(FileByteStreamTests, SkipStopsAtEndOfFile)
{
    FileByteStream stream{ValidArchive};
    const auto size = static_cast<std::int64_t>(stream.Size());

    EXPECT_EQ(stream.Skip(size + 100), size);
    EXPECT_EQ(stream.Position(), stream.Size());
    EXPECT_EQ(stream.Skip(1), 0);

    std::array<std::byte, 16> buffer{};
    EXPECT_EQ(stream.ReadSync(buffer), 0);
}

TEST(FileByteStreamTests, ReadsAcrossChunks)
{
    const LargeTempFile file;

    FileByteStream stream{file.Path()};

    EXPECT_EQ(stream.Size(), file.Contents().size());
    EXPECT_TRUE(std::ranges::equal(ReadToEnd(stream), file.Contents()));
}

TEST(FileByteStreamTests, SkipsAcrossChunks)
{
    const LargeTempFile file;
    const std::span<const std::byte> expected = file.Contents();

    FileByteStream stream{file.Path()};
    std::array<std::byte, 100> buffer{};

    // Buffers the first chunk.
    ASSERT_EQ(stream.ReadSync(buffer), buffer.size());
    EXPECT_TRUE(std::ranges::equal(buffer, expected.first(buffer.size())));

    // Skips within the buffered chunk, then reads across its end.
    constexpr std::size_t NearChunkEnd = (1024 * 1024) - 10;
    ASSERT_EQ(stream.Skip(NearChunkEnd - buffer.size()), NearChunkEnd - buffer.size());
    ASSERT_EQ(stream.ReadSync(buffer), buffer.size());
    EXPECT_TRUE(std::ranges::equal(buffer, expected.subspan(NearChunkEnd, buffer.size())));

    // Skips well past the buffered chunk.
    ASSERT_EQ(stream.Skip((2 * 1024 * 1024) + 7), (2 * 1024 * 1024) + 7);
    const auto position = static_cast<std::size_t>(stream.Position());
    ASSERT_EQ(stream.ReadSync(buffer), buffer.size());
    EXPECT_TRUE(std::ranges::equal(buffer, expected.subspan(position, buffer.size())));

    const std::vector<std::byte> rest = ReadToEnd(stream);
    EXPECT_TRUE(std::ranges::equal(rest, expected.subspan(position + buffer.size())));
    EXPECT_EQ(stream.Position(), expected.size());
}

TEST(FileByteStreamTests, MissingFileThrows)
{
    EXPECT_THROW(FileByteStream{TestDataPath("ErrorCases/does-not-exist.tar")}, std::filesystem::filesystem_error);
}
