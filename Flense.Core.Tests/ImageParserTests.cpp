#include <gtest/gtest.h>

import Flense.Core.ImageParsing;
import Flense.Core.Tests.Support;
import std;

using namespace Flense::Core;
using namespace Flense::Core::Tests;

namespace
{
    std::string_view ChangeKindName(const FilesystemChangeKind kind)
    {
        switch (kind)
        {
        case FilesystemChangeKind::Unspecified:
            return "Unspecified";
        case FilesystemChangeKind::None:
            return "None";
        case FilesystemChangeKind::Added:
            return "Added";
        case FilesystemChangeKind::Removed:
            return "Removed";
        case FilesystemChangeKind::OpaqueWhiteout:
            return "OpaqueWhiteout";
        case FilesystemChangeKind::Modified:
            return "Modified";
        }

        return "<invalid>";
    }

    Image LoadImage(const std::string_view name)
    {
        const std::filesystem::path path = TestDataPath(std::filesystem::path{"Images"} / name);

        if (!std::filesystem::exists(path))
        {
            throw std::runtime_error(
                std::format("{} not found; run TestData/GenerateTestData.ps1, then rebuild", path.string()));
        }

        return ParseImageArchive(path);
    }

    FilesystemChangeTreeNodeRef FindNode(const ImageLayer& layer, const std::string_view path)
    {
        FilesystemChangeTreeNodeRef node = layer.FilesystemChanges();

        for (const auto component : std::views::split(path, '/'))
        {
            const auto& children = node->Children();
            const auto it = children.find(std::string{std::string_view{component}});

            if (it == children.end())
            {
                return nullptr;
            }

            node = it->second;
        }

        return node;
    }

    testing::AssertionResult HasChange(const ImageLayer& layer, const std::string_view path,
                                       const FilesystemChangeKind expected)
    {
        const FilesystemChangeTreeNodeRef node = FindNode(layer, path);

        if (!node)
        {
            return testing::AssertionFailure() << path << " is missing";
        }

        const FilesystemChangeKind actual = node->Data().changeKind;
        if (actual != expected)
        {
            return testing::AssertionFailure()
                   << path << " is " << ChangeKindName(actual) << ", expected " << ChangeKindName(expected);
        }

        return testing::AssertionSuccess();
    }

    testing::AssertionResult HasSize(const ImageLayer& layer, const std::string_view path, const std::uint64_t expected)
    {
        const FilesystemChangeTreeNodeRef node = FindNode(layer, path);

        if (!node)
        {
            return testing::AssertionFailure() << path << " is missing";
        }

        if (node->Data().size != expected)
        {
            return testing::AssertionFailure()
                   << path << " has size " << node->Data().size << ", expected " << expected;
        }

        return testing::AssertionSuccess();
    }

    testing::AssertionResult IsAbsent(const ImageLayer& layer, const std::string_view path)
    {
        if (const FilesystemChangeTreeNodeRef node = FindNode(layer, path))
        {
            return testing::AssertionFailure() << path << " is present as " << ChangeKindName(node->Data().changeKind);
        }

        return testing::AssertionSuccess();
    }

    testing::AssertionResult IsLink(const ImageLayer& layer, const std::string_view path, const FileKind kind,
                                    const std::string_view target)
    {
        const FilesystemChangeTreeNodeRef node = FindNode(layer, path);

        if (!node)
        {
            return testing::AssertionFailure() << path << " is missing";
        }

        const FilesystemChangeInfo& info = node->Data();
        if (info.kind != kind)
        {
            return testing::AssertionFailure()
                   << path << " has kind " << static_cast<int>(info.kind) << ", expected " << static_cast<int>(kind);
        }

        if (info.linkTarget != target)
        {
            return testing::AssertionFailure()
                   << path << " links to '" << info.linkTarget.value_or("<none>") << "', expected '" << target << "'";
        }

        return testing::AssertionSuccess();
    }

    template <typename Error> Error CaptureError(const std::string_view errorCase)
    {
        try
        {
            static_cast<void>(ParseImageArchive(TestDataPath(std::filesystem::path{"ErrorCases"} / errorCase)));
        }
        catch (const Error& error)
        {
            return error;
        }

        throw std::runtime_error(std::format("{} parsed without the expected error", errorCase));
    }
} // namespace

