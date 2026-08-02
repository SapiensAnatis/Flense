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

        void OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

      private:
        winrt::fire_and_forget ProcessFileAsync();

        winrt::Flense::ImageDetailsViewModel m_viewModel{ winrt::make<implementation::ImageDetailsViewModel>() };
    };
}

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage, implementation::ImageDetailsPage>
    {
    };
}
