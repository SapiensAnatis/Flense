#pragma once

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace winrt::Flense::implementation
{
    // Wraps a WinRT random-access stream so it satisfies Flense::Core::LibArchiveSource.
    // All methods block the calling thread (via .get() on the underlying WinRT async
    // operations), so instances must only be driven from a background thread - never
    // from a UI/STA thread.
    class WinRtByteStream
    {
    public:
        explicit WinRtByteStream(winrt::Windows::Storage::Streams::IRandomAccessStream stream);

        size_t ReadSync(std::span<std::byte> buffer);
        int64_t Skip(int64_t request);
        uint64_t Size() const;
        uint64_t Position() const;

    private:
        winrt::Windows::Storage::Streams::IRandomAccessStream m_stream;
        winrt::Windows::Storage::Streams::DataReader m_reader;
        uint64_t m_position{0};
    };
} // namespace winrt::Flense::implementation
