module Flense.Core:NestedArchiveByteStream;

import :ArchiveReader;
import std;

namespace Flense::Core
{
    NestedArchiveByteStream::NestedArchiveByteStream(ArchiveEntry* entry, std::span<const std::byte> initialBuffer)
        : m_entry(entry), m_initialBuffer(initialBuffer)
    {
    }

    std::size_t NestedArchiveByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        std::size_t bufferIdx = 0;

        if (m_position < m_initialBuffer.size())
        {
            const std::size_t available = m_initialBuffer.size() - m_position;
            const std::size_t toCopy = std::min(available, buffer.size());

            std::copy_n(m_initialBuffer.begin() + m_position, toCopy, buffer.begin());

            bufferIdx += toCopy;
            m_position += toCopy;
        }

        if (bufferIdx == buffer.size())
        {
            return bufferIdx;
        }

        const std::size_t read = m_entry->ReadInto(buffer.subspan(bufferIdx));
        m_position += read;

        return bufferIdx + read;
    }

    std::int64_t NestedArchiveByteStream::Skip(const std::int64_t request)
    {
        const std::int64_t actualSkip = m_entry->ParentReader()->Skip(request);
        m_position += static_cast<std::uint64_t>(actualSkip);

        return actualSkip;
    }

    std::uint64_t NestedArchiveByteStream::Size()
    {
        return m_entry->Size();
    }

    std::uint64_t NestedArchiveByteStream::Position()
    {
        return m_position;
    }

    static_assert(::Flense::Core::ByteStream<NestedArchiveByteStream>);
} // namespace Flense::Core