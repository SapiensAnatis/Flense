#pragma once

#include "DebouncedNotifyingProperty.h"
#include "Debouncer.h"
#include "DockerEngineImage.h"
#include "ImageSelectionViewModel.g.h"

#include <chrono>
#include <vector>

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
              INIT_NOTIFYING_PROPERTY(HasLoaded, false),
              m_searchDebouncer([this] { ApplySearchFilter(); }, std::chrono::milliseconds(300))
        {
        }

        wil::single_threaded_notifying_property<
            winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::DockerEngineImage>>
            Images;

        DebouncedNotifyingProperty<bool> IsLoading;

        // Keeps the list unloaded until it has data, so a slow first load doesn't show it empty and then replace it
        // with the loading panel.
        wil::single_threaded_notifying_property<bool> HasLoaded;

        // Manual search query property - needs to call debouncer on set
        winrt::hstring SearchQuery();
        void SearchQuery(const winrt::hstring& value);

        winrt::Windows::Foundation::IAsyncAction LoadImageList();

      private:
        void ApplySearchFilter();

        winrt::hstring m_searchQuery;
        std::vector<winrt::Flense::DockerEngineImage> m_allImages;
        Debouncer m_searchDebouncer;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct ImageSelectionViewModel
        : ImageSelectionViewModelT<ImageSelectionViewModel, implementation::ImageSelectionViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
