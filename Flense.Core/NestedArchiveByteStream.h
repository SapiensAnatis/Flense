#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Flense::Core
{
    class ArchiveEntry;

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
        explicit NestedArchiveByteStream(ArchiveEntry* entry, ArchiveReader* reader,
                                         std::span<std::byte> initialBuffer);

        size_t ReadSync(std::span<std::byte> buffer);
        int64_t Skip(int64_t request);
        uint64_t Size();
        uint64_t Position();

      private:
        uint64_t m_position{0};
        std::span<std::byte> m_initialBuffer;
        ArchiveEntry* m_entry{nullptr};
        ArchiveReader* m_reader{nullptr};
    };

} // namespace Flense::Core
