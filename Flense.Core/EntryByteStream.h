#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Flense::Core
{
    class ArchiveEntry;

    /// <summary>
    /// Byte source for ArchiveReader that reads a nested archive straight out of an entry of the
    /// archive containing it, without buffering.
    /// </summary>
    /// <remarks>
    /// Used when layers are parsed on the same thread that walks the outer archive, where there is
    /// nothing to be gained by taking a copy of the bytes first.
    ///
    /// Skip reads and discards rather than seeking the outer source: a skip here means bytes of this
    /// entry's decompressed body, which is not the same quantity as bytes of the outer stream.
    /// </remarks>
    class EntryByteStream
    {
      public:
        /// <summary>
        /// Constructs a new instance of the EntryByteStream class.
        /// </summary>
        /// <param name="entry">Non-owning reference to the archive entry. Must outlive the stream, and
        /// must not have been invalidated by a call to the owning reader's Next().</param>
        /// <param name="prefix">Bytes already read from the entry, replayed before reading resumes.</param>
        EntryByteStream(ArchiveEntry* entry, std::span<const std::byte> prefix);

        size_t ReadSync(std::span<std::byte> buffer);
        int64_t Skip(int64_t request);
        uint64_t Size();
        uint64_t Position();

      private:
        ArchiveEntry* m_entry;
        std::span<const std::byte> m_prefix;
        uint64_t m_position{0};
    };

} // namespace Flense::Core
