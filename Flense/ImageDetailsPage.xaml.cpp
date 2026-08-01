#include "pch.h"

#include "ImageDetailsPage.xaml.h"
#if __has_include("ImageDetailsPage.g.cpp")
#include "ImageDetailsPage.g.cpp"
#endif

#include "ProcessArchive.h"
#include "WinRtByteStream.h"

#include <stop_token>

using namespace winrt;
using namespace winrt::Windows::Foundation;
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
            MessageTextBlock().Text(L"Loading image: " + m_imageFile.Name());
            ProcessFileAsync();
        }
        else
        {
            MessageTextBlock().Text(L"Invalid parameter passed to ImageDetailsPage.");
        }
    }

    fire_and_forget ImageDetailsPage::ProcessFileAsync()
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue();

        auto rawStream = co_await m_imageFile.OpenReadAsync();
        WinRtByteStream stream{rawStream};

        LoadingProgressBar().Visibility(Visibility::Visible);
        LoadingProgressBar().Value(0);

        std::stop_source stopSource;

        co_await ::Flense::Core::ProcessArchive<IAsyncAction>(
            stream,
            [dispatcher, weak = get_weak()](double percent) {
                dispatcher.TryEnqueue([weak, percent] {
                    if (auto self = weak.get())
                    {
                        self->LoadingProgressBar().Value(percent);
                    }
                });
            },
            stopSource.get_token());

        LoadingProgressBar().Visibility(Visibility::Collapsed);
    }
} // namespace winrt::Flense::implementation
