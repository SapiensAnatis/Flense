#pragma once

#include <archive.h>
#include <archive_entry.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <stop_token>
#include <string>
#include <vector>

namespace Flense::Core
{
    /// <summary>
    /// A source libarchive can pull compressed bytes from.
    /// </summary>
    /// <remarks>
    /// All operations are  synchronous/blocking by design: libarchive drives reading via plain C callbacks
    /// that block the calling thread until they return, so there is no point in the source itself being
    /// async - callers are expected to ArchiveReader::ProcessArchive on a background thread.
    /// </remarks>
    template <typename T>
    concept LibArchiveSource = requires(T& source, std::span<std::byte> buffer, int64_t requestedSkip) {
        { source.ReadSync(buffer) } -> std::convertible_to<size_t>;
        { source.Skip(requestedSkip) } -> std::convertible_to<int64_t>;
        { source.Size() } -> std::convertible_to<uint64_t>;
        { source.Position() } -> std::convertible_to<uint64_t>;
    };

    /// <summary>
    /// A class for interacting with .tar and .tar.gz archives.
    /// </summary>
    struct ArchiveReader
    {
        /// <summary>
        /// Loads an archive from a byte source, returning an ArchiveReader that can be used to enumerate the entries.
        /// </summary>
        /// <remarks>
        /// Blocking - must be called from a background thread.
        /// </remarks>
        /// <typeparam name="TSource">The type of the byte source.</typeparam>
        /// <param name="source">The byte source to process.</param>
        /// <param name="onProgress">The function to call with progress updates.</param>
        /// <param name="stopToken">The token to check for cancellation.</param>
        /// <returns>An ArchiveReader instance for enumerating the archive entries.</returns>
        template <LibArchiveSource TSource>
        static ArchiveReader CreateFromStream(TSource& source, std::function<void(double)> onProgress,
                                              std::stop_token stopToken)
        {
            ArchivePtr archive{archive_read_new()};

            archive_read_support_format_tar(archive.get());
            archive_read_support_filter_gzip(archive.get());

            ReadContext<TSource> context{&source, &stopToken};
            archive_read_set_callback_data(archive.get(), &context);
            archive_read_set_read_callback(archive.get(), &ArchiveReadCallback<TSource>);
            archive_read_set_skip_callback(archive.get(), &ArchiveSkipCallback<TSource>);
            archive_read_open1(archive.get());

            std::vector<ArchiveEntryPtr> entries;

            uint64_t const totalSize = source.Size();
            double lastReportedPercent = -1.0;

            archive_entry* entry;
            while (!stopToken.stop_requested() && archive_read_next_header(archive.get(), &entry) == ARCHIVE_OK)
            {
                entries.emplace_back(ArchiveEntryPtr{entry});

                // Skip the entry's body rather than reading it via archive_read_data_block -
                // nothing currently needs the file contents, and skipping lets the source
                // seek past the data instead of copying it through the read callback.
                archive_read_data_skip(archive.get());

                assert(totalSize > 0);
                double const percent = static_cast<double>(source.Position()) / static_cast<double>(totalSize) * 100.0;

                // Only report on meaningful (>=5%) changes - onProgress typically
                // marshals to a UI thread, and posting on every entry (there can
                // be thousands in a large archive) can flood it badly enough to
                // look and behave like a hang.
                if (percent - lastReportedPercent >= 5.0)
                {
                    lastReportedPercent = percent;
                    onProgress(percent);
                }
            }

            // TODO: This always succeeds on paper, we are undoubtedly missing some error handling

            return ArchiveReader{std::move(archive), std::move(entries)};
        }

      private:
        struct ArchiveDeleter
        {
            void operator()(archive* a) const
            {
                archive_read_free(a);
            }
        };

        using ArchivePtr = std::unique_ptr<archive, ArchiveDeleter>;

        struct ArchiveEntryDeleter
        {
            void operator()(archive_entry* e) const
            {
                archive_entry_free(e);
            }
        };

        using ArchiveEntryPtr = std::unique_ptr<archive_entry, ArchiveEntryDeleter>;

        static constexpr size_t ChunkSize = 64 * 1024;

        template <LibArchiveSource TSource> struct ReadContext
        {
            TSource* source;
            std::stop_token* stopToken;
            std::array<std::byte, ChunkSize> buffer;
        };

        template <LibArchiveSource TSource>
        static la_ssize_t ArchiveReadCallback(archive*, void* clientData, const void** buffer)
        {
            auto& context = *static_cast<ReadContext<TSource>*>(clientData);
            if (context.stopToken->stop_requested())
            {
                return 0;
            }

            size_t const bytesRead = context.source->ReadSync(std::span(context.buffer));
            *buffer = context.buffer.data();
            return static_cast<la_ssize_t>(bytesRead);
        }

        template <LibArchiveSource TSource>
        static la_int64_t ArchiveSkipCallback(archive*, void* clientData, la_int64_t request)
        {
            auto& context = *static_cast<ReadContext<TSource>*>(clientData);
            if (context.stopToken->stop_requested() || request <= 0)
            {
                return 0;
            }

            return context.source->Skip(request);
        }

        ArchiveReader(ArchivePtr&& archive, std::vector<ArchiveEntryPtr>&& entries)
            : m_archive(std::move(archive)), m_entries(std::move(entries))
        {
        }

        ArchivePtr m_archive;
        std::vector<ArchiveEntryPtr> m_entries;
    };

} // namespace Flense::Core
