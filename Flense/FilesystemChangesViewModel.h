#pragma once

#include "FilesystemChangesViewModel.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemChangesViewModel : FilesystemChangesViewModelT<FilesystemChangesViewModel>,
                                        wil::notify_property_changed_base<FilesystemChangesViewModel>
    {
        FilesystemChangesViewModel() = default;

        // We can't use wil::single_threaded_notifying_property here as all of these setters have custom logic in the
        // setters to ensure the filtering re-runs when they are changed

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Nodes();
        void Nodes(
            const winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode>& value);

        winrt::hstring SearchQuery();
        void SearchQuery(const winrt::hstring& value);

        winrt::Flense::FilesystemChangeVisibility ChangeVisibility();
        void ChangeVisibility(const winrt::Flense::FilesystemChangeVisibility& value);

      private:
        void ApplyFilters();
        void ScheduleApplyFilters();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_nodes{nullptr};
        winrt::hstring m_searchQuery;
        winrt::Flense::FilesystemChangeVisibility m_changeVisibility{true, true, true, true};

        winrt::Microsoft::UI::Dispatching::DispatcherQueueTimer m_searchDebounceTimer{nullptr};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemChangesViewModel
        : FilesystemChangesViewModelT<FilesystemChangesViewModel, implementation::FilesystemChangesViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
