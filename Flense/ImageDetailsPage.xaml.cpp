#include "pch.h"
#include "ModulePreamble.h"

#include "ImageDetailsPage.xaml.h"
#include "TitleBarService.h"
#if __has_include("ImageDetailsPage.g.cpp")
#include "ImageDetailsPage.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Storage;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Navigation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    Flense::ImageDetailsViewModel ImageDetailsPage::ViewModel()
    {
        return m_viewModel;
    }

    winrt::fire_and_forget ImageDetailsPage::OnNavigatedTo(const NavigationEventArgs& e)
    {
        auto lifetime = get_strong();

        if (auto imageArchive = e.Parameter().try_as<StorageFile>())
        {
            m_viewModel.ImageArchive(imageArchive);
            m_viewModel.StatusMessage(L"Loading image: " + imageArchive.Name());
            m_loadAsyncAction = m_viewModel.LoadAsync();

            try
            {
                co_await m_loadAsyncAction;
            }
            catch (const winrt::hresult_canceled&)
            {
                if (Frame().CanGoBack())
                {
                    Frame().GoBack();
                }
            }
        }
        else
        {
            m_viewModel.StatusMessage(L"Invalid parameter passed to ImageDetailsPage.");
        }

        m_loadAsyncAction = nullptr;
    }

    void ImageDetailsPage::OnNavigatedFrom(const NavigationEventArgs& /* e */)
    {
        // The title reflects the image this page is showing, so it must not outlive the navigation.
        TitleBarService::Instance().Reset();
    }

    void ImageDetailsPage::CancelLoadingButton_Click(const IInspectable& /* sender */, const RoutedEventArgs& /* e */)
    {
        CancelLoading();
    }

    void ImageDetailsPage::CancelLoading()
    {
        if (m_loadAsyncAction)
        {
            m_loadAsyncAction.Cancel();
        }
    }

    bool ImageDetailsPage::UnboxChecked(const IReference<bool>& value)
    {
        return value && value.Value();
    }

    winrt::Flense::FilesystemChangeVisibility ImageDetailsPage::BuildChangeVisibility(
        const IReference<bool>& showUnchanged, const IReference<bool>& showAdded, const IReference<bool>& showModified,
        const IReference<bool>& showRemoved)
    {
        return {UnboxChecked(showUnchanged), UnboxChecked(showAdded), UnboxChecked(showModified),
                UnboxChecked(showRemoved)};
    }
} // namespace winrt::Flense::implementation
