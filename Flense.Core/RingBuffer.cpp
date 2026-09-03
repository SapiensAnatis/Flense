module;

#include "Mutex.h"

module Flense.Core;

import :RingBuffer;

import std;

namespace Flense::Core
{
    RingBuffer::RingBuffer(const std::size_t capacity) : m_buffer(capacity)
    {
        if (capacity == 0)
        {
            throw std::invalid_argument("RingBuffer: capacity cannot be zero");
        }
    }

    std::size_t RingBuffer::Write(const std::span<const std::byte> source)
    {
        const MutexLocker locker(&m_mutex);

        std::size_t written = 0;

        while (written < source.size())
        {
            m_readMoved.wait(m_mutex, [this]() REQUIRES(m_mutex) { return m_closed || FreeBytes() > 0; });

            if (m_closed)
            {
                return written;
            }

            const std::span writeSpan = std::span(m_buffer).subspan(m_writeIndex);
            const std::size_t chunkSize = std::min({source.size() - written, FreeBytes(), writeSpan.size()});

            std::ranges::copy(source.subspan(written, chunkSize), writeSpan.begin());

            m_writeIndex = (m_writeIndex + chunkSize) % m_buffer.size();
            m_size += chunkSize;
            written += chunkSize;

            m_writeMoved.notify_one();
        }

        return written;
    }

    std::size_t RingBuffer::Read(const std::span<std::byte> target)
    {
        const MutexLocker locker(&m_mutex);

        std::size_t read = 0;

        while (read < target.size())
        {
            m_writeMoved.wait(m_mutex, [this]() REQUIRES(m_mutex) { return m_closed || m_size > 0; });

            if (m_size == 0)
            {
                return read;
            }

            const std::span readSpan = std::span(m_buffer).subspan(m_readIndex);
            const std::size_t chunkSize = std::min({target.size() - read, m_size, readSpan.size()});

            std::ranges::copy(readSpan.first(chunkSize), target.subspan(read).begin());

            m_readIndex = (m_readIndex + chunkSize) % m_buffer.size();
            m_size -= chunkSize;
            read += chunkSize;

            m_readMoved.notify_one();
        }

        return read;
    }

    void RingBuffer::Close()
    {
        const MutexLocker locker(&m_mutex);

        if (!m_closed)
        {
            m_closed = true;
            m_readMoved.notify_all();
            m_writeMoved.notify_all();
        }
    }

    std::size_t RingBuffer::FreeBytes() const
    {
        return m_buffer.size() - m_size;
    }

} // namespace Flense::Core