#pragma once

#include "ImageDetailsViewModel.g.h"

namespace winrt::Flense::implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel>
    {
        ImageDetailsViewModel() = default;

        winrt::Windows::Storage::StorageFile ImageArchive();
        void ImageArchive(winrt::Windows::Storage::StorageFile const& value);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> Filenames();

        winrt::event_token PropertyChanged(
            winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;

      private:
        winrt::Windows::Storage::StorageFile m_imageFile{nullptr};
        winrt::hstring m_fileName;
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> m_filenames{
            winrt::single_threaded_observable_vector<winrt::hstring>()};
        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel, implementation::ImageDetailsViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
