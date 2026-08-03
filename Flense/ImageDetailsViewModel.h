#pragma once

#include "ImageDetailsViewModel.g.h"
#include "ImageParser.h"

#include <vector>

namespace winrt::Flense::implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel>
    {
        ImageDetailsViewModel() = default;

        winrt::Windows::Storage::StorageFile ImageArchive();
        void ImageArchive(winrt::Windows::Storage::StorageFile const& value);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> Filenames();

        bool IsLoading();
        bool IsLoaded();
        double LoadingProgress();

        winrt::Windows::Foundation::IAsyncAction LoadAsync();

        winrt::event_token PropertyChanged(
            winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler const& handler);
        void PropertyChanged(winrt::event_token const& token) noexcept;

      private:
        void IsLoading(bool value);
        void LoadingProgress(double value);

        winrt::Windows::Storage::StorageFile m_imageFile{nullptr};
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::hstring> m_filenames{
            winrt::single_threaded_observable_vector<winrt::hstring>()};
        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;

        bool m_isLoading{true};
        double m_loadingProgress{0.0};
        std::vector<::Flense::Core::ImageLayer> m_layers;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel, implementation::ImageDetailsViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
