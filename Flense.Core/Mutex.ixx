module;

#include "ThreadSafetyAttributes.h"

export module Flense.Core:Mutex;

import std;

export namespace Flense::Core
{
    // Defines an annotated interface for mutexes.
    // Implemented using std::mutex.
    class CAPABILITY("mutex") Mutex
    {
      public:
        // Acquire/lock this mutex exclusively.  Only one thread can have exclusive
        // access at any one time.  Write operations to guarded data require an
        // exclusive lock.
        void Lock() ACQUIRE()
        {
            m_mutex.lock();
            m_owner.store(std::this_thread::get_id());
        }

        // Acquire/lock this mutex for read operations, which require only a shared
        // lock.  This assumes a multiple-reader, single writer semantics.  Multiple
        // threads may acquire the mutex simultaneously as readers, but a writer
        // must wait for all of them to release the mutex before it can acquire it
        // exclusively.
        void ReaderLock() ACQUIRE_SHARED() = delete;

        // Release/unlock an exclusive mutex.
        void Unlock() RELEASE()
        {
            m_mutex.unlock();
            m_owner.store(std::thread::id{});
        }

        void lock() ACQUIRE() // NOLINT(readability-identifier-naming) - std::condition_variable_any shim
        {
            Lock();
        }

        void unlock() RELEASE() // NOLINT(readability-identifier-naming) - std::condition_variable_any shim
        {
            Unlock();
        }

        // Release/unlock a shared mutex.
        void ReaderUnlock() RELEASE_SHARED() = delete;

        // Generic unlock, can unlock exclusive and shared mutexes.
        void GenericUnlock() RELEASE_GENERIC() = delete;

        // Try to acquire the mutex.  Returns true on success, and false on failure.
        bool TryLock() TRY_ACQUIRE(true)
        {
            if (m_mutex.try_lock())
            {
                m_owner.store(std::this_thread::get_id());
                return true;
            }

            return false;
        }

        // Try to acquire the mutex for read operations.
        bool ReaderTryLock() TRY_ACQUIRE_SHARED(true) = delete;

        // Assert that this mutex is currently held by the calling thread.
        void AssertHeld() ASSERT_CAPABILITY(this)
        {
            if (m_owner != std::this_thread::get_id())
            {
                std::abort();
            }
        }

        // Assert that is mutex is currently held for read operations.
        void AssertReaderHeld() ASSERT_SHARED_CAPABILITY(this) = delete;

        // For negative capabilities.
        const Mutex& operator!() const
        {
            return *this;
        }

      private:
        std::mutex m_mutex;
        std::atomic<std::thread::id> m_owner;

        static_assert(std::atomic<std::thread::id>::is_always_lock_free);
    };

    // MutexLocker is an RAII class that acquires a mutex in its constructor, and
    // releases it in its destructor.
    class SCOPED_CAPABILITY MutexLocker
    {
      private:
        Mutex* mut;
        bool locked;

      public:
        MutexLocker(const MutexLocker& other) = delete;
        MutexLocker(MutexLocker&& other) = delete;

        MutexLocker& operator=(const MutexLocker& other) = delete;
        MutexLocker& operator=(MutexLocker&& other) = delete;

        // Acquire mu, implicitly acquire *this and associate it with mu.
        MutexLocker(Mutex* mu) ACQUIRE(mu) : mut(mu), locked(true)
        {
            mu->Lock();
        }

        // Assume mu is held, implicitly acquire *this and associate it with mu.
        MutexLocker(Mutex* mu, std::adopt_lock_t /* unused */) REQUIRES(mu) : mut(mu), locked(true)
        {
        }

        // Assume mu is not held, implicitly acquire *this and associate it with mu.
        MutexLocker(Mutex* mu, std::defer_lock_t /* unused */) EXCLUDES(mu) : mut(mu), locked(false)
        {
        }

        // Same as constructors, but without tag types. (Requires C++17 copy elision.)
        static MutexLocker Lock(Mutex* mu) ACQUIRE(mu)
        {
            return {mu};
        }

        static MutexLocker Adopt(Mutex* mu) REQUIRES(mu)
        {
            return {mu, std::adopt_lock};
        }

        static MutexLocker DeferLock(Mutex* mu) EXCLUDES(mu)
        {
            return {mu, std::defer_lock};
        }

        // Release *this and all associated mutexes, if they are still held.
        // There is no warning if the scope was already unlocked before.
        ~MutexLocker() RELEASE()
        {
            if (locked)
            {
                mut->Unlock();
            }
        }

        // Acquire all associated mutexes exclusively.
        void Lock() ACQUIRE()
        {
            mut->Lock();
            locked = true;
        }

        // Try to acquire all associated mutexes exclusively.
        bool TryLock() TRY_ACQUIRE(true)
        {
            return locked = mut->TryLock();
        }

        // Release all associated mutexes. Warn on double unlock.
        void Unlock() RELEASE()
        {
            mut->Unlock();
            locked = false;
        }
    };
} // namespace Flense::Core