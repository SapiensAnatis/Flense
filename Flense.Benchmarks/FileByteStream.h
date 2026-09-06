#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>

namespace Flense::Benchmarks
{
    /// <summary>
    /// Reads bytes from a file on disk, satisfying the Flense::Core::ByteStream concept.
    /// </summary>
    /// <remarks>
    /// Opened with FILE_FLAG_NO_BUFFERING, so every read comes from disk rather than the OS page
    /// cache - without that, only the very first pass over a file would ever be a genuine cold read,
    /// and every pass after it (in this run or a previous one) would be measuring the cache instead.
    /// Unbuffered reads must land at offsets and lengths that are multiples of the volume's sector
    /// size, into a sector-aligned buffer, so this class keeps one such buffer internally and serves
    /// ReadSync/Skip to its caller as an ordinary, unaligned byte stream on top of it.
    /// </remarks>
    class FileByteStream
    {
      public:
        /// <summary>
        /// Opens a file for sequential, uncached reading.
        /// </summary>
        /// <param name="path">Path to the file.</param>
        /// <exception cref="std::runtime_error">The file could not be opened or its size read.</exception>
        explicit FileByteStream(const std::filesystem::path& path);

        ~FileByteStream();

        FileByteStream(const FileByteStream&) = delete;
        FileByteStream& operator=(const FileByteStream&) = delete;

        /// <summary>
        /// Reads up to buffer.size() bytes from the current position.
        /// </summary>
        /// <param name="buffer">The buffer to fill.</param>
        /// <returns>The number of bytes read; 0 at end of file.</returns>
        size_t ReadSync(std::span<std::byte> buffer);

        /// <summary>
        /// Seeks forwards by up to request bytes, clamped to the end of the file.
        /// </summary>
        /// <param name="request">The number of bytes to skip.</param>
        /// <returns>The number of bytes actually skipped.</returns>
        int64_t Skip(int64_t request);

        /// <summary>
        /// The total size of the file in bytes.
        /// </summary>
        uint64_t Size() const;

        /// <summary>
        /// The current read position in bytes from the start of the file.
        /// </summary>
        /// <remarks>
        /// Required by the ByteStream concept rather than by anything here - ArchiveReader itself only
        /// ever uses ReadSync and Skip.
        /// </remarks>
        uint64_t Position() const;

      private:
        /// <summary>
        /// Reads the sector-aligned chunk covering the current position into the aligned buffer,
        /// discarding whatever it previously held.
        /// </summary>
        /// <returns>Whether the file has any bytes left at or beyond the current position.</returns>
        bool FillBuffer();

        /// <summary>
        /// Closes a HANDLE.
        /// </summary>
        struct HandleDeleter
        {
            void operator()(void* handle) const noexcept;
        };

        /// <summary>
        /// Frees a buffer allocated with _aligned_malloc.
        /// </summary>
        struct AlignedBufferDeleter
        {
            void operator()(std::byte* buffer) const noexcept;
        };

        std::filesystem::path m_path;
        std::unique_ptr<void, HandleDeleter> m_handle;
        uint64_t m_size{0};
        uint64_t m_position{0};

        size_t m_sectorSize{0};
        std::unique_ptr<std::byte[], AlignedBufferDeleter> m_alignedBuffer;
        size_t m_bufferCapacity{0};

        /// <summary>The file offset the aligned buffer's contents were read from.</summary>
        uint64_t m_bufferStart{0};

        /// <summary>How much of the aligned buffer, from m_bufferStart, is real file data rather than
        /// past-EOF padding.</summary>
        size_t m_bufferValidLength{0};
    };
} // namespace Flense::Benchmarks
