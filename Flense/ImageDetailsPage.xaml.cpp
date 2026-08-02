#include "pch.h"

#include "ImageDetailsPage.xaml.h"
#if __has_include("ImageDetailsPage.g.cpp")
#include "ImageDetailsPage.g.cpp"
#endif

#include "ArchiveReader.h"
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
    Flense::ImageDetailsViewModel ImageDetailsPage::ViewModel()
    {
        return m_viewModel;
    }

    void ImageDetailsPage::OnNavigatedTo(NavigationEventArgs const& e)
    {
        if (auto imageFile = e.Parameter().try_as<StorageFile>())
        {
            m_viewModel.ImageFile(imageFile);
            m_viewModel.FileName(imageFile.Name());
            MessageTextBlock().Text(L"Loading image: " + imageFile.Name());
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

        auto rawStream = co_await m_viewModel.ImageFile().OpenReadAsync();
        WinRtByteStream stream{rawStream};

        LoadingProgressBar().Visibility(Visibility::Visible);
        LoadingProgressBar().Value(0);

        std::stop_source stopSource;

        ::Flense::Core::ArchiveReader reader;

        co_await winrt::resume_background();

        reader.ProcessArchive(
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

        co_await wil::resume_foreground(dispatcher);

        LoadingProgressBar().Visibility(Visibility::Collapsed);
    }
} // namespace winrt::Flense::implementation
