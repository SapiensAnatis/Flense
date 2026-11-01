export module Flense.Core.Shared:ByteStream;

import std;

export namespace Flense::Core
{
    /// <summary>
    /// A source of bytes that can be read sequentially, such as a file or a connection to the Docker daemon.
    /// </summary>
    /// <remarks>
    /// All operations are synchronous and blocking by design. libarchive drives reading via plain C callbacks that
    /// block the calling thread until they return, so there is no point in the stream itself being async. Callers are
    /// expected to read from a background thread.
    /// </remarks>
    class ByteStream
    {
      public:
        virtual ~ByteStream() = default;

        /// <summary>
        /// Reads up to buffer.size() bytes from the current position.
        /// </summary>
        /// <returns>The number of bytes read, which is 0 once the stream is exhausted.</returns>
        virtual std::size_t ReadSync(std::span<std::byte> buffer) = 0;

        /// <summary>
        /// Advances by up to request bytes without reading them.
        /// </summary>
        /// <returns>The number of bytes skipped. Streams that cannot skip return 0, which tells the caller to read
        /// and discard the bytes instead.</returns>
        virtual std::int64_t Skip(std::int64_t /* request */)
        {
            return 0;
        }

      protected:
        ByteStream() = default;
        ByteStream(const ByteStream&) = default;
        ByteStream(ByteStream&&) = default;
        ByteStream& operator=(const ByteStream&) = default;
        ByteStream& operator=(ByteStream&&) = default;
    };
} // namespace Flense::Core
