#pragma once

// The include order below is important as wil may not define cppwinrt symbols if winrt headers are not included yet.
// clang-format off
#include <winrt/Microsoft.UI.Dispatching.h>
#include <wil/cppwinrt_authoring.h>
// clang-format on

#include <functional>

#define INIT_DEBOUNCED_NOTIFYING_PROPERTY(NAME, DELAY, VALUE) NAME(&m_propertyChanged, *this, L## #NAME, DELAY, VALUE)

namespace winrt::Flense::implementation
{
    template <typename T> class DebouncedNotifyingProperty
    {
      public:
        using Timer = winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer;

        template <typename... TArgs>
        DebouncedNotifyingProperty(winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler>* npc,
                                   const winrt::Windows::Foundation::IInspectable& sender, std::wstring_view name,
                                   winrt::Windows::Foundation::TimeSpan delay, TArgs&&... value)
            : m_inner(npc, sender, name, std::forward<TArgs>(value)...), m_pending(m_inner())
        {
            auto dispatcher = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

            if (!dispatcher)
            {
                throw winrt::hresult_error(
                    E_UNEXPECTED,
                    L"DebouncedNotifyingProperty must be constructed on a thread with a DispatcherQueue.");
            }

            m_timer = dispatcher.CreateTimer();

            m_timer.Interval(delay);
            m_timer.IsRepeating(false);

            m_tick = m_timer.Tick(winrt::auto_revoke, [this](auto&&, auto&&) { Flush(); });
        }

        decltype(auto) operator()() const noexcept
        {
            return m_inner();
        }

        template <typename Q> DebouncedNotifyingProperty& operator=(Q&& newValue)
        {
            if (m_pending != newValue)
            {
                m_pending = std::forward<Q>(newValue);
                m_timer.Stop();
                m_timer.Start();
            }

            return *this;
        }

        template <typename Q> DebouncedNotifyingProperty& operator()(Q&& newValue)
        {
            return *this = std::forward<Q>(newValue);
        }

        std::wstring_view Name() const noexcept
        {
            return m_inner.Name();
        }

        void OnCommitted(std::function<void()> func)
        {
            m_onCommittedCallback = std::move(func);
        }

        void Flush()
        {
            m_timer.Stop();

            if (m_pending == m_inner())
            {
                return;
            }

            m_inner(m_pending);

            if (m_onCommittedCallback)
            {
                m_onCommittedCallback();
            }
        }

        DebouncedNotifyingProperty(const DebouncedNotifyingProperty&) = delete;
        DebouncedNotifyingProperty& operator=(const DebouncedNotifyingProperty&) = delete;

      private:
        wil::single_threaded_notifying_property<T> m_inner;
        T m_pending;
        Timer m_timer{nullptr};
        Timer::Tick_revoker m_tick;
        std::function<void()> m_onCommittedCallback;
    };
} // namespace winrt::Flense::implementation