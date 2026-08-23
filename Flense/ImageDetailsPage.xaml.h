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

        winrt::fire_and_forget OnNavigatedTo(const winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs& e);

        void OnNavigatedFrom(const winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs& e);

        void CancelLoadingButton_Click(const winrt::Windows::Foundation::IInspectable& sender,
                                       const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);

      private:
        winrt::Flense::ImageDetailsViewModel m_viewModel;
        winrt::Windows::Foundation::IAsyncAction m_loadAsyncAction{nullptr};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage, implementation::ImageDetailsPage>
    {
    };
} // namespace winrt::Flense::factory_implementation
