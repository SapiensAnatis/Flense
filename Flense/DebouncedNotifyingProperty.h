#pragma once

#include "Debouncer.h"

#define INIT_DEBOUNCED_NOTIFYING_PROPERTY(NAME, DELAY, VALUE) NAME(&m_propertyChanged, *this, L## #NAME, DELAY, VALUE)

namespace winrt::Flense::implementation
{
    template <typename T> class DebouncedNotifyingProperty
    {
      public:
        template <typename... TArgs>
        DebouncedNotifyingProperty(winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler>* npc,
                                   const winrt::Windows::Foundation::IInspectable& sender, std::wstring_view name,
                                   winrt::Windows::Foundation::TimeSpan delay, TArgs&&... value)
            : m_inner(npc, sender, name, std::forward<TArgs>(value)...), m_pending(m_inner()),
              m_debouncer([this] { m_inner(m_pending); }, delay)
        {
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
                m_debouncer.Debounce();
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

        DebouncedNotifyingProperty(const DebouncedNotifyingProperty&) = delete;
        DebouncedNotifyingProperty& operator=(const DebouncedNotifyingProperty&) = delete;

      private:
        wil::single_threaded_notifying_property<T> m_inner;
        T m_pending;
        Debouncer m_debouncer;
    };
} // namespace winrt::Flense::implementation
