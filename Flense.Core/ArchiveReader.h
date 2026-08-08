#pragma once

#include "FileKind.h"

#include <archive.h>
#include <archive_entry.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

namespace Flense::Core
{
    /// <summary>
    /// A source libarchive can pull compressed bytes from.
    /// </summary>
    /// <remarks>
    /// All operations are synchronous/blocking by design: libarchive drives reading via plain C callbacks
    /// that block the calling thread until they return, so there is no point in the source itself being
    /// async - callers are expected to ArchiveReader::ProcessArchive on a background thread.
    /// </remarks>
    template <typename T>
    concept ByteStream = requires(T& source, std::span<std::byte> buffer, int64_t requestedSkip) {
        { source.ReadSync(buffer) } -> std::convertible_to<size_t>;
        { source.Skip(requestedSkip) } -> std::convertible_to<int64_t>;
        { source.Size() } -> std::convertible_to<uint64_t>;
        { source.Position() } -> std::convertible_to<uint64_t>;
    };

    class ArchiveReader;

    /// <summary>
    /// A single entry (file, directory, etc.) within an archive, hiding the underlying libarchive
    /// representation from callers.
    /// </summary>
    /// <remarks>
    /// Only valid until the owning ArchiveReader's Next() is called again - libarchive reuses/invalidates
    /// the underlying archive_entry at that point. Read this entry's contents first if you need them.
    /// </remarks>
    class ArchiveEntry
    {
      public:
        std::string_view Pathname() const
        {
            const char* pathname = archive_entry_pathname(m_entry);

            if (pathname)
            {
                return std::string_view(pathname);
            }
            else
            {
                return "";
            }
        }

        /// <summary>
        /// The total size of this entry's body, as recorded in the archive header.
        /// </summary>
        uint64_t Size() const
        {
            const int64_t size = archive_entry_size(m_entry);
            return size > 0 ? static_cast<uint64_t>(size) : 0;
        }

        /// <summary>
        /// Reads up to buffer.size() bytes into buffer, continuing from wherever this entry's read
        /// position currently is - so repeated calls read successive chunks (e.g. sniff a small
        /// stack-allocated prefix, then read the rest into a caller-sized buffer once it's known to
        /// be worth reading in full). Must be called before the owning ArchiveReader's Next() is
        /// called again, otherwise libarchive will already have moved past this entry's data.
        /// </summary>
        /// <returns>The number of bytes actually read, which is less than buffer.size() once this
        /// entry's body is exhausted.</returns>
        size_t ReadInto(std::span<std::byte> buffer) const
        {
            size_t totalRead = 0;
            while (totalRead < buffer.size())
            {
                const la_ssize_t bytesRead =
                    archive_read_data(m_archive, buffer.data() + totalRead, buffer.size() - totalRead);
                if (bytesRead <= 0)
                {
                    break;
                }

                totalRead += static_cast<size_t>(bytesRead);
            }

            return totalRead;
        }

        /// <summary>
        /// Gets the type of the entry (i.e. dir/file/symlink/other).
        /// </summary>
        /// <returns>The type of the entry.</returns>
        FileKind FileKind() const
        {
            switch (archive_entry_filetype(m_entry))
            {
            case AE_IFDIR:
                return FileKind::Directory;
            case AE_IFREG:
                return FileKind::File;
            case AE_IFLNK:
                return FileKind::Symlink;
            default:
                return FileKind::Other;
            }
        }

      private:
        friend class ArchiveReader;

        ArchiveEntry(archive_entry* entry, archive* archive) : m_entry(entry), m_archive(archive)
        {
        }

        archive_entry* m_entry;
        archive* m_archive;
    };

    /// <summary>
    /// A thin, pull-based wrapper over libarchive for reading .tar and .tar.gz archives: call Next()
    /// repeatedly to enumerate entries, optionally reading each one's contents before moving on.
    /// </summary>
    class ArchiveReader
    {
      public:
        /// <summary>
        /// Opens an archive from a byte source, returning an ArchiveReader that can be used to enumerate its entries.
        /// </summary>
        /// <remarks>
        /// Blocking - Next() and ArchiveEntry::ReadInto() must be called from a background thread.
        /// </remarks>
        /// <typeparam name="TSource">The type of the byte stream.</typeparam>
        /// <param name="source">The byte source to read from. Must outlive the returned ArchiveReader.</param>
        /// <param name="stopToken">The token to check for cancellation.</param>
        /// <returns>An ArchiveReader instance for enumerating the archive entries.</returns>
        template <ByteStream TStream> static ArchiveReader CreateFromStream(TStream& source, std::stop_token stopToken)
        {
            ArchivePtr archive{archive_read_new()};

            archive_read_support_format_tar(archive.get());
            archive_read_support_filter_gzip(archive.get());

            auto context = std::make_unique<Context>(Context{
                .readSync = [&source](std::span<std::byte> buffer) { return source.ReadSync(buffer); },
                .skip = [&source](int64_t request) { return source.Skip(request); },
                .stopToken = std::move(stopToken),
            });

            archive_read_set_callback_data(archive.get(), context.get());
            archive_read_set_read_callback(archive.get(), &ArchiveReadCallback);
            archive_read_set_skip_callback(archive.get(), &ArchiveSkipCallback);
            archive_read_open1(archive.get());

            return ArchiveReader{std::move(archive), std::move(context)};
        }

        /// <summary>
        /// Advances to the next entry in the archive. Automatically skips any part of the previous
        /// entry's data that wasn't consumed via ArchiveEntry::ReadInto(). Returns nullopt once
        /// the archive is exhausted (or cancellation via the stop_token passed to CreateFromStream
        /// stops the underlying read).
        /// </summary>
        /// <remarks>
        /// Invalidates any string_views or other references obtained from previous entries.
        /// </remarks>
        std::optional<ArchiveEntry> Next()
        {
            archive_entry* entry;
            if (m_context->stopToken.stop_requested() ||
                archive_read_next_header(m_archive.get(), &entry) != ARCHIVE_OK)
            {
                return std::nullopt;
            }

            // TODO: This always succeeds on paper, we are undoubtedly missing some error handling

            return ArchiveEntry{entry, m_archive.get()};
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

        struct Context
        {
            std::function<size_t(std::span<std::byte>)> readSync;
            std::function<int64_t(int64_t)> skip;
            std::stop_token stopToken;
            std::array<std::byte, ChunkSize> buffer;
        };

        static la_ssize_t ArchiveReadCallback(archive*, void* clientData, const void** buffer)
        {
            auto& context = *static_cast<Context*>(clientData);
            if (context.stopToken.stop_requested())
            {
                return 0;
            }

            const size_t bytesRead = context.readSync(std::span(context.buffer));
            *buffer = context.buffer.data();
            return static_cast<la_ssize_t>(bytesRead);
        }

        static la_int64_t ArchiveSkipCallback(archive*, void* clientData, la_int64_t request)
        {
            auto& context = *static_cast<Context*>(clientData);
            if (context.stopToken.stop_requested() || request <= 0)
            {
                return 0;
            }

            return context.skip(request);
        }

        ArchiveReader(ArchivePtr&& archive, std::unique_ptr<Context>&& context)
            : m_archive(std::move(archive)), m_context(std::move(context))
        {
        }

        ArchivePtr m_archive;
        std::unique_ptr<Context> m_context;
    };

} // namespace Flense::Core
