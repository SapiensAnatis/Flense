#include "pch.h"
#include "FilesystemItemTemplateSelector.h"
#if __has_include("FilesystemItemTemplateSelector.g.cpp")
#include "FilesystemItemTemplateSelector.g.cpp"
#endif

namespace winrt::Flense::implementation
{
    Microsoft::UI::Xaml::DataTemplate FilesystemItemTemplateSelector::SelectTemplateCore(
        const winrt::Windows::Foundation::IInspectable& item, const winrt::Microsoft::UI::Xaml::DependencyObject&)
    {
        return SelectTemplateCore(item);
    }

    Microsoft::UI::Xaml::DataTemplate FilesystemItemTemplateSelector::SelectTemplateCore(
        const winrt::Windows::Foundation::IInspectable& item)
    {
        if (auto node = item.try_as<winrt::Flense::FilesystemTreeNode>())
        {
            return node.IsDirectory() ? m_folderTemplate : m_fileTemplate;
        }

        return nullptr;
    }

    Microsoft::UI::Xaml::DataTemplate FilesystemItemTemplateSelector::FileTemplate()
    {
        return m_fileTemplate;
    }

    void FilesystemItemTemplateSelector::FileTemplate(const Microsoft::UI::Xaml::DataTemplate& value)
    {
        m_fileTemplate = value;
    }

    Microsoft::UI::Xaml::DataTemplate FilesystemItemTemplateSelector::FolderTemplate()
    {
        return m_folderTemplate;
    }

    void FilesystemItemTemplateSelector::FolderTemplate(const Microsoft::UI::Xaml::DataTemplate& value)
    {
        m_folderTemplate = value;
    }
}
