#pragma once

#include "FilesystemChangesViewModel.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemChangesViewModel : FilesystemChangesViewModelT<FilesystemChangesViewModel>
    {
        FilesystemChangesViewModel() = default;

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> Nodes();
        void Nodes(
            const winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode>& value);

        winrt::event_token PropertyChanged(
            const winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler& handler);
        void PropertyChanged(const winrt::event_token& token) noexcept;

      private:
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> m_nodes{nullptr};

        winrt::event<winrt::Microsoft::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemChangesViewModel
        : FilesystemChangesViewModelT<FilesystemChangesViewModel, implementation::FilesystemChangesViewModel>
    {
    };
} // namespace winrt::Flense::factory_implementation
