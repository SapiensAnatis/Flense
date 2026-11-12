#pragma once

#include "ImageSelectionPage.g.h"
#include "ImageSelectionViewModel.h"

namespace winrt::Flense::implementation
{
    struct ImageSelectionPage : ImageSelectionPageT<ImageSelectionPage>
    {
        ImageSelectionPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        wil::single_threaded_property<winrt::Flense::ImageSelectionViewModel> ViewModel;

        winrt::fire_and_forget OnNavigatedTo(const winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs& e);
        void OnNavigatedFrom(const winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs& e);

        void CancelLoadingButton_Click(const winrt::Windows::Foundation::IInspectable& sender,
                                       const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);
        void ImageList_ItemInvoked(const winrt::Microsoft::UI::Xaml::Controls::ItemsView& sender,
                                   const winrt::Microsoft::UI::Xaml::Controls::ItemsViewItemInvokedEventArgs& args);
        void ImageList_SelectionChanged(
            const winrt::Microsoft::UI::Xaml::Controls::ItemsView& sender,
            const winrt::Microsoft::UI::Xaml::Controls::ItemsViewSelectionChangedEventArgs& args);
        void OpenImageButton_Click(const winrt::Windows::Foundation::IInspectable& sender,
                                   const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);
        void OpenImageButton_Loaded(const winrt::Windows::Foundation::IInspectable& sender,
                                    const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);
        void CancelButton_Click(const winrt::Windows::Foundation::IInspectable& sender,
                                const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);
        void LoadFailureBackButton_Loaded(const winrt::Windows::Foundation::IInspectable& sender,
                                          const winrt::Microsoft::UI::Xaml::RoutedEventArgs& e);

      private:
        void CancelLoading();
        void SelectImage(const winrt::Flense::DockerEngineImage& image);

        winrt::Windows::Foundation::IAsyncAction m_loadAsyncAction{nullptr};
        bool m_navigatedAway = false;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageSelectionPage : ImageSelectionPageT<ImageSelectionPage, implementation::ImageSelectionPage>
    {
    };
} // namespace winrt::Flense::factory_implementation
