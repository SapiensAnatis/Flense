#pragma once

#include "FilesystemChangesView.g.h"
#include "FilesystemChangesViewModel.h"

namespace winrt::Flense::implementation
{
    struct FilesystemChangesView : FilesystemChangesViewT<FilesystemChangesView>
    {
        FilesystemChangesView();

        winrt::Flense::FilesystemChangesViewModel ViewModel();

        static winrt::Microsoft::UI::Xaml::DependencyProperty ItemsSourceProperty();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode> ItemsSource();
        void ItemsSource(
            const winrt::Windows::Foundation::Collections::IObservableVector<winrt::Flense::FilesystemTreeNode>& value);

      private:
        static void InitializeProperties();

        static void OnItemsSourceChanged(const winrt::Microsoft::UI::Xaml::DependencyObject& sender,
                                         const winrt::Microsoft::UI::Xaml::DependencyPropertyChangedEventArgs& args);

        static winrt::Microsoft::UI::Xaml::DependencyProperty s_itemsSourceProperty;

        winrt::Flense::FilesystemChangesViewModel m_viewModel;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemChangesView : FilesystemChangesViewT<FilesystemChangesView, implementation::FilesystemChangesView>
    {
    };
} // namespace winrt::Flense::factory_implementation
