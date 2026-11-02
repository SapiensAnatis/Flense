#pragma once

#include "DebouncedNotifyingProperty.h"
#include "ImageSelectionViewModel.g.h"

namespace winrt::Flense::implementation
{
    struct ImageSelectionViewModel : ImageSelectionViewModelT<ImageSelectionViewModel>,
                                     wil::notify_property_changed_base<ImageSelectionViewModel>
    {
        ImageSelectionViewModel()
            : INIT_NOTIFYING_PROPERTY(Images,
                                      winrt::single_threaded_observable_vector<winrt::Flense::DockerEngineImage>()),
              INIT_DEBOUNCED_NOTIFYING_PROPERTY(
                  IsLoading, std::chrono::milliseconds(250),
                  false), // Start false so quick loads don't show the loading indicator at all
              INIT_NOTIFYING_PROPERTY(HasLoaded, false)
        {
        }

        wil::single_threaded_notifying_property<
            winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::DockerEngineImage>>
            Images;

        DebouncedNotifyingProperty<bool> IsLoading;

        // Keeps the list unloaded until it has data, so a slow first load doesn't show it empty and then replace it
        // with the loading panel.
        wil::single_threaded_notifying_property<bool> HasLoaded;

        winrt::Windows::Foundation::IAsyncAction LoadImageList();
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageSelectionViewModel
        : ImageSelectionViewModelT<ImageSelectionViewModel, implementation::ImageSelectionViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
