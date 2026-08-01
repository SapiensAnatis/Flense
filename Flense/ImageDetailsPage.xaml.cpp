#include "pch.h"

#include "ImageDetailsPage.xaml.h"
#if __has_include("ImageDetailsPage.g.cpp")
#include "ImageDetailsPage.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Windows::Storage;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Navigation;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    StorageFile ImageDetailsPage::ImageFile()
    {
        return m_imageFile;
    }

    void ImageDetailsPage::ImageFile(StorageFile const& value)
    {
        m_imageFile = value;
    }

    void ImageDetailsPage::OnNavigatedTo(NavigationEventArgs const& e)
    {
        if (auto imageFile = e.Parameter().try_as<StorageFile>())
        {
            ImageFile(imageFile);
            MessageTextBlock().Text(L"I am going to open file at " + m_imageFile.Path());
        }
        else
        {
            MessageTextBlock().Text(L"Invalid parameter passed to ImageDetailsPage.");
        }
    }
} // namespace winrt::Flense::implementation
