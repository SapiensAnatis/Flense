#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <stop_token>

namespace Flense::Core
{
    /// <summary>
    /// A counting limit on how many bytes of buffered data may be alive at once, used to apply
    /// backpressure to a producer that would otherwise read ahead faster than consumers can keep up.
    /// </summary>
    class ByteBudget
    {
      public:
        /// <summary>
        /// Constructs a new instance of the ByteBudget class.
        /// </summary>
        /// <param name="capacity">The number of bytes that may be outstanding at once.</param>
        /// <param name="stopToken">Token that releases any thread blocked in Acquire.</param>
        ByteBudget(const uint64_t capacity, std::stop_token stopToken)
            : m_capacity(capacity), m_stopToken(std::move(stopToken))
        {
        }

        /// <summary>
        /// Blocks until the given number of bytes can be admitted, then charges them to the budget.
        /// </summary>
        /// <remarks>
        /// Always admits immediately when nothing is outstanding, even if the request is larger than
        /// the whole capacity - otherwise a single oversized item could never make progress. Returns
        /// early without charging anything if cancellation is requested.
        /// </remarks>
        /// <param name="bytes">The number of bytes about to be buffered.</param>
        void Acquire(const uint64_t bytes)
        {
            std::unique_lock lock{m_mutex};

            m_freed.wait(lock, m_stopToken,
                         [this, bytes] { return m_outstanding == 0 || m_outstanding + bytes <= m_capacity; });

            m_outstanding += bytes;
        }

        /// <summary>
        /// Returns bytes to the budget, waking whoever is waiting on it.
        /// </summary>
        /// <param name="bytes">The number of bytes no longer held.</param>
        void Release(const uint64_t bytes)
        {
            {
                const std::scoped_lock lock{m_mutex};
                m_outstanding -= bytes;
            }

            m_freed.notify_all();
        }

      private:
        std::mutex m_mutex;
        std::condition_variable_any m_freed;
        uint64_t m_capacity;
        uint64_t m_outstanding{0};
        std::stop_token m_stopToken;
    };

} // namespace Flense::Core
