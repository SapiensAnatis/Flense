module;
#include <archive.h>
#include <archive_entry.h>
export module Flense.Core:ArchiveReader;

import :Error;
import :Filesystem;
import std;

namespace Flense::Core
{
    /// <summary>
    /// Gets a formatted string describing a libarchive error.
    /// </summary>
    /// <param name="functionName">The most recent libarchive function called.</param>
    /// <param name="archive">Archive handle.</param>
    /// <returns>A string containing the function name and error, e.g. "archive_read_open1: Unrecognized archive
    /// format"</returns>
    inline std::string ErrorString(const std::string_view functionName, archive* archive)
    {
        const char* message = archive_error_string(archive);
        const char* errorDescription = (message != nullptr) ? message : "unknown libarchive error";

        return std::format("{}: {}", functionName, errorDescription);
    }

    constexpr std::size_t ArchiveChunkSize = std::size_t{64} * 1024;

    struct ArchiveContext
    {
        std::function<std::size_t(std::span<std::byte>)> readSync;
        std::function<std::int64_t(std::int64_t)> skip;

        // Only set for the duration of a single operation, by StopTokenScope.
        std::stop_token stopToken;

        std::array<std::byte, ArchiveChunkSize> buffer{};

        std::exception_ptr error;
    };

} // namespace Flense::Core

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
        { source.Size() } -> std::convertible_to<std::uint64_t>;
        { source.Position() } -> std::convertible_to<std::uint64_t>;
    };

    template <typename T>
    concept SkippableByteStream = ByteStream<T> && requires(T& source, std::int64_t requestedSkip) {
        { source.Skip(requestedSkip) } -> std::convertible_to<std::int64_t>;
    };

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
        /// <exception cref="ArchiveError">The entry's body could not be read in full.</exception>
        [[nodiscard]] std::size_t ReadInto(std::span<std::byte> buffer) const
        {
            std::size_t totalRead = 0;
            while (totalRead < buffer.size())
            {
                const std::span<std::byte> remaining = buffer.subspan(totalRead);
                const la_ssize_t bytesRead = archive_read_data(m_archive, remaining.data(), remaining.size());

                if (bytesRead == 0)
                {
                    break;
                }

                // Cancellation can't surface here: the read callback only reports it while the context
                // is holding a stop token, which ArchiveReader::Next lends for its own duration alone.
                // So a negative result is a genuine failure rather than the caller asking us to stop.
                if (bytesRead < 0)
                {
                    if (m_context->error != nullptr)
                    {
                        std::rethrow_exception(m_context->error);
                    }

                    throw ArchiveError{ArchiveErrorKind::Corrupt, ErrorString("archive_read_data", m_archive)};
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

      private:
        friend class ArchiveReader;

        ArchiveEntry(archive_entry* entry, archive* archive, ArchiveContext* context)
            : m_entry(entry), m_archive(archive), m_context(context)
        {
        }

        archive_entry* m_entry;
        archive* m_archive;
        ArchiveContext* m_context;
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

            auto context = std::make_unique<ArchiveContext>();
            context->readSync = [&source](std::span<std::byte> buffer) { return source.ReadSync(buffer); };

            archive_read_set_callback_data(archive.get(), context.get());
            archive_read_set_read_callback(archive.get(), &ArchiveReadCallback);

            if constexpr (SkippableByteStream<TStream>)
            {
                context->skip = [&source](std::int64_t request) { return source.Skip(request); };
                archive_read_set_skip_callback(archive.get(), &ArchiveSkipCallback);
            }

            const int result = archive_read_open1(archive.get());

            if (result == ARCHIVE_OK || result == ARCHIVE_WARN)
            {
                return ArchiveReader{std::move(archive), std::move(context)};
            }

            // TODO(2026-10-23): Handle ARCHIVE_RETRY?

            if (context->error != nullptr)
            {
                std::rethrow_exception(context->error);
            }

            throw ArchiveError{ArchiveErrorKind::Unreadable, ErrorString("archive_read_open1", archive.get())};
        }

        /// <summary>
        /// Advances to the next entry in the archive. Automatically skips any part of the previous
        /// entry's data that wasn't consumed via ArchiveEntry::ReadInto(). Returns nullopt once
        /// the archive is exhausted.
        /// </summary>
        /// <param name="stopToken">Cancellation token for interrupting the operation.</param>
        /// <remarks>
        /// Invalidates any string_views or other references obtained from previous entries.
        /// </remarks>
        /// <exception cref="ArchiveError">The archive could not be read.</exception>
        /// <exception cref="OperationCancelledError">stopToken was triggered before or during the call.</exception>
        std::optional<ArchiveEntry> Next(std::stop_token stopToken)
        {
            const StopTokenScope scope{*m_context, std::move(stopToken)};

            if (m_context->stopToken.stop_requested())
            {
                throw OperationCancelledError{};
            }

            archive_entry* entry = nullptr;
            const int result = archive_read_next_header(m_archive.get(), &entry);

            // ARCHIVE_WARN means the header was read but something about it was unusual (an attribute
            // libarchive doesn't support, say) - the entry itself is still usable.
            if (result == ARCHIVE_OK || result == ARCHIVE_WARN)
            {
                return ArchiveEntry{entry, m_archive.get(), m_context.get()};
            }

            // We sometimes return ARCHIVE_FATAL as a hacky way to indicate cancellation
            if (m_context->stopToken.stop_requested())
            {
                throw OperationCancelledError{};
            }

            // All good but no more entries
            if (result == ARCHIVE_EOF)
            {
                return std::nullopt;
            }

            // TODO(2026-10-23): Handle ARCHIVE_RETRY?

            if (m_context->error != nullptr)
            {
                std::rethrow_exception(m_context->error);
            }

            throw ArchiveError{ArchiveErrorKind::Corrupt, ErrorString("archive_read_next_header", m_archive.get())};
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

        /// <summary>
        /// Lends a stop_token to the context for the duration of one operation, so the libarchive callbacks
        /// can see it without the reader keeping cancellation state between calls.
        /// </summary>
        struct StopTokenScope
        {
            StopTokenScope(ArchiveContext& context, std::stop_token token) : m_context(context)
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
            ArchiveContext& m_context;
        };

        static la_ssize_t ArchiveReadCallback(archive* /* unused */, void* clientData, const void** buffer) noexcept
        {
            auto& context = *static_cast<ArchiveContext*>(clientData);

            if (context.stopToken.stop_requested())
            {
                return ARCHIVE_FATAL;
            }

            if (context.error != nullptr)
            {
                // May be left over from returning 0 from ArchiveSkipCallback, which tells libarchive to read and
                // discard instead.
                return ARCHIVE_FATAL;
            }

            // We compile with /EHsc, and at this point we will have libarchive's C frames in the callstack, so we
            // should try not to unwind through them as things may break. Instead, store the exception to be thrown
            // later.

            try
            {
                const std::size_t bytesRead = context.readSync(std::span(context.buffer));
                *buffer = context.buffer.data();
                return static_cast<la_ssize_t>(bytesRead);
            }
            catch (...)
            {
                context.error = std::current_exception();
                return ARCHIVE_FATAL;
            }
        }

        static la_int64_t ArchiveSkipCallback(archive* /* unused */, void* clientData, la_int64_t request) noexcept
        {
            auto& context = *static_cast<ArchiveContext*>(clientData);
            if (context.stopToken.stop_requested() || request <= 0)
            {
                // Our version of libarchive doesn't handle < 0 returns from skip properly.
                // TODO(2026-10-24): Change this when upgrading to 3.8.10 which has a fix
                return 0;
            }

            try
            {
                return context.skip(request);
            }
            catch (...)
            {
                context.error = std::current_exception();
                return 0;
            }
        }

        ArchiveReader(ArchivePtr&& archive, std::unique_ptr<ArchiveContext>&& context)
            : m_archive(std::move(archive)), m_context(std::move(context))
        {
        }

        ArchivePtr m_archive;
        std::unique_ptr<ArchiveContext> m_context;
    };

} // namespace Flense::Core
