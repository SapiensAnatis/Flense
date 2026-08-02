#include "pch.h"

#include "WinRtByteStream.h"

#include <algorithm>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage::Streams;

namespace winrt::Flense::implementation
{
    WinRtByteStream::WinRtByteStream(IRandomAccessStream stream)
        : m_stream(stream), m_reader(m_stream.GetInputStreamAt(0))
    {
        m_reader.InputStreamOptions(InputStreamOptions::Partial);
    }

    size_t WinRtByteStream::ReadSync(std::span<std::byte> buffer)
    {
        uint32_t const loaded = m_reader.LoadAsync(static_cast<uint32_t>(buffer.size())).get();
        if (loaded > 0)
        {
            auto* data = reinterpret_cast<uint8_t*>(buffer.data());
            m_reader.ReadBytes(winrt::array_view<uint8_t>(data, data + loaded));
            m_position += loaded;
        }
        return loaded;
    }

    int64_t WinRtByteStream::Skip(int64_t request)
    {
        uint64_t const size = m_stream.Size();
        uint64_t const maxSkip = size > m_position ? size - m_position : 0;
        uint64_t const actualSkip = std::min<uint64_t>(static_cast<uint64_t>(request), maxSkip);
        if (actualSkip == 0)
        {
            return 0;
        }

        m_position += actualSkip;

        // The reader's underlying input stream (from GetInputStreamAt) tracks its own
        // read cursor independently of IRandomAccessStream::Position, so skipping means
        // re-anchoring a fresh view at the new logical position rather than seeking m_stream.
        m_reader = DataReader(m_stream.GetInputStreamAt(m_position));
        m_reader.InputStreamOptions(InputStreamOptions::Partial);

        return static_cast<int64_t>(actualSkip);
    }

    uint64_t WinRtByteStream::Size() const
    {
        return m_stream.Size();
    }
} // namespace winrt::Flense::implementation
