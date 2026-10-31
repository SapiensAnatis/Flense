export module Flense.Core:FileByteStream;

import :Internal.FileHandle;

import std;

export namespace Flense::Core
{
    /// <summary>
    /// A ByteStream over a file on disk.
    /// </summary>
    /// <remarks>
    /// Reads the file in large chunks, whatever size the caller asks for, as small reads are noticeably slower from a
    /// cold disk. Blocking, like every other ByteStream here, so it should be driven from a background thread.
    /// </remarks>
    class FileByteStream
    {
      public:
        /// <exception cref="std::filesystem::filesystem_error">The file could not be opened.</exception>
        explicit FileByteStream(const std::filesystem::path& path);

        ~FileByteStream() = default;

        FileByteStream(const FileByteStream&) = delete;
        FileByteStream& operator=(const FileByteStream&) = delete;
        FileByteStream(FileByteStream&&) = delete;
        FileByteStream& operator=(FileByteStream&&) = delete;

        /// <exception cref="std::filesystem::filesystem_error">The file could not be read.</exception>
        std::size_t ReadSync(std::span<std::byte> buffer);

        std::int64_t Skip(std::int64_t request);

        [[nodiscard]] std::uint64_t Size() const;

        [[nodiscard]] std::uint64_t Position() const;

      private:
        static constexpr std::size_t ChunkSize = std::size_t{1024} * 1024;

        /// <summary>
        /// Refills the chunk buffer from the current position.
        /// </summary>
        /// <returns>Whether any bytes were read.</returns>
        bool FillBuffer();

        /// <summary>
        /// Reads up to one chunk from an offset into the chunk buffer.
        /// </summary>
        /// <returns>The number of bytes read.</returns>
        std::size_t ReadChunkAt(std::uint64_t offset);

        std::filesystem::path m_path;

        FileHandle m_handle;

        std::uint64_t m_size{0};
        std::uint64_t m_position{0};

        std::vector<std::byte> m_buffer = std::vector<std::byte>(ChunkSize);
        std::uint64_t m_bufferStart{0};
        std::size_t m_bufferLength{0};
    };
} // namespace Flense::Core
