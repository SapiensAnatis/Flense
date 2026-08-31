#include "pch.h"

#include "ImageDetailsViewModel.h"
#if __has_include("ImageDetailsViewModel.g.cpp")
#include "ImageDetailsViewModel.g.cpp"
#endif

#include "ImageLayerWrapper.h"
#include "TitleBarService.h"
#include "WinRtByteStream.h"

import Flense.Core;
import std;

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

    void ImageDetailsViewModel::Layers(IObservableVector<winrt::Flense::ImageLayerWrapper> value)
    {
        if (m_layers != value)
        {
            m_layers = std::move(value);
            m_propertyChanged(*this, PropertyChangedEventArgs{L"Layers"});
        }
    }

    winrt::Flense::ImageLayerWrapper ImageDetailsViewModel::SelectedLayer()
    {
        return m_selectedLayer;
    }

    void ImageDetailsViewModel::SelectedLayer(const winrt::Flense::ImageLayerWrapper& value)
    {
        if (m_selectedLayer != value)
        {
            if (m_selectedLayer)
            {
                get_self<ImageLayerWrapper>(m_selectedLayer)->UnloadTree();
            }

            m_selectedLayer = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"SelectedLayer"});
        }
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

    hstring ImageDetailsViewModel::StatusMessage()
    {
        return m_statusMessage;
    }

    void ImageDetailsViewModel::StatusMessage(const hstring& value)
    {
        if (m_statusMessage != value)
        {
            m_statusMessage = value;
            m_propertyChanged(*this, PropertyChangedEventArgs{L"StatusMessage"});
        }
    }

    IAsyncAction ImageDetailsViewModel::LoadAsync()
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue::GetForCurrentThread();

        auto cancellation = co_await get_cancellation_token();

        auto archive = m_imageFile;
        auto rawStream = co_await archive.OpenReadAsync();

        WinRtByteStream stream{rawStream};

        LoadingProgress(0);
        IsLoading(true);

        // Bridge the coroutine's cancellation into a stop_token, which is what Flense.Core reads. Without this the
        // only cancellation check would be the one between top-level entries below, and a multi-gigabyte layer blob
        // would have to be parsed to completion before we noticed.
        std::stop_source stopSource;
        cancellation.callback([&stopSource] { stopSource.request_stop(); });

        co_await winrt::resume_background();

        const std::stop_token stopToken = stopSource.get_token();

        auto reader = ::Flense::Core::ArchiveReader::CreateFromStream(stream);

        ::Flense::Core::ImageParser imageParser;

        double lastReportedPercent = 0;
        while (auto entry = reader.Next(stopToken))
        {
            imageParser.ProcessEntry(*entry, stopToken);

            if (stopToken.stop_requested())
            {
                co_return;
            }

            const double percent = (static_cast<double>(stream.Position()) / static_cast<double>(stream.Size())) * 90.0;

            // Don't flood the UI with updates
            if (percent - lastReportedPercent >= 1.0)
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

        if (stopToken.stop_requested())
        {
            co_return;
        }

        auto details = imageParser.Build();

        auto parsedLayers = details.layers | std::views::transform([](const auto& layer) {
                                return winrt::make<implementation::ImageLayerWrapper>(layer);
                            }) |
                            std::ranges::to<std::vector>();

        co_await wil::resume_foreground(dispatcher);

        Layers(winrt::single_threaded_observable_vector<winrt::Flense::ImageLayerWrapper>(std::move(parsedLayers)));

        const winrt::hstring name =
            details.repoTag.transform([](const std::string& value) { return winrt::to_hstring(value); })
                .value_or(m_imageFile.Name());

        TitleBarService::Instance().Title(name + L" - Flense");

        if (m_layers.Size() > 0)
        {
            SelectedLayer(m_layers.GetAt(0));
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
