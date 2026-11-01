export module Flense.Core.Tests.Support:MemoryByteStream;

import Flense.Core.Shared;
import std;

export namespace Flense::Core::Tests
{
    /// <summary>
    /// A ByteStream over a copy of some bytes held in memory, standing in for a connection to the Docker daemon.
    /// </summary>
    class MemoryByteStream : public ByteStream
    {
      public:
        /// <param name="contents">The bytes the stream will return.</param>
        /// <param name="maxReadSize">Caps how much each ReadSync returns, to imitate a connection that delivers data
        /// in pieces.</param>
        explicit MemoryByteStream(const std::string_view contents,
                                  const std::size_t maxReadSize = std::numeric_limits<std::size_t>::max())
            : m_contents(std::from_range, std::as_bytes(std::span(contents))), m_maxReadSize(maxReadSize)
        {
        }

        std::size_t ReadSync(const std::span<std::byte> buffer) override
        {
            const auto remaining = std::span(m_contents).subspan(m_position);
            const std::size_t count = std::min({buffer.size(), remaining.size(), m_maxReadSize});

            std::ranges::copy(remaining.first(count), buffer.begin());
            m_position += count;

            return count;
        }

      private:
        std::vector<std::byte> m_contents;
        std::size_t m_maxReadSize;
        std::size_t m_position{0};
    };
} // namespace Flense::Core::Tests
