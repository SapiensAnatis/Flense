export module Flense.Core:NestedArchiveByteStream;

import :ArchiveReader;
import std;

export namespace Flense::Core
{
    /// <summary>
    /// Byte source for ArchiveReader that reads from the libarchive handle of an existing open archive.
    /// Used for reading nested .tar image layers in Docker images.
    /// </summary>
    class NestedArchiveByteStream
    {
      public:
        /// <summary>
        /// Constructs a new instance of the NestedArchiveByteStream class.
        /// </summary>
        /// <param name="entry">Non-owning reference to the archive entry.</param>
        /// <param name="initialBuffer">An initial buffer that may have already been read from the entry.</param>
        NestedArchiveByteStream(ArchiveEntry* entry, std::span<const std::byte> initialBuffer);

        std::size_t ReadSync(std::span<std::byte> buffer);
        std::int64_t Skip(std::int64_t request);
        std::uint64_t Size();
        std::uint64_t Position();

      private:
        std::uint64_t m_position{0};
        std::span<const std::byte> m_initialBuffer;
        ArchiveEntry* m_entry{nullptr};
    };

} // namespace Flense::Core
