#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Flense::Core
{
    /// <summary>
    /// Byte source for ArchiveReader that reads from a block of memory already held by the caller.
    /// Used for reading nested .tar image layers whose compressed bytes have been lifted out of the
    /// outer archive, so that they can be decompressed off the thread walking that archive.
    /// </summary>
    class MemoryByteStream
    {
      public:
        /// <summary>
        /// Constructs a new instance of the MemoryByteStream class.
        /// </summary>
        /// <param name="bytes">Non-owning view over the bytes to read. Must outlive the stream.</param>
        explicit MemoryByteStream(std::span<const std::byte> bytes);

        size_t ReadSync(std::span<std::byte> buffer);
        int64_t Skip(int64_t request);
        uint64_t Size();
        uint64_t Position();

      private:
        std::span<const std::byte> m_bytes;
        uint64_t m_position{0};
    };

} // namespace Flense::Core
