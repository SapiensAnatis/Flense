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
        // UI Automation invokes the back button through ProgrammaticClick, which ignores the open
        // dialog's input blocker and so can raise this a second time. Showing a second
        // ContentDialog throws, and an exception escaping a fire_and_forget terminates the process.
        if (m_backDialogShowing)
        {
            co_return;
        }

        auto lifetime = get_strong();

        Controls::ContentDialog dialog;
        dialog.XamlRoot(rootFrame().XamlRoot());
        dialog.Title(box_value(L"Close image analysis?"));
        dialog.Content(box_value(L"You'll return to the main screen."));
        dialog.PrimaryButtonText(L"Close");
        dialog.CloseButtonText(L"Cancel");
        dialog.DefaultButton(Controls::ContentDialogButton::Primary);

        m_backDialogShowing = true;

        const auto result = co_await dialog.ShowAsync();

        m_backDialogShowing = false;

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
