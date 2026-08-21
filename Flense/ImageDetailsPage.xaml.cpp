#include "pch.h"

#include "ImageDetailsPage.xaml.h"
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
                // TODO: check this doesn't conflict with the back button when it's added
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

    void ImageDetailsPage::CancelLoadingButton_Click(const IInspectable& /* sender */, const RoutedEventArgs& /* e */)
    {
        if (m_loadAsyncAction)
        {
            m_loadAsyncAction.Cancel();
        }
    }
} // namespace winrt::Flense::implementation
