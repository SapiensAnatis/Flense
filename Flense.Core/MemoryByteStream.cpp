#include "pch.h"

#include "ArchiveReader.h"
#include "MemoryByteStream.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Flense::Core
{
    MemoryByteStream::MemoryByteStream(const std::span<const std::byte> bytes) : m_bytes(bytes)
    {
    }

    size_t MemoryByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        const size_t available = m_bytes.size() - m_position;
        const size_t toCopy = std::min(available, buffer.size());

        std::copy_n(m_bytes.begin() + m_position, toCopy, buffer.begin());
        m_position += toCopy;

        return toCopy;
    }

    int64_t MemoryByteStream::Skip(const int64_t request)
    {
        if (request <= 0)
        {
            return 0;
        }

        const uint64_t available = m_bytes.size() - m_position;
        const uint64_t toSkip = std::min(available, static_cast<uint64_t>(request));

        m_position += toSkip;

        return static_cast<int64_t>(toSkip);
    }

    uint64_t MemoryByteStream::Size()
    {
        return m_bytes.size();
    }

    uint64_t MemoryByteStream::Position()
    {
        return m_position;
    }

    static_assert(::Flense::Core::ByteStream<MemoryByteStream>);
} // namespace Flense::Core
