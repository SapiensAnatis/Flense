module;

#include "ThreadSafetyAttributes.h"

module Flense.Core;

import :Mutex;

import std;

namespace Flense::Core
{
    void Mutex::Lock()
    {
        m_mutex.lock();
        m_owner.store(std::this_thread::get_id());
    }

    void Mutex::Unlock()
    {
        m_mutex.unlock();
        m_owner.store(std::thread::id{});
    }

    void Mutex::lock() // NOLINT(readability-identifier-naming) - std::condition_variable_any shim
    {
        Lock();
    }

    void Mutex::unlock() // NOLINT(readability-identifier-naming) - std::condition_variable_any shim
    {
        Unlock();
    }

    bool Mutex::TryLock()
    {
        if (m_mutex.try_lock())
        {
            m_owner.store(std::this_thread::get_id());
            return true;
        }

        return false;
    }

    void Mutex::AssertHeld()
    {
        if (m_owner != std::this_thread::get_id())
        {
            std::abort();
        }
    }

    const Mutex& Mutex::operator!() const
    {
        return *this;
    }

    MutexLocker::MutexLocker(Mutex* mu) : mut(mu), locked(true)
    {
        mu->Lock();
    }

    MutexLocker::MutexLocker(Mutex* mu, std::adopt_lock_t /* unused */) : mut(mu), locked(true)
    {
    }

    MutexLocker::MutexLocker(Mutex* mu, std::defer_lock_t /* unused */) : mut(mu), locked(false)
    {
    }

    MutexLocker MutexLocker::Lock(Mutex* mu)
    {
        return {mu};
    }

    MutexLocker MutexLocker::Adopt(Mutex* mu)
    {
        return {mu, std::adopt_lock};
    }

    MutexLocker MutexLocker::DeferLock(Mutex* mu)
    {
        return {mu, std::defer_lock};
    }

    MutexLocker::~MutexLocker()
    {
        if (locked)
        {
            mut->Unlock();
        }
    }

    void MutexLocker::Lock()
    {
        mut->Lock();
        locked = true;
    }

    bool MutexLocker::TryLock()
    {
        return locked = mut->TryLock();
    }

    void MutexLocker::Unlock()
    {
        mut->Unlock();
        locked = false;
    }
} // namespace Flense::Core
