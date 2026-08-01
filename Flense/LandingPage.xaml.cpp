#include "pch.h"

#include "ImageDetailsPage.xaml.h"
#include "LandingPage.xaml.h"

#if __has_include("LandingPage.g.cpp")
#include "LandingPage.g.cpp"
#endif

#include <ShObjIdl.h>
#include <winrt/Microsoft.UI.Content.h>
#include <winrt/Microsoft.UI.Interop.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Windows.Storage.Pickers.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI;
using namespace Microsoft::UI::Xaml;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Pickers;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    fire_and_forget LandingPage::OpenImageFileButton_Click(IInspectable const& sender, RoutedEventArgs const& e)
    {
        if (auto button = sender.try_as<Controls::Button>())
        {
            button.IsEnabled(false);

            auto picker = FileOpenPicker();

            auto initializeWithWindow = picker.as<::IInitializeWithWindow>();
            HWND hwnd = GetWindowFromWindowId(button.XamlRoot().ContentIslandEnvironment().AppWindowId());
            initializeWithWindow->Initialize(hwnd);

            picker.CommitButtonText(L"Upload Image");
            picker.SuggestedStartLocation(PickerLocationId::Downloads);
            picker.ViewMode(PickerViewMode::List);
            picker.FileTypeFilter().Append(L".tar");

            StorageFile file = co_await picker.PickSingleFileAsync();

            button.IsEnabled(true);

            if (file)
            {
                Frame().Navigate(xaml_typename<Flense::ImageDetailsPage>(), file);
            }
        }
    }
} // namespace winrt::Flense::implementation
