#include "pch.h"

#include "ThreadPool.h"

#include <algorithm>
#include <cstddef>
#include <mutex>
#include <thread>
#include <utility>

namespace Flense::Core
{
    ThreadPool::ThreadPool(const size_t workerCount)
    {
        const size_t count = workerCount > 0 ? workerCount : std::max<size_t>(1, std::thread::hardware_concurrency());

        m_workers.reserve(count);
        for (size_t i = 0; i < count; i += 1)
        {
            m_workers.emplace_back([this] { RunWorker(); });
        }
    }

    ThreadPool::~ThreadPool()
    {
        {
            const std::scoped_lock lock{m_mutex};
            m_stopping = true;
        }

        m_workAvailable.notify_all();

        // std::jthread joins on destruction. Workers drain the queue before exiting, so any task
        // submitted before this point still runs.
    }

    void ThreadPool::Submit(std::move_only_function<void()> work)
    {
        {
            const std::scoped_lock lock{m_mutex};
            m_queue.push_back(std::move(work));
        }

        m_workAvailable.notify_one();
    }

    void ThreadPool::WaitForIdle()
    {
        std::unique_lock lock{m_mutex};
        m_idle.wait(lock, [this] { return m_queue.empty() && m_activeCount == 0; });
    }

    void ThreadPool::RunWorker()
    {
        while (true)
        {
            std::move_only_function<void()> work;

            {
                std::unique_lock lock{m_mutex};
                m_workAvailable.wait(lock, [this] { return m_stopping || !m_queue.empty(); });

                if (m_queue.empty())
                {
                    // Only reachable when stopping - the queue is drained before a worker gives up.
                    return;
                }

                work = std::move(m_queue.front());
                m_queue.pop_front();
                m_activeCount += 1;
            }

            try
            {
                work();
            }
            catch (...)
            {
                // A task that throws must not take the worker down with it. Tasks that care about
                // their own failures are expected to catch and record them themselves.
            }

            {
                const std::scoped_lock lock{m_mutex};
                m_activeCount -= 1;
            }

            m_idle.notify_all();
        }
    }
} // namespace Flense::Core
