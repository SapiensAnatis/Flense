#pragma once

#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <stop_token>

namespace Flense::Core
{
    template <typename T>
    concept ByteSource = requires(T& source, std::span<std::byte> buffer) {
        { source.Read(buffer) };
        { source.Size() } -> std::convertible_to<uint64_t>;
    };

    /**
     * Processes an archive from a byte source, reporting progress and checking for cancellation.
     * @tparam Awaitable The type of the awaitable that the asynchronous operation should be returned as.
     * @tparam TSource The type of the byte source.
     * @param source The byte source to process.
     * @param onProgress The function to call with progress updates.
     * @param stopToken The token to check for cancellation.
     * @return An awaitable representing the asynchronous operation.
     */
    template <typename Awaitable, ByteSource TSource>
    Awaitable ProcessArchive(TSource& source, std::function<void(double)> onProgress, std::stop_token stopToken)
    {
        constexpr size_t chunkSize = 64 * 1024;
        auto const buffer = std::make_unique<std::byte[]>(chunkSize);
        std::span<std::byte> const bufferSpan(buffer.get(), chunkSize);

        uint64_t const totalSize = source.Size();
        uint64_t bytesRead = 0;
        double lastReportedPercent = -1.0;

        while (!stopToken.stop_requested())
        {
            size_t const bytesReadThisChunk = co_await source.Read(bufferSpan);
            if (bytesReadThisChunk == 0)
            {
                break;
            }

            // TODO: replace with real processing of buffer[0, bytesReadThisChunk)
            bytesRead += bytesReadThisChunk;

            if (totalSize > 0)
            {
                // Only report on meaningful (>=5%) changes - onProgress typically
                // marshals to a UI thread, and posting on every chunk (there can
                // be thousands for a large file) can flood it badly enough to
                // look and behave like a hang.
                double const percent = static_cast<double>(bytesRead) / static_cast<double>(totalSize) * 100.0;
                if (percent - lastReportedPercent >= 5.0)
                {
                    lastReportedPercent = percent;
                    onProgress(percent);
                }
            }
        }
    }

} // namespace Flense::Core
