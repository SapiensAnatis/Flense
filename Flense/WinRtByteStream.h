#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>

#include <cstdint>
#include <span>

namespace winrt::Flense::implementation
{
    // Wraps a WinRT random-access stream so it satisfies Flense::Core::ByteSource:
    // Read() returns a genuinely async, non-blocking WinRT operation (backed by
    // I/O completion ports, not a blocked thread), rather than a synchronous call.
    class WinRtByteStream
    {
    public:
        explicit WinRtByteStream(winrt::Windows::Storage::Streams::IRandomAccessStream stream);

        winrt::Windows::Foundation::IAsyncOperation<uint32_t> Read(std::span<std::byte> buffer);
        uint64_t Size() const;

    private:
        winrt::Windows::Storage::Streams::IRandomAccessStream m_stream;
        winrt::Windows::Storage::Streams::DataReader m_reader;
    };
} // namespace winrt::Flense::implementation
