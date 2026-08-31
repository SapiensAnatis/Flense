#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

namespace Flense::Benchmarks
{
    /// <summary>
    /// Reads bytes from a file on disk, satisfying the Flense::Core::ByteStream concept.
    /// </summary>
    /// <remarks>
    /// Deliberately built on the raw Win32 file API rather than std::ifstream: iostreams would add a
    /// buffering layer of its own between the benchmark and the thing being measured, and Skip needs
    /// to be a real seek rather than a buffered discard for the archive reader's skip callback to be
    /// worth anything.
    /// </remarks>
    class FileByteStream
    {
      public:
        /// <summary>
        /// Opens a file for sequential reading.
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
        std::filesystem::path m_path;
        void* m_handle;
        uint64_t m_size{0};
        uint64_t m_position{0};
    };
} // namespace Flense::Benchmarks
