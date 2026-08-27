#include "pch.h"

#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "winrt/Flense.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Navigation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    void MainWindow::InitializeComponent()
    {
        MainWindowT::InitializeComponent();
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());
    }

    winrt::Flense::TitleBarService MainWindow::TitleBarService()
    {
        return winrt::Flense::TitleBarService::Instance();
    }

    void MainWindow::RootFrame_Navigated(const IInspectable& /* sender */, const NavigationEventArgs& /* e */)
    {
        AppTitleBar().IsBackButtonVisible(rootFrame().CanGoBack());
    }

    winrt::fire_and_forget MainWindow::AppTitleBar_BackRequested(const Controls::TitleBar& /* sender */,
                                                                  const IInspectable& /* args */)
    {
        auto lifetime = get_strong();

        Controls::ContentDialog dialog;
        dialog.XamlRoot(rootFrame().XamlRoot());
        dialog.Title(box_value(L"Close image analysis?"));
        dialog.Content(box_value(L"You'll return to the main screen."));
        dialog.PrimaryButtonText(L"Close");
        dialog.CloseButtonText(L"Cancel");
        dialog.DefaultButton(Controls::ContentDialogButton::Primary);

        auto result = co_await dialog.ShowAsync();

        if (result == Controls::ContentDialogResult::Primary && rootFrame().CanGoBack())
        {
            if (auto detailsPage = rootFrame().Content().try_as<winrt::Flense::ImageDetailsPage>())
            {
                detailsPage.CancelLoading();
            }

            rootFrame().GoBack();
        }
    }
} // namespace winrt::Flense::implementation
