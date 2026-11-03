#pragma once

#include "ImageDetailsViewModel.g.h"

#include <vector>

import Flense.Core.ImageParsing;

namespace winrt::Flense::implementation
{
    struct ImageDetailsViewModel : ImageDetailsViewModelT<ImageDetailsViewModel>,
                                   wil::notify_property_changed_base<ImageDetailsViewModel>
    {
        ImageDetailsViewModel()
            : INIT_NOTIFYING_PROPERTY(ImageArchivePath, L""),
              INIT_NOTIFYING_PROPERTY(Layers,
                                      winrt::single_threaded_observable_vector<winrt::Flense::ImageLayerWrapper>()),
              INIT_NOTIFYING_PROPERTY(IsLoading, true), INIT_NOTIFYING_PROPERTY(LoadingProgress, 0.0),
              INIT_NOTIFYING_PROPERTY(StatusMessage, L""), INIT_NOTIFYING_PROPERTY(ErrorMessage, L""),
              INIT_NOTIFYING_PROPERTY(ErrorDetails, L""),
              INIT_NOTIFYING_PROPERTY(FilterQuery, L""), INIT_NOTIFYING_PROPERTY(ShowUnchanged, true),
              INIT_NOTIFYING_PROPERTY(ShowAdded, true), INIT_NOTIFYING_PROPERTY(ShowModified, true),
              INIT_NOTIFYING_PROPERTY(ShowRemoved, true)
        {
        }

        wil::single_threaded_notifying_property<winrt::hstring> ImageArchivePath;
        wil::single_threaded_notifying_property<
            winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::ImageLayerWrapper>>
            Layers;
        wil::single_threaded_notifying_property<bool> IsLoading;
        wil::single_threaded_notifying_property<double> LoadingProgress;
        wil::single_threaded_notifying_property<winrt::hstring> StatusMessage;
        wil::single_threaded_notifying_property<winrt::hstring> ErrorMessage;
        wil::single_threaded_notifying_property<winrt::hstring> ErrorDetails;
        wil::single_threaded_notifying_property<winrt::hstring> FilterQuery;
        wil::single_threaded_notifying_property<bool> ShowUnchanged;
        wil::single_threaded_notifying_property<bool> ShowAdded;
        wil::single_threaded_notifying_property<bool> ShowModified;
        wil::single_threaded_notifying_property<bool> ShowRemoved;

        // Needs custom logic to deallocate last layer tree on set
        winrt::Flense::ImageLayerWrapper SelectedLayer();
        void SelectedLayer(const winrt::Flense::ImageLayerWrapper& value);

        winrt::Windows::Foundation::IAsyncAction LoadAsync();

        static winrt::Flense::FilesystemChangeVisibility BuildChangeVisibility(bool showUnchanged, bool showAdded,
                                                                               bool showModified, bool showRemoved);

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