TEST(ImageParserTests, ShowsAddedAndModifiedFiles)
{
    const Image image = LoadImage("BasicChangesTest.tar");
    ASSERT_EQ(image.layers.size(), 5);

    const ImageLayer& base = image.layers[0];
    EXPECT_TRUE(HasChange(base, "etc/alpine-release", FilesystemChangeKind::Added));

    const ImageLayer& created = image.layers[1];
    EXPECT_EQ(created.Command(),
              R"(RUN /bin/sh -c mkdir -p /app/config && printf 'one\n' > /app/config/settings.conf && printf )"
              R"('hello\n' > /app/hello.txt # buildkit)");
    EXPECT_TRUE(HasChange(created, "etc/alpine-release", FilesystemChangeKind::None));
    EXPECT_TRUE(HasChange(created, "app", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(created, "app/config", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(created, "app/config/settings.conf", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(created, "app/hello.txt", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasSize(created, "app/config/settings.conf", std::string_view{"one\n"}.size()));
    EXPECT_TRUE(HasSize(created, "app/hello.txt", std::string_view{"hello\n"}.size()));

    const ImageLayer& appended = image.layers[2];
    EXPECT_EQ(appended.Command(), R"(RUN /bin/sh -c printf 'two\n' >> /app/config/settings.conf # buildkit)");
    EXPECT_TRUE(HasChange(appended, "app/config/settings.conf", FilesystemChangeKind::Modified));
    EXPECT_TRUE(HasChange(appended, "app/hello.txt", FilesystemChangeKind::None));
    EXPECT_TRUE(HasSize(appended, "app/config/settings.conf", std::string_view{"one\ntwo\n"}.size()));

    const ImageLayer& sibling = image.layers[3];
    EXPECT_EQ(sibling.Command(), R"(RUN /bin/sh -c printf 'new\n' > /app/config/extra.conf # buildkit)");
    EXPECT_TRUE(HasChange(sibling, "app/config", FilesystemChangeKind::Modified));
    EXPECT_TRUE(HasChange(sibling, "app/config/extra.conf", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(sibling, "app/config/settings.conf", FilesystemChangeKind::None));
    EXPECT_TRUE(HasSize(sibling, "app/config", std::string_view{"one\ntwo\nnew\n"}.size()));

    const ImageLayer& chmodded = image.layers[4];
    EXPECT_EQ(chmodded.Command(), "RUN /bin/sh -c chmod 600 /app/hello.txt # buildkit");
    EXPECT_TRUE(HasChange(chmodded, "app/hello.txt", FilesystemChangeKind::Modified));
    EXPECT_TRUE(HasChange(chmodded, "app/config/extra.conf", FilesystemChangeKind::None));
    EXPECT_TRUE(HasSize(chmodded, "app/hello.txt", std::string_view{"hello\n"}.size()));
}

TEST(ImageParserTests, ShowsDeletionsForOneLayer)
{
    const Image image = LoadImage("DeleteTest.tar");
    ASSERT_EQ(image.layers.size(), 6);

    const ImageLayer& created = image.layers[1];
    EXPECT_EQ(created.Command(),
              "RUN /bin/sh -c mkdir -p /path/to/ && touch /path/to/file.txt && mkdir -p /path/to/directory/1 "
              "/path/to/directory/2 && touch /path/to/directory/1/1.txt /path/to/directory/2/2.txt # buildkit");
    EXPECT_TRUE(HasChange(created, "path/to/file.txt", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(created, "path/to/directory/1/1.txt", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(created, "path/to/directory/2/2.txt", FilesystemChangeKind::Added));

    const ImageLayer& deleted = image.layers[2];
    EXPECT_EQ(deleted.Command(), "RUN /bin/sh -c rm -rf /path/to/file.txt && rm -rf /path/to/directory # buildkit");
    EXPECT_TRUE(HasChange(deleted, "path/to/file.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(deleted, "path/to/directory", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(deleted, "path/to/directory/1", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(deleted, "path/to/directory/1/1.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(deleted, "path/to/directory/2/2.txt", FilesystemChangeKind::Removed));

    const ImageLayer& after = image.layers[3];
    EXPECT_EQ(after.Command(), "RUN /bin/sh -c mkdir -p /path/deleted/dirs/should/not/be/visible # buildkit");
    EXPECT_TRUE(IsAbsent(after, "path/to/file.txt"));
    EXPECT_TRUE(IsAbsent(after, "path/to/directory"));
    EXPECT_TRUE(HasChange(after, "path/to", FilesystemChangeKind::None));
    EXPECT_TRUE(HasChange(after, "path/deleted/dirs/should/not/be/visible", FilesystemChangeKind::Added));
}

TEST(ImageParserTests, ReplacingADirectoryRemovesEachChild)
{
    const Image image = LoadImage("DeleteTest.tar");
    ASSERT_EQ(image.layers.size(), 6);

    const ImageLayer& filledLayer = image.layers[4];
    EXPECT_EQ(filledLayer.Command(),
              "RUN /bin/sh -c mkdir -p /big/tree && for i in $(seq 1 1000); do mkdir -p /big/tree/dir$i && touch "
              "/big/tree/dir$i/file1.txt /big/tree/dir$i/file2.txt; done # buildkit");
    const FilesystemChangeTreeNodeRef filled = FindNode(filledLayer, "big/tree");
    ASSERT_TRUE(filled);
    EXPECT_EQ(filled->Children().size(), 1000);
    EXPECT_TRUE(HasChange(filledLayer, "big/tree/dir1000/file2.txt", FilesystemChangeKind::Added));

    const ImageLayer& replaced = image.layers[5];
    EXPECT_EQ(replaced.Command(), "RUN /bin/sh -c rm -rf /big/tree && mkdir -p /big/tree # buildkit");
    const FilesystemChangeTreeNodeRef tree = FindNode(replaced, "big/tree");
    ASSERT_TRUE(tree);
    EXPECT_NE(tree->Data().changeKind, FilesystemChangeKind::Removed);
    EXPECT_EQ(tree->Children().size(), 1000);

    for (const auto& [name, child] : tree->Children())
    {
        EXPECT_EQ(child->Data().changeKind, FilesystemChangeKind::Removed) << name;
    }

    EXPECT_TRUE(HasChange(replaced, "big/tree/dir1/file1.txt", FilesystemChangeKind::Removed));
}

TEST(ImageParserTests, ParsesZstdLayers)
{
    const Image image = LoadImage("DeleteTest.zstd.tar");

    ASSERT_EQ(image.layers.size(), 6);

    EXPECT_EQ(image.layers[1].Command(),
              "RUN /bin/sh -c mkdir -p /path/to/ && touch /path/to/file.txt && mkdir -p /path/to/directory/1 "
              "/path/to/directory/2 && touch /path/to/directory/1/1.txt /path/to/directory/2/2.txt # buildkit");
    EXPECT_TRUE(HasChange(image.layers[1], "path/to/file.txt", FilesystemChangeKind::Added));

    EXPECT_EQ(image.layers[2].Command(),
              "RUN /bin/sh -c rm -rf /path/to/file.txt && rm -rf /path/to/directory # buildkit");
    EXPECT_TRUE(HasChange(image.layers[2], "path/to/file.txt", FilesystemChangeKind::Removed));

    EXPECT_EQ(image.layers[3].Command(), "RUN /bin/sh -c mkdir -p /path/deleted/dirs/should/not/be/visible # buildkit");
    EXPECT_TRUE(IsAbsent(image.layers[3], "path/to/file.txt"));
}

TEST(ImageParserTests, NestedDeleteDoesNotRemoveAFileTwice)
{
    const Image image = LoadImage("NestedDeleteTest.tar");
    ASSERT_EQ(image.layers.size(), 4);

    EXPECT_EQ(image.layers[1].Command(), "RUN /bin/sh -c mkdir -p /nested/dir && touch /nested/dir/deleted-first.txt "
                                         "/nested/dir/deleted-with-dir.txt # buildkit");
    EXPECT_TRUE(HasChange(image.layers[1], "nested/dir/deleted-first.txt", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(image.layers[1], "nested/dir/deleted-with-dir.txt", FilesystemChangeKind::Added));

    EXPECT_EQ(image.layers[2].Command(), "RUN /bin/sh -c rm /nested/dir/deleted-first.txt # buildkit");
    EXPECT_TRUE(HasChange(image.layers[2], "nested/dir/deleted-first.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(image.layers[2], "nested/dir/deleted-with-dir.txt", FilesystemChangeKind::None));

    EXPECT_EQ(image.layers[3].Command(), "RUN /bin/sh -c rm -rf /nested/dir # buildkit");
    EXPECT_TRUE(HasChange(image.layers[3], "nested/dir", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(image.layers[3], "nested/dir/deleted-with-dir.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(IsAbsent(image.layers[3], "nested/dir/deleted-first.txt"));
}

TEST(ImageParserTests, KeepsSymlinkTargetsAsWritten)
{
    const Image image = LoadImage("LinksTest.tar");
    ASSERT_EQ(image.layers.size(), 5);

    const ImageLayer& layer = image.layers[2];
    EXPECT_EQ(layer.Command(),
              "RUN /bin/sh -c cd /links/lib/real && ln -s libfoo.so.1.2.3 libfoo.so.1 && cd /links/lib && ln -s "
              "real/libfoo.so.1 libfoo.so && cd /links/bin && ln -s ../lib/libfoo.so libfoo-from-bin && cd "
              "/links/nested/deeper && ln -s ../../data.txt data-relative && ln -s /links/data.txt "
              "/links/nested/deeper/data-absolute && ln -s ../lib /links/bin/lib-dir && ln -s does-not-exist "
              "/links/dangling # buildkit");
    EXPECT_TRUE(IsLink(layer, "links/lib/real/libfoo.so.1", FileKind::Symlink, "libfoo.so.1.2.3"));
    EXPECT_TRUE(IsLink(layer, "links/lib/libfoo.so", FileKind::Symlink, "real/libfoo.so.1"));
    EXPECT_TRUE(IsLink(layer, "links/bin/libfoo-from-bin", FileKind::Symlink, "../lib/libfoo.so"));
    EXPECT_TRUE(IsLink(layer, "links/nested/deeper/data-relative", FileKind::Symlink, "../../data.txt"));
    EXPECT_TRUE(IsLink(layer, "links/nested/deeper/data-absolute", FileKind::Symlink, "/links/data.txt"));
    EXPECT_TRUE(IsLink(layer, "links/bin/lib-dir", FileKind::Symlink, "../lib"));
    EXPECT_TRUE(IsLink(layer, "links/dangling", FileKind::Symlink, "does-not-exist"));
    EXPECT_TRUE(HasChange(layer, "links/dangling", FilesystemChangeKind::Added));
}

TEST(ImageParserTests, HardlinkTargetsAreArchiveRootPaths)
{
    const Image image = LoadImage("LinksTest.tar");
    ASSERT_EQ(image.layers.size(), 5);

    // Either name can end up as the link, depending on the order the layer differ walks the files in.
    const ImageLayer& layer = image.layers[3];
    EXPECT_EQ(layer.Command(),
              R"(RUN /bin/sh -c mkdir -p /links/hard/nested && echo "shared" > /links/hard/original.txt && ln )"
              "/links/hard/original.txt /links/hard/nested/hardlink.txt # buildkit");
    const FilesystemChangeTreeNodeRef original = FindNode(layer, "links/hard/original.txt");
    const FilesystemChangeTreeNodeRef linked = FindNode(layer, "links/hard/nested/hardlink.txt");
    ASSERT_TRUE(original);
    ASSERT_TRUE(linked);

    if (original->Data().kind == FileKind::Hardlink)
    {
        EXPECT_TRUE(IsLink(layer, "links/hard/original.txt", FileKind::Hardlink, "links/hard/nested/hardlink.txt"));
        EXPECT_EQ(linked->Data().kind, FileKind::File);
    }
    else
    {
        EXPECT_TRUE(IsLink(layer, "links/hard/nested/hardlink.txt", FileKind::Hardlink, "links/hard/original.txt"));
        EXPECT_EQ(original->Data().kind, FileKind::File);
    }

    EXPECT_EQ(image.layers[4].Command(),
              "RUN /bin/sh -c ln /links/data.txt /links/nested/deeper/data-copied-not-linked # buildkit");
    const FilesystemChangeTreeNodeRef copied = FindNode(image.layers[4], "links/nested/deeper/data-copied-not-linked");
    ASSERT_TRUE(copied);
    EXPECT_EQ(copied->Data().kind, FileKind::File);
    EXPECT_EQ(copied->Data().size, std::string_view{"data\n"}.size());
}

TEST(ImageParserTests, OpaqueWhiteoutHidesLowerLayers)
{
    const Image image = LoadImage("OpaqueWhiteoutTest.tar");
    ASSERT_EQ(image.layers.size(), 4);

    const ImageLayer& layer = image.layers[2];
    EXPECT_EQ(layer.Command(), "RUN /bin/sh -c touch /opaque/replaced/.wh..wh..opq /opaque/replaced/+before.txt "
                               "/opaque/replaced/after.txt # buildkit");
    EXPECT_TRUE(HasChange(layer, "opaque/replaced/old.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(layer, "opaque/replaced/sub", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(layer, "opaque/replaced/sub/old-nested.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(layer, "opaque/replaced/+before.txt", FilesystemChangeKind::Added));
    EXPECT_TRUE(HasChange(layer, "opaque/replaced/after.txt", FilesystemChangeKind::Added));
    EXPECT_TRUE(IsAbsent(layer, "opaque/replaced/.wh..wh..opq"));
    EXPECT_TRUE(HasChange(layer, "opaque/kept/kept.txt", FilesystemChangeKind::None));
    EXPECT_TRUE(HasChange(layer, "opaque/merged/old.txt", FilesystemChangeKind::None));

    const ImageLayer& next = image.layers[3];
    EXPECT_EQ(next.Command(),
              R"(RUN /bin/sh -c echo "new" > /opaque/merged/recreated.txt && touch /opaque/merged/.wh..wh..opq )"
              "/opaque/merged/sub/new-nested.txt # buildkit");
    EXPECT_TRUE(IsAbsent(next, "opaque/replaced/old.txt"));
    EXPECT_TRUE(IsAbsent(next, "opaque/replaced/sub"));
    EXPECT_TRUE(HasChange(next, "opaque/replaced/+before.txt", FilesystemChangeKind::None));
}

TEST(ImageParserTests, OpaqueWhiteoutHidesEntriesTheLayerAlsoContains)
{
    const Image image = LoadImage("OpaqueWhiteoutTest.tar");
    ASSERT_EQ(image.layers.size(), 4);

    const ImageLayer& layer = image.layers[3];
    EXPECT_EQ(layer.Command(),
              R"(RUN /bin/sh -c echo "new" > /opaque/merged/recreated.txt && touch /opaque/merged/.wh..wh..opq )"
              "/opaque/merged/sub/new-nested.txt # buildkit");
    EXPECT_TRUE(HasChange(layer, "opaque/merged/old.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(layer, "opaque/merged/recreated.txt", FilesystemChangeKind::Modified));
    EXPECT_TRUE(HasChange(layer, "opaque/merged/sub/old-nested.txt", FilesystemChangeKind::Removed));
    EXPECT_TRUE(HasChange(layer, "opaque/merged/sub/new-nested.txt", FilesystemChangeKind::Added));
}

TEST(ImageParserTests, ConfigWithoutHistoryLabelsLayersUnknown)
{
    const Image image = LoadImage("NoHistoryTest.tar");
    EXPECT_EQ(image.repoTag, "flense.test/no-history:latest");
    ASSERT_EQ(image.layers.size(), 2);

    EXPECT_EQ(image.layers[0].Command(), "Unknown command");
    EXPECT_EQ(image.layers[1].Command(), "Unknown command");

    const FilesystemChangeTreeNodeRef config = FindNode(image.layers[0], "etc/app.conf");
    ASSERT_TRUE(config);
    EXPECT_EQ(config->Data().size, std::string_view{"greeting = hello\n"}.size());

    EXPECT_TRUE(HasChange(image.layers[1], "etc/app.conf", FilesystemChangeKind::None));
    EXPECT_TRUE(HasChange(image.layers[1], "app/hello.txt", FilesystemChangeKind::Added));
}

TEST(ImageParserTests, MoreHistoryThanLayersLabelsLayersUnknown)
{
    const Image image = LoadImage("ExtraHistoryTest.tar");
    EXPECT_EQ(image.repoTag, "flense.test/extra-history:latest");
    ASSERT_EQ(image.layers.size(), 1);
    EXPECT_EQ(image.layers[0].Command(), "Unknown command");
    EXPECT_TRUE(HasChange(image.layers[0], "app/hello.txt", FilesystemChangeKind::Added));
}

TEST(ImageParserTests, ParsesKoTarball)
{
    const Image image = LoadImage("ko.tar");
    EXPECT_EQ(image.repoTag, "flense.test/ko:latest");
    ASSERT_FALSE(image.layers.empty());

    const FilesystemChangeTreeNodeRef binary = FindNode(image.layers.back(), "ko-app/kotest");
    ASSERT_TRUE(binary);
    EXPECT_EQ(binary->Data().kind, FileKind::File);
    EXPECT_GT(binary->Data().size, 0);
}

TEST(ImageParserTests, ParsesValidErrorCaseBaseline)
{
    const Image image = ParseImageArchive(TestDataPath("ErrorCases/valid.tar"));

    EXPECT_EQ(image.repoTag, "flense-error-case:latest");
    ASSERT_EQ(image.layers.size(), 1);
    EXPECT_EQ(image.layers[0].Command(), "COPY file.bin /");

    const FilesystemChangeTreeNodeRef file = FindNode(image.layers[0], "file.bin");
    ASSERT_TRUE(file);
    EXPECT_EQ(file->Data().size, 1024);
}

TEST(ImageParserTests, RejectsFileThatIsNotAnArchive)
{
    EXPECT_EQ(CaptureError<ArchiveError>("not-an-archive.tar").Kind(), ArchiveErrorKind::Unreadable);
}

TEST(ImageParserTests, RejectsTruncatedArchive)
{
    EXPECT_EQ(CaptureError<ArchiveError>("truncated.tar").Kind(), ArchiveErrorKind::Corrupt);
}

TEST(ImageParserTests, RejectsLayerThatIsNotAnArchive)
{
    const ArchiveError error = CaptureError<ArchiveError>("corrupt-layer.tar");

    EXPECT_EQ(error.Kind(), ArchiveErrorKind::Unreadable);
    ASSERT_TRUE(error.EntryPath());
    EXPECT_TRUE(error.EntryPath()->starts_with("blobs/sha256/")) << *error.EntryPath();
}

TEST(ImageParserTests, RejectsArchiveWithoutManifest)
{
    EXPECT_EQ(CaptureError<ImageFormatError>("no-manifest.tar").Kind(), ImageFormatErrorKind::NoManifest);
}

TEST(ImageParserTests, RejectsMalformedManifest)
{
    const ImageFormatError error = CaptureError<ImageFormatError>("malformed-manifest.tar");

    EXPECT_EQ(error.Kind(), ImageFormatErrorKind::MalformedMetadata);
    EXPECT_EQ(error.EntryPath(), "manifest.json");
}

TEST(ImageParserTests, RejectsManifestWithMissingConfig)
{
    const ImageFormatError error = CaptureError<ImageFormatError>("missing-config.tar");

    EXPECT_EQ(error.Kind(), ImageFormatErrorKind::MissingConfiguration);
    EXPECT_EQ(error.EntryPath(), "blobs/sha256/" + std::string(64, '0'));
}

TEST(ImageParserTests, RejectsMalformedConfig)
{
    EXPECT_EQ(CaptureError<ImageFormatError>("malformed-config.tar").Kind(), ImageFormatErrorKind::MalformedMetadata);
}

TEST(ImageParserTests, RejectsManifestWithMissingLayer)
{
    const ImageFormatError error = CaptureError<ImageFormatError>("missing-layer-blob.tar");

    EXPECT_EQ(error.Kind(), ImageFormatErrorKind::MissingLayers);
    EXPECT_EQ(error.EntryPath(), "blobs/sha256/" + std::string(64, '0'));
}
