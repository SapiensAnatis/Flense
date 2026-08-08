#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace Flense::Core
{
    /// <summary>
    /// A fixed-size pool of worker threads for running independent, self-contained pieces of work.
    /// </summary>
    /// <remarks>
    /// Deliberately not a general-purpose executor: there is no task priority, no work stealing, and
    /// no way to wait on an individual task - only WaitForIdle(), which waits for all of them. That is
    /// all ImageParser needs, and anything more would be untested weight.
    ///
    /// The queue is unbounded. Backpressure, where it matters, belongs to the caller: it is the memory
    /// a queued task holds onto that needs limiting, not the number of tasks.
    /// </remarks>
    class ThreadPool
    {
      public:
        /// <summary>
        /// Constructs a new instance of the ThreadPool class and starts its workers.
        /// </summary>
        /// <param name="workerCount">The number of worker threads, or 0 for one per hardware thread.</param>
        explicit ThreadPool(size_t workerCount);

        /// <summary>
        /// Signals the workers to finish the tasks already queued, then joins them.
        /// </summary>
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;

        /// <summary>
        /// Queues a piece of work to run on some worker thread.
        /// </summary>
        /// <param name="work">The work to run. Exceptions escaping it are swallowed, so it should do
        /// its own error handling if failures need to be observable.</param>
        void Submit(std::move_only_function<void()> work);

        /// <summary>
        /// Blocks until every submitted task has finished and the queue is empty.
        /// </summary>
        /// <remarks>
        /// Only meaningful when no other thread is submitting concurrently - it observes the pool being
        /// momentarily idle, not that no more work will ever arrive.
        /// </remarks>
        void WaitForIdle();

        /// <summary>
        /// The number of worker threads in the pool.
        /// </summary>
        size_t WorkerCount() const
        {
            return m_workers.size();
        }

      private:
        void RunWorker();

        std::mutex m_mutex;
        std::condition_variable m_workAvailable;
        std::condition_variable m_idle;
        std::deque<std::move_only_function<void()>> m_queue;
        size_t m_activeCount{0};
        bool m_stopping{false};

        // Declared last so the workers are joined before the state they touch is destroyed.
        std::vector<std::jthread> m_workers;
    };

} // namespace Flense::Core
