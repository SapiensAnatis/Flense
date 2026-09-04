export module Flense.Core:ChannelByteStream;

import :BufferPool;
import :Channel;
import std;

export namespace Flense::Core
{
    /// <summary>
    /// A rented buffer paired with the number of bytes actually filled in it - a chunk's capacity is fixed by
    /// the BufferPool it came from, but the final chunk of a stream is usually only partially filled.
    /// </summary>
    struct BufferChunk
    {
        RentedBuffer buffer;
        std::size_t length;
    };

    /// <summary>
    /// A ByteStream that reads sequentially from a Channel of BufferChunks, letting a producer thread stream a
    /// source's contents chunk-by-chunk to a consumer running on another thread. Each chunk's buffer is returned
    /// to its pool as soon as it's fully consumed.
    /// </summary>
    class ChannelByteStream
    {
      public:
        /// <summary>
        /// Constructs a new instance of the ChannelByteStream class.
        /// </summary>
        /// <param name="channel">Non-owning reference to the channel to read buffer chunks from. Must outlive
        /// this instance.</param>
        /// <param name="totalSize">The total number of bytes that will be delivered across the channel, as
        /// reported by Size().</param>
        ChannelByteStream(Channel<BufferChunk>* channel, std::uint64_t totalSize);

        std::size_t ReadSync(std::span<std::byte> buffer);
        [[nodiscard]] std::uint64_t Size() const;
        [[nodiscard]] std::uint64_t Position() const;

      private:
        Channel<BufferChunk>* m_channel;
        std::uint64_t m_totalSize;
        std::uint64_t m_position{0};

        std::optional<BufferChunk> m_current;
        std::size_t m_currentOffset{0};
    };

} // namespace Flense::Core
