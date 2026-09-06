#include "pch.h"

#include "FileByteStream.h"

#include <malloc.h>

#include <algorithm>
#include <format>
#include <stdexcept>

import Flense.Core;

namespace Flense::Benchmarks
{
    namespace
    {
        constexpr size_t DefaultBufferCapacity = 1024 * 1024;

        [[noreturn]] void ThrowLastError(const std::filesystem::path& path, const char* operation)
        {
            throw std::runtime_error(
                std::format("{} failed for '{}' (GetLastError = {})", operation, path.string(), GetLastError()));
        }

        /// <summary>
        /// Rounds a file offset down to the nearest multiple of alignment.
        /// </summary>
        uint64_t AlignDown(const uint64_t value, const size_t alignment)
        {
            return value - (value % alignment);
        }

        /// <summary>
        /// The alignment unbuffered reads on this handle should use.
        /// </summary>
        /// <remarks>
        /// The logical sector size is only the OS's minimum for FILE_FLAG_NO_BUFFERING to accept a
        /// request at all. On a 512e/4Kn drive that emulates a smaller logical sector over a larger
        /// physical one, aligning to just the logical size still lets a read straddle two physical
        /// sectors, so the drive ends up servicing it as more than the single operation we're trying
        /// to measure. Aligning to the larger of the two avoids that while still satisfying the OS's
        /// requirement.
        /// </remarks>
        /// <exception cref="std::runtime_error">The handle's storage info could not be queried.</exception>
        size_t QuerySectorSize(void* handle, const std::filesystem::path& path)
        {
            FILE_STORAGE_INFO info{};
            if (!GetFileInformationByHandleEx(handle, FileStorageInfo, &info, sizeof(info)) ||
                info.LogicalBytesPerSector == 0)
            {
                ThrowLastError(path, "GetFileInformationByHandleEx");
            }

            if (info.PhysicalBytesPerSectorForPerformance == 0)
            {
                throw std::runtime_error(std::format(
                    "GetFileInformationByHandleEx returned zero PhysicalBytesPerSectorForPerformance for '{}'",
                    path.string()));
            }

            return info.PhysicalBytesPerSectorForPerformance;
        }
    } // namespace

    FileByteStream::FileByteStream(const std::filesystem::path& path) : m_path(path)
    {
        // FILE_FLAG_NO_BUFFERING bypasses the OS page cache entirely - every read reaches disk, which
        // is what makes benchmark runs comparable to a genuine cold start instead of to whichever
        // earlier pass happened to leave the file cached.
        void* const handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, nullptr);

        if (handle == INVALID_HANDLE_VALUE)
        {
            ThrowLastError(path, "CreateFileW");
        }

        // Only handed to the unique_ptr once known valid - INVALID_HANDLE_VALUE is not nullptr, so it
        // would otherwise look like a real, closable handle to it.
        m_handle.reset(handle);

        LARGE_INTEGER size{};
        if (!GetFileSizeEx(m_handle.get(), &size))
        {
            ThrowLastError(path, "GetFileSizeEx");
        }

        m_size = static_cast<uint64_t>(size.QuadPart);

        m_sectorSize = QuerySectorSize(m_handle.get(), path);
        m_bufferCapacity = std::max(m_sectorSize, AlignDown(DefaultBufferCapacity, m_sectorSize));

        m_alignedBuffer.reset(static_cast<std::byte*>(_aligned_malloc(m_bufferCapacity, m_sectorSize)));
        if (!m_alignedBuffer)
        {
            throw std::bad_alloc();
        }
    }

    FileByteStream::~FileByteStream() = default;

    void FileByteStream::HandleDeleter::operator()(void* handle) const noexcept
    {
        CloseHandle(handle);
    }

    void FileByteStream::AlignedBufferDeleter::operator()(std::byte* buffer) const noexcept
    {
        _aligned_free(buffer);
    }

    bool FileByteStream::FillBuffer()
    {
        if (m_position >= m_size)
        {
            m_bufferValidLength = 0;
            return false;
        }

        const uint64_t physicalStart = AlignDown(m_position, m_sectorSize);

        LARGE_INTEGER distance{};
        distance.QuadPart = static_cast<int64_t>(physicalStart);
        if (!SetFilePointerEx(m_handle.get(), distance, nullptr, FILE_BEGIN))
        {
            ThrowLastError(m_path, "SetFilePointerEx");
        }

        DWORD bytesRead = 0;
        if (!ReadFile(m_handle.get(), m_alignedBuffer.get(), static_cast<DWORD>(m_bufferCapacity), &bytesRead, nullptr))
        {
            ThrowLastError(m_path, "ReadFile");
        }

        // A request that runs past the real end of file is allowed - it just reads however much of
        // the sector-aligned tail actually exists. Trust the file's known size over whatever ReadFile
        // reports for that trailing, partly-nonexistent sector.
        const uint64_t realBytesAvailable = m_size - physicalStart;
        m_bufferValidLength = static_cast<size_t>(std::min<uint64_t>(bytesRead, realBytesAvailable));
        m_bufferStart = physicalStart;

        return m_position < m_bufferStart + m_bufferValidLength;
    }

    size_t FileByteStream::ReadSync(const std::span<std::byte> buffer)
    {
        size_t totalRead = 0;

        while (totalRead < buffer.size())
        {
            const bool withinBuffer = m_position >= m_bufferStart && m_position < m_bufferStart + m_bufferValidLength;
            if (!withinBuffer && !FillBuffer())
            {
                break;
            }

            const size_t bufferOffset = static_cast<size_t>(m_position - m_bufferStart);
            const size_t available = m_bufferValidLength - bufferOffset;
            const size_t toCopy = std::min(available, buffer.size() - totalRead);

            const std::span<const std::byte> sourceSpan{m_alignedBuffer.get(), m_bufferCapacity};
            std::ranges::copy(sourceSpan.subspan(bufferOffset, toCopy), buffer.subspan(totalRead).begin());

            totalRead += toCopy;
            m_position += toCopy;
        }

        return totalRead;
    }

    int64_t FileByteStream::Skip(const int64_t request)
    {
        if (request <= 0)
        {
            return 0;
        }

        const uint64_t remaining = m_size - m_position;
        const int64_t toSkip = static_cast<int64_t>(std::min<uint64_t>(static_cast<uint64_t>(request), remaining));

        // Just advance the logical position - ReadSync notices it has fallen outside the aligned
        // buffer and refills from the right place, so there is nothing to do with the buffer here.
        m_position += static_cast<uint64_t>(toSkip);
        return toSkip;
    }

    uint64_t FileByteStream::Size() const
    {
        return m_size;
    }

    uint64_t FileByteStream::Position() const
    {
        return m_position;
    }

    static_assert(Core::ByteStream<FileByteStream>);
} // namespace Flense::Benchmarks
