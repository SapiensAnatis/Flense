#include "pch.h"
#include "ModulePreamble.h"

#include "DeveloperMenuBarItem.xaml.h"
#if __has_include("DeveloperMenuBarItem.g.cpp")
#include "DeveloperMenuBarItem.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Flense::implementation
{
    void DeveloperMenuBarItem::ToggleTheme_Click(const IInspectable& /* sender */, const RoutedEventArgs& /* e */)
    {
        if (auto root = XamlRoot().Content().try_as<FrameworkElement>())
        {
            root.RequestedTheme(root.ActualTheme() == ElementTheme::Dark ? ElementTheme::Light : ElementTheme::Dark);
        }
    }
} // namespace winrt::Flense::implementation
