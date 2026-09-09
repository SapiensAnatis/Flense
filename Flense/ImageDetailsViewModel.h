#pragma once

#include "ImageDetailsViewModel.g.h"

#include <vector>

import Flense.Core;

namespace winrt::Flense::implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel>,
                                   wil::notify_property_changed_base<ImageDetailsViewModel>
    {
        ImageDetailsViewModel()
            : INIT_NOTIFYING_PROPERTY(ImageArchive, nullptr),
              INIT_NOTIFYING_PROPERTY(Layers,
                                      winrt::single_threaded_observable_vector<winrt::Flense::ImageLayerWrapper>()),
              INIT_NOTIFYING_PROPERTY(IsLoading, true), INIT_NOTIFYING_PROPERTY(LoadingProgress, 0.0),
              INIT_NOTIFYING_PROPERTY(StatusMessage, L"")
        {
        }

        wil::single_threaded_notifying_property<winrt::Windows::Storage::StorageFile> ImageArchive;
        wil::single_threaded_notifying_property<
            winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::ImageLayerWrapper>>
            Layers;
        wil::single_threaded_notifying_property<bool> IsLoading;
        wil::single_threaded_notifying_property<double> LoadingProgress;
        wil::single_threaded_notifying_property<winrt::hstring> StatusMessage;

        // Needs custom logic to deallocate last layer tree on set
        winrt::Flense::ImageLayerWrapper SelectedLayer();
        void SelectedLayer(const winrt::Flense::ImageLayerWrapper& value);

        winrt::Windows::Foundation::IAsyncAction LoadAsync();

      private:
        winrt::Flense::ImageLayerWrapper m_selectedLayer{nullptr};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel, implementation::ImageDetailsViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
