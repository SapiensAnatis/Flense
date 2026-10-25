module Flense.Core:Internal.ChannelByteStream.Impl;

import :Internal.BufferPool;
import :Internal.Channel;
import :Internal.ChannelByteStream;
import std;

namespace Flense::Core
{
    ChannelByteStream::ChannelByteStream(Channel<BufferChunk>::Receiver&& receiver, std::uint64_t totalSize)
        : m_receiver(std::move(receiver)), m_totalSize(totalSize)
    {
    }

    std::size_t ChannelByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        std::size_t totalRead = 0;

        while (totalRead < buffer.size())
        {
            if (!m_current || m_currentOffset == m_current->length)
            {
                std::optional<BufferChunk> next = m_receiver.Pop();
                if (!next)
                {
                    break;
                }

                m_current = std::move(next);
                m_currentOffset = 0;
            }

            const std::span<std::byte> available =
                m_current->buffer.Buffer().subspan(m_currentOffset, m_current->length - m_currentOffset);
            const std::span<std::byte> dest = buffer.subspan(totalRead);
            const std::size_t chunkSize = std::min(available.size(), dest.size());

            std::ranges::copy(available.first(chunkSize), dest.begin());

            totalRead += chunkSize;
            m_currentOffset += chunkSize;
        }

        m_position += totalRead;
        return totalRead;
    }

    std::uint64_t ChannelByteStream::Size() const
    {
        return m_totalSize;
    }

    std::uint64_t ChannelByteStream::Position() const
    {
        return m_position;
    }
} // namespace Flense::Core
