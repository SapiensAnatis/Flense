#include "pch.h"

#include "ChunkPool.h"

#include <chrono>
#include <mutex>
#include <utility>

namespace Flense::Core
{
    ChunkPool::ChunkPool(const size_t chunkSize, const size_t maxChunks, std::stop_token stopToken)
        : m_chunkSize(chunkSize), m_maxChunks(maxChunks > 0 ? maxChunks : 1), m_stopToken(std::move(stopToken))
    {
    }

    Chunk ChunkPool::Acquire()
    {
        const auto start = std::chrono::steady_clock::now();

        std::unique_lock lock{m_mutex};

        // Recorded before waiting, so that the counter measures time the cap actually cost rather than
        // the cost of asking.
        const bool wouldBlock = m_free.empty() && m_createdChunks >= m_maxChunks;

        m_chunkFreed.wait(lock, m_stopToken, [this] { return !m_free.empty() || m_createdChunks < m_maxChunks; });

        if (wouldBlock)
        {
            const auto waited = std::chrono::steady_clock::now() - start;
            m_stallNanos.fetch_add(static_cast<uint64_t>(std::chrono::nanoseconds{waited}.count()),
                                   std::memory_order_relaxed);
        }

        if (!m_free.empty())
        {
            Chunk chunk = std::move(m_free.back());
            m_free.pop_back();
            chunk.used = 0;

            return chunk;
        }

        if (m_createdChunks < m_maxChunks)
        {
            m_createdChunks += 1;

            return Chunk{.buffer = std::vector<std::byte>(m_chunkSize), .used = 0};
        }

        // Only reachable when cancellation broke the wait.
        return Chunk{};
    }

    void ChunkPool::Release(Chunk chunk)
    {
        if (chunk.buffer.empty())
        {
            return;
        }

        {
            const std::scoped_lock lock{m_mutex};

            chunk.used = 0;
            m_free.push_back(std::move(chunk));
        }

        m_chunkFreed.notify_one();
    }
} // namespace Flense::Core
