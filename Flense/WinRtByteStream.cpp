#include "pch.h"

#include "WinRtByteStream.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage::Streams;

namespace winrt::Flense::implementation
{
    WinRtByteStream::WinRtByteStream(IRandomAccessStream stream)
        : m_stream(stream), m_reader(m_stream.GetInputStreamAt(0))
    {
        m_reader.InputStreamOptions(InputStreamOptions::Partial);
    }

    IAsyncOperation<uint32_t> WinRtByteStream::Read(std::span<std::byte> buffer)
    {
        uint32_t const loaded = co_await m_reader.LoadAsync(static_cast<uint32_t>(buffer.size()));
        if (loaded > 0)
        {
            auto* data = reinterpret_cast<uint8_t*>(buffer.data());
            m_reader.ReadBytes(winrt::array_view<uint8_t>(data, data + loaded));
        }
        co_return loaded;
    }

    uint64_t WinRtByteStream::Size() const
    {
        return m_stream.Size();
    }
} // namespace winrt::Flense::implementation
