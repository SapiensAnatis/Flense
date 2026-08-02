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
    // A source libarchive can pull compressed bytes from. All operations are
    // synchronous/blocking by design: libarchive drives reading via plain C
    // callbacks that block the calling thread until they return, so there is
    // no point in the source itself being async - callers are expected to run
    // ArchiveReader::ProcessArchive on a background thread.
    template <typename T>
    concept LibArchiveSource = requires(T& source, std::span<std::byte> buffer, int64_t requestedSkip) {
        { source.ReadSync(buffer) } -> std::convertible_to<size_t>;
        { source.Skip(requestedSkip) } -> std::convertible_to<int64_t>;
        { source.Size() } -> std::convertible_to<uint64_t>;
        { source.Position() } -> std::convertible_to<uint64_t>;
    };

    struct ArchiveReader
    {
        ArchiveReader();

        /**
         * Processes an archive from a byte source, reporting progress and checking for cancellation.
         * Blocking - must be called from a background thread.
         * @tparam TSource The type of the byte source.
         * @param source The byte source to process.
         * @param onProgress The function to call with progress updates.
         * @param stopToken The token to check for cancellation.
         */
        template <LibArchiveSource TSource>
        std::vector<std::string> ProcessArchive(TSource& source, std::function<void(double)> onProgress,
                                                std::stop_token stopToken)
        {
            ReadContext<TSource> context{&source, &stopToken};
            archive_read_set_callback_data(m_archive.get(), &context);
            archive_read_set_read_callback(m_archive.get(), &ArchiveReadCallback<TSource>);
            archive_read_set_skip_callback(m_archive.get(), &ArchiveSkipCallback<TSource>);
            archive_read_open1(m_archive.get());

            std::vector<std::string> fileNames;

            uint64_t const totalSize = source.Size();
            double lastReportedPercent = -1.0;

            archive_entry* entry;
            while (!stopToken.stop_requested() && archive_read_next_header(m_archive.get(), &entry) == ARCHIVE_OK)
            {
                fileNames.emplace_back(archive_entry_pathname(entry));

                // Skip the entry's body rather than reading it via archive_read_data_block -
                // nothing currently needs the file contents, and skipping lets the source
                // seek past the data instead of copying it through the read callback.
                archive_read_data_skip(m_archive.get());

                assert(totalSize > 0);
                double const percent =
                    static_cast<double>(source.Position()) / static_cast<double>(totalSize) * 100.0;

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

            return fileNames;
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

        static constexpr size_t ChunkSize = 64 * 1024;

        template <typename TSource> struct ReadContext
        {
            TSource* source;
            std::stop_token* stopToken;
            std::array<std::byte, ChunkSize> buffer;
        };

        template <typename TSource>
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

        template <typename TSource>
        static la_int64_t ArchiveSkipCallback(archive*, void* clientData, la_int64_t request)
        {
            auto& context = *static_cast<ReadContext<TSource>*>(clientData);
            if (context.stopToken->stop_requested() || request <= 0)
            {
                return 0;
            }

            return context.source->Skip(request);
        }

        void ProcessTarBytes(std::span<const std::byte> bytes);

        std::vector<std::string> m_filenames;
        ArchivePtr m_archive;
    };

} // namespace Flense::Core
