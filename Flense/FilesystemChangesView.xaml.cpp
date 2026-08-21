#include "pch.h"

#include "FilesystemChangesView.xaml.h"
#if __has_include("FilesystemChangesView.g.cpp")
#include "FilesystemChangesView.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Windows::Foundation;

namespace winrt::Flense::implementation
{
    FilesystemChangesView::FilesystemChangesView()
    {
        InitializeComponent();
    }

    IInspectable FilesystemChangesView::ItemsSource()
    {
        return m_itemsSource;
    }

    void FilesystemChangesView::ItemsSource(const IInspectable& value)
    {
        if (m_itemsSource != value)
        {
            m_itemsSource = value;
            FilesystemTree().ItemsSource(value);
        }
    }
} // namespace winrt::Flense::implementation
