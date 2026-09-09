#pragma once

#include "TitleBarService.g.h"

namespace winrt::Flense::implementation
{
    struct TitleBarService : TitleBarServiceT<TitleBarService>, wil::notify_property_changed_base<TitleBarService>
    {
        TitleBarService() : INIT_NOTIFYING_PROPERTY(Title, DefaultTitle)
        {
        }

        static winrt::Flense::TitleBarService Instance();

        wil::single_threaded_notifying_property<winrt::hstring> Title;

        void Reset();

      private:
        static constexpr std::wstring_view DefaultTitle{L"Flense"};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct TitleBarService : TitleBarServiceT<TitleBarService, implementation::TitleBarService>
    {
    };
} // namespace winrt::Flense::factory_implementation
