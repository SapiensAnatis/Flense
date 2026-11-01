export module Flense.Core.Shared:ByteStreamRange;

import :ByteStream;

import std;

export namespace Flense::Core
{
    /// <summary>
    /// Exposes a ByteStream as a single-pass range of chars, reading it through a fixed-size buffer so that the whole
    /// stream never has to be held in memory.
    /// </summary>
    /// <remarks>
    /// Iterators share the range's position, so advancing one advances them all. Don't read from the stream directly
    /// while iterating, because the range will already have buffered some of what follows.
    /// </remarks>
    class ByteStreamRange
    {
      public:
        class Iterator
        {
          public:
            using iterator_category = std::input_iterator_tag;
            using value_type = char;
            using difference_type = std::ptrdiff_t;
            using pointer = const char*;
            using reference = const char&;

            Iterator() = default;

            explicit Iterator(ByteStreamRange* range) : m_range(range)
            {
            }

            [[nodiscard]] reference operator*() const
            {
                return m_range->m_buffer.at(m_range->m_position);
            }

            Iterator& operator++()
            {
                m_range->Advance();
                return *this;
            }

            void operator++(int)
            {
                m_range->Advance();
            }

            [[nodiscard]] friend bool operator==(const Iterator& iterator, std::default_sentinel_t /* end */)
            {
                return iterator.AtEnd();
            }

          private:
            [[nodiscard]] bool AtEnd() const
            {
                return m_range->m_position == m_range->m_size;
            }

            ByteStreamRange* m_range = nullptr;
        };

        explicit ByteStreamRange(ByteStream& stream) : m_stream(stream), m_buffer(BufferSize)
        {
        }

        ByteStreamRange(const ByteStreamRange&) = delete;
        ByteStreamRange(ByteStreamRange&&) = delete;

        ~ByteStreamRange() = default;

        ByteStreamRange& operator=(const ByteStreamRange&) = delete;
        ByteStreamRange& operator=(ByteStreamRange&&) = delete;

        [[nodiscard]] Iterator begin() // NOLINT(readability-identifier-naming) - range interface
        {
            if (m_position == m_size)
            {
                Refill();
            }

            return Iterator(this);
        }

        // NOLINTNEXTLINE(readability-identifier-naming,readability-convert-member-functions-to-static) - range interface
        [[nodiscard]] std::default_sentinel_t end() const
        {
            return std::default_sentinel;
        }

      private:
        static constexpr std::size_t BufferSize = std::size_t{64} * 1024;

        void Advance()
        {
            ++m_position;
            if (m_position == m_size)
            {
                Refill();
            }
        }

        void Refill()
        {
            m_size = m_stream.ReadSync(std::as_writable_bytes(std::span(m_buffer)));
            m_position = 0;
        }

        ByteStream& m_stream;
        std::vector<char> m_buffer;
        std::size_t m_position = 0;
        std::size_t m_size = 0;
    };
} // namespace Flense::Core
