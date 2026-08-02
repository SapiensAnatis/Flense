#include "pch.h"

#include "ImageDetailsViewModel.h"
#if __has_include("ImageDetailsViewModel.g.cpp")
#include "ImageDetailsViewModel.g.cpp"
#endif

#include "ArchiveReader.h"
#include "WinRtByteStream.h"

#include <stop_token>

using namespace winrt;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Xaml::Data;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;

namespace winrt::Flense::implementation
{
    StorageFile ImageDetailsViewModel::ImageArchive()
    {
        return m_imageFile;
    }

    void ImageDetailsViewModel::ImageArchive(StorageFile const& value)
    {
        if (m_imageFile != value)
        {
            m_imageFile = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"ImageArchive"});
        }
    }

    IObservableVector<hstring> ImageDetailsViewModel::Filenames()
    {
        return m_filenames;
    }

    bool ImageDetailsViewModel::IsLoading()
    {
        return m_isLoading;
    }

    void ImageDetailsViewModel::IsLoading(bool value)
    {
        if (m_isLoading != value)
        {
            m_isLoading = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"IsLoading"});
            m_propertyChanged(*this, PropertyChangedEventArgs{L"IsLoaded"});
        }
    }

    bool ImageDetailsViewModel::IsLoaded()
    {
        return !m_isLoading;
    }

    double ImageDetailsViewModel::LoadingProgress()
    {
        return m_loadingProgress;
    }

    void ImageDetailsViewModel::LoadingProgress(double value)
    {
        if (m_loadingProgress != value)
        {
            m_loadingProgress = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"LoadingProgress"});
        }
    }

    IAsyncAction ImageDetailsViewModel::LoadAsync()
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue::GetForCurrentThread();

        auto archive = m_imageFile;
        auto rawStream = co_await archive.OpenReadAsync();

        WinRtByteStream stream{rawStream};

        LoadingProgress(0);
        IsLoading(true);

        std::stop_source stopSource;

        ::Flense::Core::ArchiveReader reader;

        co_await winrt::resume_background();

        auto utf8Filenames = reader.ProcessArchive(
            stream,
            [dispatcher, weak = get_weak()](double percent) {
                dispatcher.TryEnqueue([weak, percent] {
                    if (auto self = weak.get())
                    {
                        self->LoadingProgress(percent);
                    }
                });
            },
            stopSource.get_token());

        co_await wil::resume_foreground(dispatcher);

        LoadingProgress(100);

        for (const auto& str : utf8Filenames)
        {
            Filenames().Append(winrt::to_hstring(str));
        }

        IsLoading(false);
    }

    event_token ImageDetailsViewModel::PropertyChanged(PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void ImageDetailsViewModel::PropertyChanged(event_token const& token) noexcept
    {
        m_propertyChanged.remove(token);
    }
} // namespace winrt::Flense::implementation
