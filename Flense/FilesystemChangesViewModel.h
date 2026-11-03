#pragma once

#include "Debouncer.h"
#include "FilesystemChangesViewModel.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemChangesViewModel : FilesystemChangesViewModelT<FilesystemChangesViewModel>,
                                        wil::notify_property_changed_base<FilesystemChangesViewModel>
    {
        FilesystemChangesViewModel();

        // We can't use wil::single_threaded_notifying_property for Nodes or ChangeVisibility as their setters have
        // custom logic to ensure the filtering re-runs when they are changed

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Nodes();
        void Nodes(
            const winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode>& value);

        winrt::Flense::FilesystemChangeVisibility ChangeVisibility();
        void ChangeVisibility(const winrt::Flense::FilesystemChangeVisibility& value);

        winrt::hstring SearchQuery();
        void SearchQuery(const winrt::hstring& value);

      private:
        void ApplyFilters();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_nodes{nullptr};
        winrt::Flense::FilesystemChangeVisibility m_changeVisibility{true, true, true, true};
        winrt::hstring m_searchQuery;
        Debouncer m_searchDebouncer;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemChangesViewModel
        : FilesystemChangesViewModelT<FilesystemChangesViewModel, implementation::FilesystemChangesViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
