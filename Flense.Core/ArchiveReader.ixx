module;
#include <archive.h>
#include <archive_entry.h>
export module Flense.Core:ArchiveReader;

import :Filesystem;
import std;

export namespace Flense::Core
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
    concept ByteStream = requires(T& source, std::span<std::byte> buffer, std::int64_t requestedSkip) {
        { source.ReadSync(buffer) } -> std::convertible_to<std::size_t>;
        { source.Skip(requestedSkip) } -> std::convertible_to<std::int64_t>;
        { source.Size() } -> std::convertible_to<std::uint64_t>;
        { source.Position() } -> std::convertible_to<std::uint64_t>;
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
        [[nodiscard]] std::string_view Pathname() const
        {
            const char* pathname = archive_entry_pathname(m_entry);

            if (pathname != nullptr)
            {
                return {pathname};
            }

            return "";
        }

        /// <summary>
        /// The total size of this entry's body, as recorded in the archive header.
        /// </summary>
        [[nodiscard]] std::uint64_t Size() const
        {
            const std::int64_t size = archive_entry_size(m_entry);
            return size > 0 ? static_cast<std::uint64_t>(size) : 0;
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
        [[nodiscard]] std::size_t ReadInto(std::span<std::byte> buffer) const
        {
            std::size_t totalRead = 0;
            while (totalRead < buffer.size())
            {
                const std::span<std::byte> remaining = buffer.subspan(totalRead);
                const la_ssize_t bytesRead = archive_read_data(m_archive, remaining.data(), remaining.size());
                if (bytesRead <= 0)
                {
                    break;
                }

                totalRead += static_cast<std::size_t>(bytesRead);
            }

            return totalRead;
        }

        /// <summary>
        /// Gets the type of the entry (i.e. dir/file/symlink/other).
        /// </summary>
        /// <returns>The type of the entry.</returns>
        [[nodiscard]] FileKind FileKind() const
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

        /// <summary>
        /// Gets the parent archive reader.
        /// </summary>
        /// <remarks>
        /// Used for NestedArchiveByteStream where we want to call ArchiveReader::Skip while reading a sub-tar.
        /// </remarks>
        [[nodiscard]] ArchiveReader* ParentReader() const
        {
            return m_parentReader;
        }

      private:
        friend class ArchiveReader;

        ArchiveEntry(archive_entry* entry, archive* archive, ArchiveReader* parentReader)
            : m_entry(entry), m_archive(archive), m_parentReader(parentReader)
        {
        }

        archive_entry* m_entry;
        archive* m_archive;
        ArchiveReader* m_parentReader;
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
        /// <returns>An ArchiveReader instance for enumerating the archive entries.</returns>
        template <ByteStream TStream> static ArchiveReader CreateFromStream(TStream& source)
        {
            ArchivePtr archive{archive_read_new()};

            archive_read_support_format_tar(archive.get());
            archive_read_support_filter_gzip(archive.get());

            auto context = std::make_unique<Context>();
            context->readSync = [&source](std::span<std::byte> buffer) { return source.ReadSync(buffer); };
            context->skip = [&source](std::int64_t request) { return source.Skip(request); };

            archive_read_set_callback_data(archive.get(), context.get());
            archive_read_set_read_callback(archive.get(), &ArchiveReadCallback);
            archive_read_set_skip_callback(archive.get(), &ArchiveSkipCallback);
            archive_read_open1(archive.get());

            return ArchiveReader{std::move(archive), std::move(context)};
        }

        /// <summary>
        /// Advances to the next entry in the archive. Automatically skips any part of the previous
        /// entry's data that wasn't consumed via ArchiveEntry::ReadInto(). Returns nullopt once
        /// the archive is exhausted, or once cancellation is requested.
        /// </summary>
        /// <param name="stopToken">Checked as the underlying stream is read, so this call can be interrupted
        /// part-way through skipping a large entry rather than only between entries. It applies to this call
        /// alone - the reader holds no cancellation state of its own between operations, and a nullopt return
        /// means "exhausted or cancelled", so callers that need to tell the two apart test the token
        /// themselves.</param>
        /// <remarks>
        /// Invalidates any string_views or other references obtained from previous entries.
        /// </remarks>
        std::optional<ArchiveEntry> Next(std::stop_token stopToken)
        {
            const StopTokenScope scope{*m_context, std::move(stopToken)};

            archive_entry* entry = nullptr;
            if (m_context->stopToken.stop_requested() ||
                archive_read_next_header(m_archive.get(), &entry) != ARCHIVE_OK)
            {
                return std::nullopt;
            }

            // TODO: This always succeeds on paper, we are undoubtedly missing some error handling

            return ArchiveEntry{entry, m_archive.get(), this};
        }

        /// <summary>
        /// Skips a number of bytes in the underlying byte source.
        /// </summary>
        /// <param name="request">The number of bytes to skip.</param>
        /// <returns>The number of bytes skipped.</returns>
        /// <remarks>
        /// Used for NestedArchiveByteStream where libarchive may want to skip while reading a sub-tar.
        /// </remarks>
        std::int64_t Skip(std::int64_t request)
        {
            return m_context->skip(request);
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

        static constexpr std::size_t ChunkSize = std::size_t{64} * 1024;

        struct Context
        {
            std::function<std::size_t(std::span<std::byte>)> readSync;
            std::function<std::int64_t(std::int64_t)> skip;

            // Only set for the duration of a single operation, by StopTokenScope.
            std::stop_token stopToken;

            std::array<std::byte, ChunkSize> buffer{};
        };

        /// <summary>
        /// Lends a stop_token to the context for the duration of one operation, so the libarchive callbacks
        /// can see it without the reader keeping cancellation state between calls.
        /// </summary>
        struct StopTokenScope
        {
            StopTokenScope(Context& context, std::stop_token token) : m_context(context)
            {
                m_context.stopToken = token;
            }

            ~StopTokenScope()
            {
                m_context.stopToken = std::stop_token{};
            }

            StopTokenScope(const StopTokenScope&) = delete;
            StopTokenScope& operator=(const StopTokenScope&) = delete;
            StopTokenScope(StopTokenScope&&) = delete;
            StopTokenScope& operator=(StopTokenScope&&) = delete;

          private:
            Context& m_context;
        };

        static la_ssize_t ArchiveReadCallback(archive* /* unused */, void* clientData, const void** buffer)
        {
            auto& context = *static_cast<Context*>(clientData);
            if (context.stopToken.stop_requested())
            {
                return ARCHIVE_FATAL;
            }

            const std::size_t bytesRead = context.readSync(std::span(context.buffer));
            *buffer = context.buffer.data();
            return static_cast<la_ssize_t>(bytesRead);
        }

        static la_int64_t ArchiveSkipCallback(archive* /* unused */, void* clientData, la_int64_t request)
        {
            auto& context = *static_cast<Context*>(clientData);
            if (context.stopToken.stop_requested() || request <= 0)
            {
                return ARCHIVE_FATAL;
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
