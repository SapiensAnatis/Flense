#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <stop_token>
#include <vector>

namespace Flense::Core
{
    /// <summary>
    /// A fixed-capacity block of bytes borrowed from a ChunkPool.
    /// </summary>
    /// <remarks>
    /// buffer is always the pool's chunk size; used says how much of it holds data, which is less than
    /// the whole only for the final chunk of a layer.
    /// </remarks>
    struct Chunk
    {
        std::vector<std::byte> buffer;
        size_t used{0};
    };

    /// <summary>
    /// A bounded supply of equally-sized buffers, handed out to whoever is reading ahead and returned
    /// as soon as their contents have been consumed.
    /// </summary>
    /// <remarks>
    /// This is what caps the parser's memory. Reading ahead is what lets layers be decompressed in
    /// parallel at all - the archive is forward-only, so bytes have to be taken before the walk can
    /// advance - and the pool is what stops that read-ahead from being unbounded.
    ///
    /// Handing out fixed chunks rather than reserving a whole layer's worth up front matters: a layer
    /// larger than the entire pool still streams through it, a chunk at a time, instead of having to
    /// be admitted wholesale or not at all.
    ///
    /// Chunks are created lazily, so a small image never touches the full budget.
    /// </remarks>
    class ChunkPool
    {
      public:
        /// <summary>
        /// Constructs a new instance of the ChunkPool class.
        /// </summary>
        /// <param name="chunkSize">The size of each chunk in bytes.</param>
        /// <param name="maxChunks">The number of chunks that may exist at once.</param>
        /// <param name="stopToken">Token that releases any thread blocked in Acquire.</param>
        ChunkPool(size_t chunkSize, size_t maxChunks, std::stop_token stopToken);

        /// <summary>
        /// Blocks until a chunk is free, then hands it over.
        /// </summary>
        /// <returns>A chunk with used == 0, or one with an empty buffer if cancellation was requested.</returns>
        Chunk Acquire();

        /// <summary>
        /// Returns a chunk so that someone else can have it.
        /// </summary>
        /// <param name="chunk">The chunk. An empty one is ignored, so a cancelled Acquire can be fed straight back.</param>
        void Release(Chunk chunk);

        size_t ChunkSize() const
        {
            return m_chunkSize;
        }

        /// <summary>
        /// How long callers have spent blocked in Acquire, in total - i.e. how much the memory cap has
        /// cost in read-ahead. Zero means the pool was never the constraint.
        /// </summary>
        std::chrono::nanoseconds StalledFor() const
        {
            return std::chrono::nanoseconds{m_stallNanos.load(std::memory_order_relaxed)};
        }

      private:
        std::mutex m_mutex;
        std::condition_variable_any m_chunkFreed;
        std::vector<Chunk> m_free;
        size_t m_chunkSize;
        size_t m_maxChunks;
        size_t m_createdChunks{0};
        std::atomic<uint64_t> m_stallNanos{0};
        std::stop_token m_stopToken;
    };

} // namespace Flense::Core
