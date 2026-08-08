#include "pch.h"

#include "ArchiveReader.h"
#include "ChunkQueueByteStream.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <span>
#include <utility>

namespace Flense::Core
{
    ChunkQueueByteStream::ChunkQueueByteStream(ChunkPool& pool, const uint64_t size, std::stop_token stopToken)
        : m_pool(&pool), m_size(size), m_stopToken(std::move(stopToken))
    {
    }

    ChunkQueueByteStream::~ChunkQueueByteStream()
    {
        const std::scoped_lock lock{m_mutex};
        ReleaseHeldChunks();
    }

    bool ChunkQueueByteStream::Push(Chunk chunk)
    {
        {
            const std::scoped_lock lock{m_mutex};

            if (m_consumerFinished)
            {
                m_pool->Release(std::move(chunk));
                return false;
            }

            m_queue.push_back(std::move(chunk));
        }

        m_chunkQueued.notify_one();

        return true;
    }

    void ChunkQueueByteStream::Finish()
    {
        {
            const std::scoped_lock lock{m_mutex};
            m_producerFinished = true;
        }

        m_chunkQueued.notify_one();
    }

    void ChunkQueueByteStream::ConsumerFinished()
    {
        const std::scoped_lock lock{m_mutex};

        m_consumerFinished = true;
        ReleaseHeldChunks();
    }

    bool ChunkQueueByteStream::EnsureCurrentChunk(std::unique_lock<std::mutex>& lock)
    {
        if (m_currentOffset < m_current.used)
        {
            return true;
        }

        if (!m_current.buffer.empty())
        {
            m_pool->Release(std::move(m_current));
            m_current = Chunk{};
        }

        m_currentOffset = 0;

        m_chunkQueued.wait(lock, m_stopToken, [this] { return !m_queue.empty() || m_producerFinished; });

        if (m_queue.empty())
        {
            // The writer is done, or cancellation broke the wait.
            return false;
        }

        m_current = std::move(m_queue.front());
        m_queue.pop_front();

        // A zero-length final chunk is possible for an empty entry; treat it as end of stream.
        return m_current.used > 0;
    }

    void ChunkQueueByteStream::ReleaseHeldChunks()
    {
        if (!m_current.buffer.empty())
        {
            m_pool->Release(std::move(m_current));
            m_current = Chunk{};
        }

        m_currentOffset = 0;

        while (!m_queue.empty())
        {
            m_pool->Release(std::move(m_queue.front()));
            m_queue.pop_front();
        }
    }

    size_t ChunkQueueByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        std::unique_lock lock{m_mutex};

        size_t written = 0;

        while (written < buffer.size())
        {
            if (!EnsureCurrentChunk(lock))
            {
                break;
            }

            const size_t available = m_current.used - m_currentOffset;
            const size_t toCopy = std::min(available, buffer.size() - written);

            std::copy_n(m_current.buffer.begin() + m_currentOffset, toCopy, buffer.begin() + written);

            m_currentOffset += toCopy;
            m_position += toCopy;
            written += toCopy;
        }

        return written;
    }

    int64_t ChunkQueueByteStream::Skip(const int64_t request)
    {
        if (request <= 0)
        {
            return 0;
        }

        std::unique_lock lock{m_mutex};

        uint64_t skipped = 0;
        const uint64_t wanted = static_cast<uint64_t>(request);

        while (skipped < wanted)
        {
            if (!EnsureCurrentChunk(lock))
            {
                break;
            }

            const uint64_t available = m_current.used - m_currentOffset;
            const uint64_t toSkip = std::min(available, wanted - skipped);

            m_currentOffset += static_cast<size_t>(toSkip);
            m_position += toSkip;
            skipped += toSkip;
        }

        return static_cast<int64_t>(skipped);
    }

    uint64_t ChunkQueueByteStream::Size()
    {
        return m_size;
    }

    uint64_t ChunkQueueByteStream::Position()
    {
        const std::scoped_lock lock{m_mutex};
        return m_position;
    }

    static_assert(::Flense::Core::ByteStream<ChunkQueueByteStream>);
} // namespace Flense::Core
