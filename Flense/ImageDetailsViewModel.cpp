#include "pch.h"

#include "ImageDetailsViewModel.h"
#if __has_include("ImageDetailsViewModel.g.cpp")
#include "ImageDetailsViewModel.g.cpp"
#endif

#include "ArchiveReader.h"
#include "ImageLayerWrapper.h"
#include "ImageParser.h"
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

    void ImageDetailsViewModel::ImageArchive(const StorageFile& value)
    {
        if (m_imageFile != value)
        {
            m_imageFile = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"ImageArchive"});
        }
    }

    IObservableVector<winrt::Flense::ImageLayerWrapper> ImageDetailsViewModel::Layers()
    {
        return m_layers;
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

        co_await winrt::resume_background();

        auto reader = ::Flense::Core::ArchiveReader::CreateFromStream(stream, stopSource.get_token());

        ::Flense::Core::ImageParser imageParser;

        double lastReportedPercent = -1.0;
        while (auto entry = reader.Next())
        {
            imageParser.ProcessEntry(*entry);

            const double percent = (static_cast<double>(stream.Position()) / static_cast<double>(stream.Size())) * 50.0;

            // Only report on meaningful (>=5%) changes - TryEnqueue marshals to the UI thread,
            // and posting on every entry (there can be thousands in a large archive) can flood it
            // badly enough to look and behave like a hang.
            if (percent - lastReportedPercent >= 5.0)
            {
                lastReportedPercent = percent;
                dispatcher.TryEnqueue([weak = get_weak(), percent] {
                    if (auto self = weak.get())
                    {
                        self->LoadingProgress(percent);
                    }
                });
            }
        }

        co_await wil::resume_foreground(dispatcher);

        for (const auto& layer : imageParser.Build())
        {
            Layers().Append(winrt::make<implementation::ImageLayerWrapper>(layer));
        }

        LoadingProgress(100);

        IsLoading(false);
    }

    event_token ImageDetailsViewModel::PropertyChanged(const PropertyChangedEventHandler& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void ImageDetailsViewModel::PropertyChanged(const event_token& token) noexcept
    {
        m_propertyChanged.remove(token);
    }
} // namespace winrt::Flense::implementation
