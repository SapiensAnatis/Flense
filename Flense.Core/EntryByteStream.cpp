#include "pch.h"

#include "ArchiveReader.h"
#include "EntryByteStream.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Flense::Core
{
    namespace
    {
        constexpr size_t SkipBufferSize = 64 * 1024;
    }

    EntryByteStream::EntryByteStream(ArchiveEntry* entry, const std::span<const std::byte> prefix)
        : m_entry(entry), m_prefix(prefix)
    {
    }

    size_t EntryByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        size_t written = 0;

        if (m_position < m_prefix.size())
        {
            const size_t available = m_prefix.size() - static_cast<size_t>(m_position);
            const size_t toCopy = std::min(available, buffer.size());

            std::copy_n(m_prefix.begin() + m_position, toCopy, buffer.begin());

            written += toCopy;
            m_position += toCopy;
        }

        if (written == buffer.size())
        {
            return written;
        }

        const size_t read = m_entry->ReadInto(buffer.subspan(written));
        m_position += read;

        return written + read;
    }

    int64_t EntryByteStream::Skip(const int64_t request)
    {
        if (request <= 0)
        {
            return 0;
        }

        std::array<std::byte, SkipBufferSize> scratch{};

        uint64_t skipped = 0;
        const uint64_t wanted = static_cast<uint64_t>(request);

        while (skipped < wanted)
        {
            const size_t toRead = static_cast<size_t>(std::min<uint64_t>(scratch.size(), wanted - skipped));
            const size_t read = ReadSync(std::span{scratch}.first(toRead));

            if (read == 0)
            {
                break;
            }

            skipped += read;
        }

        return static_cast<int64_t>(skipped);
    }

    uint64_t EntryByteStream::Size()
    {
        return m_entry->Size();
    }

    uint64_t EntryByteStream::Position()
    {
        return m_position;
    }

    static_assert(::Flense::Core::ByteStream<EntryByteStream>);
} // namespace Flense::Core
