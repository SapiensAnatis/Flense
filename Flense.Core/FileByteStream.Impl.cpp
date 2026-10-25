module Flense.Core:FileByteStream.Impl;

import :ArchiveReader;
import :FileByteStream;

import std;

namespace Flense::Core
{
    bool FileByteStream::FillBuffer()
    {
        m_bufferStart = m_position;
        m_bufferLength = m_position < m_size ? ReadChunkAt(m_position) : 0;

        return m_bufferLength > 0;
    }

    std::size_t FileByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        std::size_t totalRead = 0;

        while (totalRead < buffer.size())
        {
            const bool inBuffer = m_position >= m_bufferStart && m_position < m_bufferStart + m_bufferLength;
            if (!inBuffer && !FillBuffer())
            {
                break;
            }

            const auto offset = static_cast<std::size_t>(m_position - m_bufferStart);
            const std::size_t toCopy = std::min(m_bufferLength - offset, buffer.size() - totalRead);

            std::ranges::copy(std::span{m_buffer}.subspan(offset, toCopy), buffer.subspan(totalRead).begin());

            totalRead += toCopy;
            m_position += toCopy;
        }

        return totalRead;
    }

    std::int64_t FileByteStream::Skip(const std::int64_t request)
    {
        if (request <= 0)
        {
            return 0;
        }

        const std::uint64_t remaining = m_size > m_position ? m_size - m_position : 0;
        const std::uint64_t toSkip = std::min(static_cast<std::uint64_t>(request), remaining);

        m_position += toSkip;
        return static_cast<std::int64_t>(toSkip);
    }

    std::uint64_t FileByteStream::Size() const
    {
        return m_size;
    }

    std::uint64_t FileByteStream::Position() const
    {
        return m_position;
    }

    static_assert(SkippableByteStream<FileByteStream>);
} // namespace Flense::Core
