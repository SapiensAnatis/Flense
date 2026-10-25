module;

#include "ThreadSafetyAttributes.h"

#if defined(__INTELLISENSE__)
#include <chrono>
#endif

module Flense.Core:Internal.BufferPool.Impl;

import :Internal.BufferPool;
import :Internal.Mutex;
import :Internal.Platform;

import std;

namespace Flense::Core
{
    std::vector<std::byte> BufferPool::GetBuffer()
    {
#if defined(_DEBUG)
        const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
#endif

        const MutexLocker locker(&m_mutex);

        m_bufferAvailable.wait(m_mutex, [this]() REQUIRES(m_mutex) { return !m_buffers.empty(); });

#if defined(_DEBUG)
        const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        DebugWriteLine(std::format("BufferPool::GetBuffer waited for {}",
                                   std::chrono::duration_cast<std::chrono::milliseconds>(end - start)));
#endif

        std::vector<std::byte> buffer = std::move(m_buffers.back());
        m_buffers.pop_back();
        return buffer;
    }
} // namespace Flense::Core
