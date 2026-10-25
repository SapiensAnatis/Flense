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

        fire_and_forget OpenImageFileButton_Click(const winrt::Windows::Foundation::IInspectable& sender,
                                                  const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct LandingPage : LandingPageT<LandingPage, implementation::LandingPage>
    {
    };
} // namespace winrt::Flense::factory_implementation
