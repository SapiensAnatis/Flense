#pragma once

#include "DeveloperMenuBarItem.g.h"

namespace winrt::Flense::implementation
{
    struct DeveloperMenuBarItem : DeveloperMenuBarItemT<DeveloperMenuBarItem>
    {
        DeveloperMenuBarItem()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        void ToggleTheme_Click(const winrt::Windows::Foundation::IInspectable& sender,
                               const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct DeveloperMenuBarItem : DeveloperMenuBarItemT<DeveloperMenuBarItem, implementation::DeveloperMenuBarItem>
    {
    };
} // namespace winrt::Flense::factory_implementation
