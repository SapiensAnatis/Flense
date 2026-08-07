#pragma once

#include "FilesystemItemTemplateSelector.g.h"

namespace winrt::Flense::implementation
{
    struct FilesystemItemTemplateSelector : FilesystemItemTemplateSelectorT<FilesystemItemTemplateSelector>
    {
        FilesystemItemTemplateSelector() = default;

        Microsoft::UI::Xaml::DataTemplate SelectTemplateCore(
            const winrt::Windows::Foundation::IInspectable& item,
            const winrt::Microsoft::UI::Xaml::DependencyObject& container);
        Microsoft::UI::Xaml::DataTemplate SelectTemplateCore(const winrt::Windows::Foundation::IInspectable& item);

        Microsoft::UI::Xaml::DataTemplate FileTemplate();
        void FileTemplate(const Microsoft::UI::Xaml::DataTemplate& value);

        Microsoft::UI::Xaml::DataTemplate FolderTemplate();
        void FolderTemplate(const Microsoft::UI::Xaml::DataTemplate& value);

      private:
        Microsoft::UI::Xaml::DataTemplate m_fileTemplate{nullptr};
        Microsoft::UI::Xaml::DataTemplate m_folderTemplate{nullptr};
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct FilesystemItemTemplateSelector
        : FilesystemItemTemplateSelectorT<FilesystemItemTemplateSelector,
                                          implementation::FilesystemItemTemplateSelector>
    {
    };
} // namespace winrt::Flense::factory_implementation
