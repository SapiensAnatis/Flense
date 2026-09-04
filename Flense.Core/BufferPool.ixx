module;

#include "ThreadSafetyAttributes.h"

export module Flense.Core:BufferPool;

import std;
import :Mutex;

namespace Flense::Core
{
    class BufferPool
    {
      public:
        BufferPool(std::size_t bufferSize, std::size_t bufferCount)
        {
            if (bufferSize == 0 || bufferCount == 0)
            {
                throw std::invalid_argument("bufferSize and bufferCount must be non-zero and positive");
            }

            for (std::size_t i = 0; i < bufferCount; ++i)
            {
                m_buffers.emplace_back(bufferSize);
            }
        }

        [[nodiscard]] std::vector<std::byte> GetBuffer()
        {
            const MutexLocker locker(&m_mutex);

            m_bufferAvailable.wait(m_mutex, [this]() REQUIRES(m_mutex) { return !m_buffers.empty(); });

            std::vector<std::byte> buffer = std::move(m_buffers.back());
            m_buffers.pop_back();
            return buffer;
        }

        void ReturnBuffer(std::vector<std::byte>&& buffer)
        {
            const MutexLocker locker(&m_mutex);
            m_buffers.push_back(std::move(buffer));
            m_bufferAvailable.notify_one();
        }

      private:
        Mutex m_mutex;
        std::vector<std::vector<std::byte>> m_buffers GUARDED_BY(m_mutex);
        std::condition_variable_any m_bufferAvailable;
    };

    class RentedBuffer
    {
      public:
        RentedBuffer(const RentedBuffer&) = delete;

        RentedBuffer(RentedBuffer&& other) noexcept : m_pool(other.m_pool), m_buffer(std::exchange(other.m_buffer, {}))
        {
            other.m_pool = nullptr;
        }

        RentedBuffer& operator=(const RentedBuffer&) = delete;

        RentedBuffer& operator=(RentedBuffer&& other) noexcept
        {
            if (this != &other)
            {
                Release();
                m_pool = std::exchange(other.m_pool, nullptr);
                m_buffer = std::exchange(other.m_buffer, {});
            }
            return *this;
        }

        static RentedBuffer From(BufferPool* pool)
        {
            return {pool, pool->GetBuffer()};
        }

        [[nodiscard]] std::span<std::byte> Buffer() noexcept
        {
            return {m_buffer};
        }

        ~RentedBuffer()
        {
            Release();
        }

      private:
        void Release()
        {
            if (m_pool != nullptr)
            {
                try
                {
                    m_pool->ReturnBuffer(std::move(m_buffer));
                }
                catch (...) // NOLINT(bugprone-empty-catch)
                {
                    // Don't throw from a destructor.
                    // std::vector<std::byte> has a noexcept move constructor, so this will not corrupt the vector.
                }

                m_pool = nullptr;
            }
        }

        RentedBuffer(BufferPool* pool, std::vector<std::byte>&& buffer) : m_pool(pool), m_buffer(std::move(buffer))
        {
        }

        BufferPool* m_pool;
        std::vector<std::byte> m_buffer;
    };
} // namespace Flense::Core