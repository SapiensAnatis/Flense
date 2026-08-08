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

    winrt::fire_and_forget ImageDetailsPage::OnNavigatedTo(NavigationEventArgs const& e)
    {
        if (auto imageArchive = e.Parameter().try_as<StorageFile>())
        {
            m_viewModel.ImageArchive(imageArchive);
            MessageTextBlock().Text(L"Loading image: " + imageArchive.Name());
            co_await m_viewModel.LoadAsync();
        }
        else
        {
            MessageTextBlock().Text(L"Invalid parameter passed to ImageDetailsPage.");
        }
    }

    void ImageDetailsPage::OnNavigatedFrom(NavigationEventArgs const&)
    {
        // A load in progress owns worker threads chewing through layer blobs. Nobody is going to look
        // at the result now, so tell it to stop rather than letting it run to completion.
        winrt::get_self<implementation::ImageDetailsViewModel>(m_viewModel)->Cancel();
    }
} // namespace winrt::Flense::implementation
