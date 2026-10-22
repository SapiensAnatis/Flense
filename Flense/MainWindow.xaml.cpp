#include "pch.h"

#include "DialogService.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "winrt/Flense.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Microsoft::UI::Xaml::Navigation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    void MainWindow::InitializeComponent()
    {
        MainWindowT::InitializeComponent();
        ExtendsContentIntoTitleBar(true);
        SetTitleBar(AppTitleBar());

#if defined(_DEBUG)
        AppMenuBar().Items().Append(winrt::Flense::DeveloperMenuBarItem());
#endif
    }

    winrt::Flense::TitleBarService MainWindow::TitleBarService()
    {
        return winrt::Flense::TitleBarService::Instance();
    }

    void MainWindow::Exit_Click(const IInspectable& /* sender */, const RoutedEventArgs& /* e */)
    {
        Application::Current().Exit();
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

        // UI Automation invokes the back button through ProgrammaticClick, which ignores the open
        // dialog's input blocker and so can raise this a second time. DialogService drops the second
        // request, which arrives here as None.
        const auto result = co_await DialogService::Instance().ShowAsync(dialog);

        if (result && result.Value() == Controls::ContentDialogResult::Primary && rootFrame().CanGoBack())
        {
            if (auto detailsPage = rootFrame().Content().try_as<winrt::Flense::ImageDetailsPage>())
            {
                detailsPage.CancelLoading();
            }

            rootFrame().GoBack();
        }
    }
} // namespace winrt::Flense::implementation
