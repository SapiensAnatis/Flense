#pragma once

#include "LandingPage.g.h"

namespace winrt::Flense::implementation
{
    struct LandingPage : LandingPageT<LandingPage>
    {
        LandingPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        fire_and_forget OpenImageFileButton_Click(winrt::Windows::Foundation::IInspectable const& sender,
                                                  winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct LandingPage : LandingPageT<LandingPage, implementation::LandingPage>
    {
    };
} // namespace winrt::Flense::factory_implementation
