module;

#include "ThreadSafetyAttributes.h"

export module Flense.Core:Channel;

import std;
import :Mutex;

namespace Flense::Core
{
    /// <summary>
    /// Unbounded channel for communication between threads.
    /// </summary>
    /// <typeparam name="T">The type of object to be passed between threads.</typeparam>
    template <typename T> class Channel
    {
      public:
        /// <summary>
        /// Pushes an item into the channel. Never blocks.
        /// </summary>
        /// <param name="value">The value to push.</param>
        void Push(T&& value)
        {
            const MutexLocker locker(&m_mutex);

            if (m_closed)
            {
                return;
            }

            m_queue.push(std::move(value));
            m_itemPushed.notify_one();
        }

        /// <summary>
        /// Pop an item off of the channel. If empty, blocks until the channel either has an item available (in which
        /// case the item is returned) or closed (in which case nullopt is returned).
        /// </summary>
        /// <returns>The popped value, or nullopt if the channel is closed and drained.</returns>
        [[nodiscard]] std::optional<T> Pop()
        {
            const MutexLocker locker(&m_mutex);

            m_itemPushed.wait(m_mutex, [this]() REQUIRES(m_mutex) { return m_closed || !m_queue.empty(); });

            if (m_queue.empty())
            {
                return std::nullopt;
            }

            std::optional<T> value(std::move(m_queue.front()));
            m_queue.pop();
            return value;
        }

        /// <summary>
        /// Close the channel, preventing any further pushes.
        /// </summary>
        void Close()
        {
            const MutexLocker locker(&m_mutex);

            if (!m_closed)
            {
                m_closed = true;
                m_itemPushed.notify_all();
            }
        }

      private:
        Mutex m_mutex;
        std::condition_variable_any m_itemPushed;

        std::queue<T> m_queue GUARDED_BY(m_mutex);
        bool m_closed GUARDED_BY(m_mutex){false};
    };
} // namespace Flense::Core
