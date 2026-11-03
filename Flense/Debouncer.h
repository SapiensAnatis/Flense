#pragma once

// The include order below is important as wil may not define cppwinrt symbols if winrt headers are not included yet.
// clang-format off
#include <winrt/Microsoft.UI.Dispatching.h>
#include <wil/cppwinrt_authoring.h>
// clang-format on

#include <functional>

namespace winrt::Flense::implementation
{
    class Debouncer
    {
        using Timer = winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer;

      public:
        Debouncer(std::function<void()> callback, winrt::Windows::Foundation::TimeSpan delay)
            : m_callback(std::move(callback))
        {
            auto dispatcher = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

            if (!dispatcher)
            {
                throw winrt::hresult_error(E_UNEXPECTED,
                                           L"Debouncer must be constructed on a thread with a DispatcherQueue.");
            }

            m_timer = dispatcher.CreateTimer();

            m_timer.Interval(delay);
            m_timer.IsRepeating(false);

            m_tick = m_timer.Tick(winrt::auto_revoke, [this](auto&&, auto&&) { Flush(); });
        }

        Debouncer(const Debouncer&) = delete;
        Debouncer& operator=(const Debouncer&) = delete;

        void Debounce()
        {
            m_timer.Stop();
            m_timer.Start();
        }

        void Flush()
        {
            m_timer.Stop();
            m_callback();
        }

      private:
        std::function<void()> m_callback;
        Timer m_timer{nullptr};
        Timer::Tick_revoker m_tick;
    };
} // namespace winrt::Flense::implementation
