#pragma once

#include "FilesystemChangesView.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemChangesView : FilesystemChangesViewT<FilesystemChangesView>
    {
        FilesystemChangesView();

        winrt::Windows::Foundation::IInspectable ItemsSource();
        void ItemsSource(const winrt::Windows::Foundation::IInspectable& value);

      private:
        winrt::Windows::Foundation::IInspectable m_itemsSource{nullptr};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemChangesView : FilesystemChangesViewT<FilesystemChangesView, implementation::FilesystemChangesView>
    {
    };
} // namespace winrt::Flense::factory_implementation
