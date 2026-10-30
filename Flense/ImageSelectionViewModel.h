#pragma once

#include "ImageSelectionViewModel.g.h"

namespace winrt::Flense::implementation
{
    struct ImageSelectionViewModel : ImageSelectionViewModelT<ImageSelectionViewModel>,
                                     wil::notify_property_changed_base<ImageSelectionViewModel>
    {
        ImageSelectionViewModel()
            : INIT_NOTIFYING_PROPERTY(Images,
                                      winrt::single_threaded_observable_vector<winrt::Flense::DockerEngineImage>())
        {
        }

        wil::single_threaded_notifying_property<
            winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::DockerEngineImage>>
            Images;

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
