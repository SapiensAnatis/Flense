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
        void ImageArchive(const winrt::Windows::Storage::StorageFile& value);

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::ImageLayerWrapper> Layers();

        bool IsLoading();
        bool IsLoaded();
        double LoadingProgress();

        winrt::Windows::Foundation::IAsyncAction LoadAsync();

        winrt::event_token PropertyChanged(
            const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler& handler);
        void PropertyChanged(const winrt::event_token& token) noexcept;

      private:
        void IsLoading(bool value);
        void LoadingProgress(double value);
        void Layers(winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::ImageLayerWrapper> value);

        winrt::Windows::Storage::StorageFile m_imageFile{nullptr};

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::ImageLayerWrapper> m_layers{
            winrt::single_threaded_observable_vector<winrt::Flense::ImageLayerWrapper>()};

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;

        bool m_isLoading{true};
        double m_loadingProgress{0.0};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel, implementation::ImageDetailsViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
