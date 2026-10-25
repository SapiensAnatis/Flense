#include <gtest/gtest.h>

import Flense.Core;
import Flense.Core.Tests.Support;
import std;

using namespace Flense::Core;
using namespace Flense::Core::Tests;

namespace
{
    using Stream = FaultyByteStream<FileByteStream>;

    const std::filesystem::path ValidArchive = TestDataPath("ErrorCases/valid.tar");

    /// <summary>
    /// Advances to the first regular file whose body is too big to have been buffered by the read that fetched its
    /// header, so reading or skipping the rest of it has to go back to the stream.
    /// </summary>
    ArchiveEntry NextLargeEntry(ArchiveReader& reader)
    {
        while (const std::optional<ArchiveEntry> entry = reader.Next({}))
        {
            if (entry->FileKind() == FileKind::File && entry->Size() > 2 * Stream::MaxReadSize)
            {
                return *entry;
            }
        }

        throw std::runtime_error("archive has no entry large enough to need further reads");
    }

    struct EntrySummary
    {
        std::string pathname;
        FileKind kind;
        std::uint64_t size;
        std::string contents;
    };

    std::vector<EntrySummary> ReadAllEntries(ArchiveReader& reader, const bool readContents)
    {
        std::vector<EntrySummary> entries;

        while (const std::optional<ArchiveEntry> entry = reader.Next({}))
        {
            EntrySummary& summary =
                entries.emplace_back(std::string{entry->Pathname()}, entry->FileKind(), entry->Size(), std::string{});

            if (readContents && summary.kind == FileKind::File)
            {
                std::vector<std::byte> buffer(summary.size);
                const std::size_t bytesRead = entry->ReadInto(buffer);
                std::ranges::transform(std::span{buffer}.first(bytesRead), std::back_inserter(summary.contents),
                                       [](const std::byte b) { return static_cast<char>(b); });
            }
        }

        return entries;
    }

    /// <summary>
    /// Checks the entries match the layout GenerateErrorCases.ps1 gives valid.tar. The blob names are digests that
    /// change whenever it is regenerated, so only their directory is checked.
    /// </summary>
    void ExpectValidArchiveLayout(const std::vector<EntrySummary>& entries)
    {
        ASSERT_EQ(entries.size(), 5);

        EXPECT_EQ(entries[0].pathname, "blobs/");
        EXPECT_EQ(entries[0].kind, FileKind::Directory);

        EXPECT_EQ(entries[1].pathname, "blobs/sha256/");
        EXPECT_EQ(entries[1].kind, FileKind::Directory);

        for (const EntrySummary& entry : std::span{entries}.subspan(2))
        {
            EXPECT_EQ(entry.kind, FileKind::File) << entry.pathname;
            EXPECT_GT(entry.size, 0) << entry.pathname;
        }

        EXPECT_TRUE(entries[2].pathname.starts_with("blobs/sha256/"));
        EXPECT_TRUE(entries[3].pathname.starts_with("blobs/sha256/"));
        EXPECT_EQ(entries[4].pathname, "manifest.json");
    }
} // namespace

TEST(ArchiveReaderTests, ReadsEveryEntry)
{
    Stream stream{ValidArchive};
    ArchiveReader reader = ArchiveReader::CreateFromStream(stream);

    const std::vector<EntrySummary> entries = ReadAllEntries(reader, true);

    ExpectValidArchiveLayout(entries);

    for (const EntrySummary& entry : entries)
    {
        EXPECT_EQ(entry.contents.size(), entry.kind == FileKind::File ? entry.size : 0) << entry.pathname;
    }

    EXPECT_TRUE(entries[4].contents.contains(R"("RepoTags":["flense-error-case:latest"])")) << entries[4].contents;
}

TEST(ArchiveReaderTests, SkipsUnreadEntries)
{
    Stream stream{ValidArchive};
    ArchiveReader reader = ArchiveReader::CreateFromStream(stream);

    ExpectValidArchiveLayout(ReadAllEntries(reader, false));
    EXPECT_GT(stream.skipCalls, 0);
}

TEST(ArchiveReaderTests, ReadFailsBeforeFirstHeader)
{
    Stream stream{ValidArchive};
    stream.failReadAt = 0;

    EXPECT_THROW(ArchiveReader::CreateFromStream(stream), InjectedReadError);
}

TEST(ArchiveReaderTests, ReadFailsMidEntry)
{
    Stream stream{ValidArchive};
    ArchiveReader reader = ArchiveReader::CreateFromStream(stream);
    const ArchiveEntry entry = NextLargeEntry(reader);

    stream.failReadAt = stream.Position();
    std::vector<std::byte> buffer(entry.Size());

    EXPECT_THROW(static_cast<void>(entry.ReadInto(buffer)), InjectedReadError);
}

TEST(ArchiveReaderTests, SkipFailsOverUnreadEntry)
{
    Stream stream{ValidArchive};
    ArchiveReader reader = ArchiveReader::CreateFromStream(stream);
    static_cast<void>(NextLargeEntry(reader));

    stream.failSkip = true;

    EXPECT_THROW(reader.Next({}), InjectedSkipError);
    EXPECT_GT(stream.skipCalls, 0);
}

TEST(ArchiveReaderTests, StopBeforeNextOverUnreadEntry)
{
    Stream stream{ValidArchive};
    ArchiveReader reader = ArchiveReader::CreateFromStream(stream);
    static_cast<void>(NextLargeEntry(reader));

    std::stop_source stopSource;
    stopSource.request_stop();

    EXPECT_THROW(reader.Next(stopSource.get_token()), OperationCancelledError);
}
