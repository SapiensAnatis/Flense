export module Flense.Core.Tests.Support:FaultyByteStream;

import Flense.Core;
import std;

export namespace Flense::Core::Tests
{
    class InjectedReadError : public std::runtime_error
    {
      public:
        InjectedReadError() : std::runtime_error("injected read failure")
        {
        }
    };

    class InjectedSkipError : public std::runtime_error
    {
      public:
        InjectedSkipError() : std::runtime_error("injected skip failure")
        {
        }
    };

    /// <summary>
    /// Wraps another ByteStream, capping how much each read returns and throwing from ReadSync or Skip on demand.
    /// </summary>
    template <SkippableByteStream TInner> class FaultyByteStream
    {
      public:
        template <typename... TArgs>
        explicit FaultyByteStream(TArgs&&... args) : m_inner(std::forward<TArgs>(args)...)
        {
        }

        /// <summary>Caps how much each ReadSync returns. ArchiveReader asks for far more than the small test archives
        /// hold, so without a cap the first read would consume the whole file and later reads and skips would never
        /// reach the stream.</summary>
        static constexpr std::size_t MaxReadSize = 512;

        /// <summary>ReadSync throws InjectedReadError once Position() reaches this.</summary>
        std::optional<std::uint64_t> failReadAt;

        /// <summary>Skip throws InjectedSkipError while this is set.</summary>
        bool failSkip{false};

        /// <summary>How many times Skip has been called, whether or not it threw.</summary>
        int skipCalls{0};

        std::size_t ReadSync(const std::span<std::byte> buffer)
        {
            if (failReadAt.has_value() && Position() >= *failReadAt)
            {
                throw InjectedReadError{};
            }

            return m_inner.ReadSync(buffer.first(std::min(buffer.size(), MaxReadSize)));
        }

        std::int64_t Skip(const std::int64_t request)
        {
            ++skipCalls;

            if (failSkip)
            {
                throw InjectedSkipError{};
            }

            return m_inner.Skip(request);
        }

        [[nodiscard]] std::uint64_t Size() const
        {
            return m_inner.Size();
        }

        [[nodiscard]] std::uint64_t Position() const
        {
            return m_inner.Position();
        }

      private:
        TInner m_inner;
    };

    static_assert(SkippableByteStream<FaultyByteStream<FileByteStream>>);
} // namespace Flense::Core::Tests
