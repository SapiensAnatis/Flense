export module Flense.Core.Tests.Support:MemoryByteStream;

import Flense.Core;
import std;

export namespace Flense::Core::Tests
{
    /// <summary>
    /// A ByteStream over a file read fully into memory.
    /// </summary>
    class MemoryByteStream
    {
      public:
        /// <param name="path">Path to the file to load.</param>
        /// <param name="maxReadSize">Caps how much each ReadSync returns. ArchiveReader asks for far more than the
        /// small test archives hold, so without a cap the first read would consume the whole file and later reads
        /// and skips would never reach the stream.</param>
        explicit MemoryByteStream(const std::filesystem::path& path, const std::size_t maxReadSize = 512)
            : m_maxReadSize(maxReadSize)
        {
            std::ifstream file{path, std::ios::binary};
            if (!file)
            {
                throw std::runtime_error(std::format("could not open '{}'", path.string()));
            }

            const std::vector<char> contents{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
            m_data.resize(contents.size());
            std::ranges::transform(contents, m_data.begin(), [](const char c) { return static_cast<std::byte>(c); });
        }

        std::size_t ReadSync(const std::span<std::byte> buffer)
        {
            const std::size_t toCopy = std::min({buffer.size(), m_maxReadSize, m_data.size() - m_position});
            std::ranges::copy(std::span{m_data}.subspan(m_position, toCopy), buffer.begin());
            m_position += toCopy;
            return toCopy;
        }

        std::int64_t Skip(const std::int64_t request)
        {
            if (request <= 0)
            {
                return 0;
            }

            const std::size_t toSkip = std::min(static_cast<std::size_t>(request), m_data.size() - m_position);
            m_position += toSkip;
            return static_cast<std::int64_t>(toSkip);
        }

        [[nodiscard]] std::uint64_t Size() const
        {
            return m_data.size();
        }

        [[nodiscard]] std::uint64_t Position() const
        {
            return m_position;
        }

        [[nodiscard]] std::size_t MaxReadSize() const
        {
            return m_maxReadSize;
        }

      private:
        std::vector<std::byte> m_data;
        std::size_t m_position{0};
        std::size_t m_maxReadSize;
    };

    static_assert(SkippableByteStream<MemoryByteStream>);
} // namespace Flense::Core::Tests
