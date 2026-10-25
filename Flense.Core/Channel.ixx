module;

#include "ThreadSafetyAttributes.h"

export module Flense.Core:Internal.Channel;

import std;
import :Internal.Mutex;

namespace Flense::Core
{
    /// <summary>
    /// Unbounded channel for communication between threads.
    /// </summary>
    /// <typeparam name="T">The type of object to be passed between threads.</typeparam>
    template <typename T> class Channel
    {
      public:
        class Sender
        {
          public:
            explicit Sender(std::shared_ptr<Channel> channel) : m_channel(std::move(channel))
            {
            }

            Sender(const Sender& other) = delete;
            Sender(Sender&& other) noexcept = default;

            Sender& operator=(const Sender& other) = delete;
            Sender& operator=(Sender&& other) = delete; // Could be done, but not currently needed

            [[nodiscard]] bool Push(T&& value)
            {
                return m_channel->Push(std::move(value));
            }

            ~Sender()
            {
                const bool movedFrom = m_channel == nullptr;

                if (!movedFrom)
                {
                    m_channel->Close();
                }
            }

          private:
            std::shared_ptr<Channel> m_channel;
        };

        class Receiver
        {
          public:
            explicit Receiver(std::shared_ptr<Channel> channel) : m_channel(std::move(channel))
            {
            }

            Receiver(const Receiver& other) = delete;
            Receiver(Receiver&& other) noexcept = default;

            Receiver& operator=(const Receiver& other) = delete;
            Receiver& operator=(Receiver&& other) = delete; // Could be done, but not currently needed

            [[nodiscard]] std::optional<T> Pop()
            {
                return m_channel->Pop();
            }

            ~Receiver()
            {
                const bool movedFrom = m_channel == nullptr;

                if (!movedFrom)
                {
                    m_channel->Close();

                    // If the receiver is dead, the items in the channel have no destination anymore and should be
                    // destroyed.
                    m_channel->Drain();
                }
            }

          private:
            std::shared_ptr<Channel> m_channel;
        };

        /// <summary>
        /// Pushes an item into the channel. Never blocks.
        /// </summary>
        /// <param name="value">The value to push.</param>
        [[nodiscard]] bool Push(T&& value)
        {
            {
                const MutexLocker locker(&m_mutex);

                if (m_closed)
                {
                    return false;
                }

                m_queue.push(std::move(value));
            }

            m_itemPushed.notify_one();
            return true;
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
        /// Clear the channel.
        /// </summary>
        void Drain()
        {
            std::queue<T> discarded;

            {
                const MutexLocker locker(&m_mutex);
                std::swap(m_queue, discarded);
            }
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

        static std::pair<Sender, Receiver> Create()
        {
            auto channel = std::shared_ptr<Channel>(new Channel());

            return std::pair{Sender(channel), Receiver(channel)};
        }

      private:
        Channel() = default;

        Mutex m_mutex;
        std::condition_variable_any m_itemPushed;

        std::queue<T> m_queue GUARDED_BY(m_mutex);
        bool m_closed GUARDED_BY(m_mutex){false};
    };
} // namespace Flense::Core
