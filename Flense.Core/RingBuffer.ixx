module;

#include "ThreadSafetyAttributes.h"

export module Flense.Core:RingBuffer;

import std;
import :Mutex;
import :ArchiveReader;

namespace Flense::Core
{
    class RingBuffer
    {
      public:
        explicit RingBuffer(size_t capacity);

        [[nodiscard]] std::size_t Write(std::span<const std::byte> source);
        [[nodiscard]] std::size_t Read(std::span<std::byte> target);
        void Close();

      private:
        Mutex m_mutex;

        [[nodiscard]] std::size_t FreeBytes() const REQUIRES(m_mutex);

        std::size_t m_readIndex GUARDED_BY(m_mutex){0};
        std::size_t m_writeIndex GUARDED_BY(m_mutex){0};
        std::size_t m_size GUARDED_BY(m_mutex){0};
        bool m_closed GUARDED_BY(m_mutex){false};

        std::condition_variable_any m_readMoved;
        std::condition_variable_any m_writeMoved;

        std::vector<std::byte> m_buffer GUARDED_BY(m_mutex);
    };

    class RingBufferStream
    {
      public:
        explicit RingBufferStream(RingBuffer* buffer, std::int64_t totalDataSize);

        std::size_t ReadSync(std::span<std::byte> target);
        [[nodiscard]] std::int64_t Size() const;
        [[nodiscard]] std::int64_t Position() const;

      private:
        RingBuffer* m_buffer{nullptr};
        std::int64_t m_totalDataSize{0};
        std::int64_t m_position{0};
    };

    static_assert(ByteStream<RingBufferStream>);

} // namespace Flense::Core
