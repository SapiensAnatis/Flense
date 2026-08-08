#pragma once

#include "ImageDetailsPage.g.h"
#include "ImageDetailsViewModel.h"

namespace winrt::Flense::implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage>
    {
        ImageDetailsPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        winrt::Flense::ImageDetailsViewModel ViewModel();

        winrt::fire_and_forget OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);
        void OnNavigatedFrom(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

      private:
        winrt::Flense::ImageDetailsViewModel m_viewModel;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage, implementation::ImageDetailsPage>
    {
    };
} // namespace winrt::Flense::factory_implementation
