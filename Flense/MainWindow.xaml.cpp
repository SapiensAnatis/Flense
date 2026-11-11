#include "pch.h"

#include "DialogService.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "AboutDialog.xaml.h"
#include "AppVersion.h"
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

        auto& dialogService = DialogService::Instance();
        dialogService.SetWindow(*this);
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

        auto detailsPage = rootFrame().Content().try_as<winrt::Flense::ImageDetailsPage>();
        if (!detailsPage)
        {
            if (rootFrame().CanGoBack())
            {
                rootFrame().GoBack();
            }
            co_return;
        }

        Controls::ContentDialog dialog;
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
            detailsPage.CancelLoading();
            rootFrame().GoBack();
        }
    }

    winrt::fire_and_forget MainWindow::About_Click(const IInspectable& /* sender */, const RoutedEventArgs& /* e */)
    {
        Flense::AboutDialog dialog;

        auto discard = co_await DialogService::Instance().ShowAsync(dialog);
    }

    winrt::fire_and_forget MainWindow::ReportABug_Click(const winrt::Windows::Foundation::IInspectable& /* sender */,
                                                        const winrt::Microsoft::UI::Xaml::RoutedEventArgs& /* e */)
    {
        static const hstring WindowsVersion = [] {
            using winrt::Windows::System::Profile::AnalyticsInfo;

            const std::string version = winrt::to_string(AnalyticsInfo::VersionInfo().DeviceFamilyVersion());

            std::uint64_t packed = 0;
            const auto [ptr, ec] =
                std::from_chars(std::to_address(version.begin()), std::to_address(version.end()), packed);

            if (ec != std::errc{})
            {
                return hstring(L"Unknown");
            }

            return hstring(std::format(L"{}.{}.{}.{}", (packed >> 48) & 0xFFFF, (packed >> 32) & 0xFFFF,
                                       (packed >> 16) & 0xFFFF, packed & 0xFFFF));
        }();

        const std::wstring url = std::format(L"https://github.com/SapiensAnatis/Flense/issues/"
                                             L"new?template=bug_report.yml&app-version={}&os-version={}",
                                             GetAppVersionString(), WindowsVersion);

        std::optional<hstring> error;

        try
        {
            const bool launched =
                co_await winrt::Windows::System::Launcher::LaunchUriAsync(winrt::Windows::Foundation::Uri{url});

            if (!launched)
            {
                error = L"No app was available to open the link.";
            }
        }
        catch (const hresult_error& ex)
        {
            error = ex.message();
        }
        catch (...)
        {
            error = L"Unknown error";
        }

        if (error)
        {
            Controls::ContentDialog dialog;
            dialog.Title(box_value(L"Couldn't open browser"));
            dialog.Content(box_value(*error));
            dialog.CloseButtonText(L"OK");

            co_await DialogService::Instance().ShowAsync(dialog);
        }
    }
} // namespace winrt::Flense::implementation
