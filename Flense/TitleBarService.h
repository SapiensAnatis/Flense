#pragma once

#include "TitleBarService.g.h"

namespace winrt::Flense::implementation
{
    struct TitleBarService : TitleBarServiceT<TitleBarService>
    {
        TitleBarService() = default;

        static winrt::Flense::TitleBarService Instance();

        winrt::hstring Title() const
        {
            return m_title;
        }

        void Title(const winrt::hstring& value);

        void Reset();

        winrt::event_token PropertyChanged(
            const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler& handler);
        void PropertyChanged(const winrt::event_token& token) noexcept;

      private:
        static constexpr std::wstring_view DefaultTitle{L"Flense"};

        winrt::hstring m_title{DefaultTitle};
        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct TitleBarService : TitleBarServiceT<TitleBarService, implementation::TitleBarService>
    {
    };
} // namespace winrt::Flense::factory_implementation
