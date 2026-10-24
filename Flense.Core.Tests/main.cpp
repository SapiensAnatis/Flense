#include <gtest/gtest.h>

import Flense.Core.Tests.Support;
import std;

namespace
{
    /// <summary>
    /// Fails the run if any single test takes longer than a deadline, so that hangs are reported rather than blocking
    /// forever.
    /// </summary>
    /// <remarks>
    /// A hung test is usually stuck inside libarchive, where it can't be interrupted or unwound, so on expiry this
    /// reports the test and terminates the whole process instead of trying to carry on with the rest.
    /// </remarks>
    class TestWatchdog : public testing::EmptyTestEventListener
    {
      public:
        explicit TestWatchdog(const std::chrono::seconds timeout) : m_timeout(timeout)
        {
            m_thread = std::jthread{[this](const std::stop_token stopToken) { Run(stopToken); }};
        }

        void OnTestStart(const testing::TestInfo& testInfo) override
        {
            {
                const std::scoped_lock lock{m_mutex};
                m_testName = std::format("{}.{}", testInfo.test_suite_name(), testInfo.name());
                m_deadline = std::chrono::steady_clock::now() + m_timeout;
            }

            m_changed.notify_all();
        }

        void OnTestEnd(const testing::TestInfo& /* unused */) override
        {
            {
                const std::scoped_lock lock{m_mutex};
                m_deadline.reset();
            }

            m_changed.notify_all();
        }

      private:
        void Run(const std::stop_token stopToken)
        {
            std::unique_lock lock{m_mutex};

            while (!stopToken.stop_requested())
            {
                if (!m_deadline.has_value())
                {
                    m_changed.wait(lock, stopToken, [this] { return m_deadline.has_value(); });
                    continue;
                }

                const auto deadline = *m_deadline;
                const bool changed =
                    m_changed.wait_until(lock, stopToken, deadline, [&] { return m_deadline != deadline; });

                if (!changed && !stopToken.stop_requested())
                {
                    std::println(stderr, "[  TIMEOUT ] {} did not finish within {}", m_testName, m_timeout);
                    std::fflush(stderr);
                    std::fflush(stdout);
                    std::_Exit(1);
                }
            }
        }

        std::chrono::seconds m_timeout;
        std::mutex m_mutex;
        std::condition_variable_any m_changed;
        std::optional<std::chrono::steady_clock::time_point> m_deadline;
        std::string m_testName;

        // Declared last so it is destroyed first, joining the thread before the state it uses goes away.
        std::jthread m_thread;
    };
} // namespace

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);

    const std::filesystem::path testData = Flense::Core::Tests::TestDataPath("ErrorCases/valid.tar");
    if (!std::filesystem::exists(testData))
    {
        std::println(stderr,
                     "Test data not found at '{}'; run TestData/GenerateErrorCases.ps1 (PowerShell 7), then rebuild to "
                     "copy it next to the executable.",
                     testData.string());
        return 1;
    }

    // gtest takes ownership of appended listeners.
    testing::UnitTest::GetInstance()->listeners().Append(new TestWatchdog{std::chrono::seconds{60}});

    return RUN_ALL_TESTS();
}
