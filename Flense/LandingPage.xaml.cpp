#include "pch.h"

#include "ImageDetailsPage.xaml.h"
#include "ImageSelectionPage.xaml.h"
#include "LandingPage.xaml.h"

#if __has_include("LandingPage.g.cpp")
#include "LandingPage.g.cpp"
#endif

#include <winrt/Microsoft.UI.Content.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.Windows.Storage.Pickers.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace winrt::Microsoft::UI;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::Windows::Storage::Pickers;
using namespace winrt::Windows::Foundation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    fire_and_forget LandingPage::OpenImageFileButton_Click(const IInspectable& sender, const RoutedEventArgs& e)
    {
        if (auto button = sender.try_as<Controls::Button>())
        {
            button.IsEnabled(false);

            FileOpenPicker picker{button.XamlRoot().ContentIslandEnvironment().AppWindowId()};

            picker.CommitButtonText(L"Upload Image");
            picker.SuggestedStartLocation(PickerLocationId::Downloads);
            picker.ViewMode(PickerViewMode::List);
            picker.FileTypeFilter().Append(L".tar");
            picker.FileTypeFilter().Append(L".tar.gz");

            const PickFileResult result = co_await picker.PickSingleFileAsync();

            button.IsEnabled(true);

            if (result)
            {
                Frame().Navigate(xaml_typename<Flense::ImageDetailsPage>(), box_value(result.Path()));
            }
        }
    }

    void LandingPage::SelectImageButton_Click(const IInspectable& /* sender */, const RoutedEventArgs& /* e */)
    {
        Frame().Navigate(xaml_typename<Flense::ImageSelectionPage>());
    }
} // namespace winrt::Flense::implementation
