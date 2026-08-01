#pragma once

#include "ImageDetailsPage.g.h"

namespace winrt::Flense::implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage>
    {
        ImageDetailsPage()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        winrt::Windows::Storage::StorageFile ImageFile();
        void ImageFile(winrt::Windows::Storage::StorageFile const& value);

        void OnNavigatedTo(winrt::Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

      private:
        winrt::fire_and_forget ProcessFileAsync();

        winrt::Windows::Storage::StorageFile m_imageFile{ nullptr };
    };
}

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsPage : ImageDetailsPageT<ImageDetailsPage, implementation::ImageDetailsPage>
    {
    };
}
