#pragma once

#include "ChunkPool.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <span>
#include <stop_token>

namespace Flense::Core
{
    /// <summary>
    /// A one-writer, one-reader byte pipe built out of pooled chunks: the thread walking the outer
    /// archive pushes a layer's compressed bytes in, and a worker thread reads them out as a ByteStream
    /// while they are still arriving.
    /// </summary>
    /// <remarks>
    /// Chunks go back to the pool the moment the reader finishes with them, so a layer's peak cost is
    /// whatever is in flight between the two threads rather than its whole compressed size. That is
    /// what keeps a layer bigger than the entire pool from being a special case.
    ///
    /// Shared between two threads, so it must outlive both - hold it by shared_ptr.
    /// </remarks>
    class ChunkQueueByteStream
    {
      public:
        /// <summary>
        /// Constructs a new instance of the ChunkQueueByteStream class.
        /// </summary>
        /// <param name="pool">The pool that chunks are returned to. Must outlive the stream.</param>
        /// <param name="size">The layer's total compressed size, from the outer archive's header.</param>
        /// <param name="stopToken">Token that releases a reader blocked waiting for more bytes.</param>
        ChunkQueueByteStream(ChunkPool& pool, uint64_t size, std::stop_token stopToken);

        ~ChunkQueueByteStream();

        ChunkQueueByteStream(const ChunkQueueByteStream&) = delete;
        ChunkQueueByteStream& operator=(const ChunkQueueByteStream&) = delete;

        /// <summary>
        /// Hands a filled chunk to the reader. Writer side.
        /// </summary>
        /// <param name="chunk">The chunk.</param>
        /// <returns>False once the reader has stopped reading, meaning the rest of the layer can be
        /// skipped rather than copied. The chunk is returned to the pool either way.</returns>
        bool Push(Chunk chunk);

        /// <summary>
        /// Signals that no further chunks are coming, so the reader sees a clean end of stream rather
        /// than waiting for bytes that will never arrive. Writer side.
        /// </summary>
        void Finish();

        /// <summary>
        /// Signals that the reader wants nothing more, and returns everything it still holds to the
        /// pool. Reader side; must be called, or the writer will queue chunks nobody drains.
        /// </summary>
        void ConsumerFinished();

        size_t ReadSync(std::span<std::byte> buffer);
        int64_t Skip(int64_t request);
        uint64_t Size();
        uint64_t Position();

      private:
        /// <summary>
        /// Makes m_current hold unread bytes, waiting for the writer if need be. Caller must hold the lock.
        /// </summary>
        /// <param name="lock">The held lock.</param>
        /// <returns>False if the stream is exhausted or cancelled.</returns>
        bool EnsureCurrentChunk(std::unique_lock<std::mutex>& lock);

        /// <summary>
        /// Returns everything held to the pool. Caller must hold the lock.
        /// </summary>
        void ReleaseHeldChunks();

        ChunkPool* m_pool;

        std::mutex m_mutex;
        std::condition_variable_any m_chunkQueued;
        std::deque<Chunk> m_queue;

        Chunk m_current;
        size_t m_currentOffset{0};

        bool m_producerFinished{false};
        bool m_consumerFinished{false};

        uint64_t m_size;
        uint64_t m_position{0};

        std::stop_token m_stopToken;
    };

} // namespace Flense::Core
