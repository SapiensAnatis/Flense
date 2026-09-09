#include "pch.h"

#include "ImageDetailsViewModel.h"
#if __has_include("ImageDetailsViewModel.g.cpp")
#include "ImageDetailsViewModel.g.cpp"
#endif

#include "ImageLayerWrapper.h"
#include "TitleBarService.h"
#include "WinRtByteStream.h"

#if defined(__INTELLISENSE__)
#include <ranges>
#endif

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

    IAsyncAction ImageDetailsViewModel::LoadAsync()
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue::GetForCurrentThread();

        auto cancellation = co_await get_cancellation_token();

        auto archive = ImageArchive();
        auto rawStream = co_await archive.OpenReadAsync();

        WinRtByteStream stream{rawStream};

        const std::uint64_t totalBytes = stream.Size();

        LoadingProgress = 0;
        IsLoading = true;

        std::stop_source stopSource;
        cancellation.callback([&stopSource] { stopSource.request_stop(); });

        co_await winrt::resume_background();

        const std::stop_token stopToken = stopSource.get_token();

        auto reader = ::Flense::Core::ArchiveReader::CreateFromStream(stream);

        ::Flense::Core::ImageParser imageParser;

        auto updateProgress = [dispatcher, weak = get_weak(), totalBytes](const std::uint64_t bytesProcessed) {
            if (totalBytes == 0)
            {
                return;
            }

            const double percent = (static_cast<double>(bytesProcessed) / static_cast<double>(totalBytes)) * 100.0;

            dispatcher.TryEnqueue([weak, percent] {
                if (auto self = weak.get())
                {
                    self->LoadingProgress = percent;
                }
            });
        };

        while (auto entry = reader.Next(stopToken))
        {
            imageParser.ProcessEntry(*entry, stopToken);

            updateProgress(imageParser.BytesProcessed());

            if (stopToken.stop_requested())
            {
                co_return;
            }
        }

        if (stopToken.stop_requested())
        {
            co_return;
        }

        auto details = imageParser.Build(updateProgress, stopToken);

        if (stopToken.stop_requested())
        {
            co_return;
        }

        auto parsedLayers = details.layers | std::views::transform([](const auto& layer) {
                                return winrt::make<implementation::ImageLayerWrapper>(layer);
                            }) |
                            std::ranges::to<std::vector>();

        co_await wil::resume_foreground(dispatcher);

        Layers = winrt::single_threaded_observable_vector<winrt::Flense::ImageLayerWrapper>(std::move(parsedLayers));

        const winrt::hstring name =
            details.repoTag.transform([](const std::string& value) { return winrt::to_hstring(value); })
                .value_or(archive.Name());

        TitleBarService::Instance().Title(name + L" - Flense");

        if (Layers().Size() > 0)
        {
            SelectedLayer(Layers().GetAt(0));
        }

        LoadingProgress(100);

        IsLoading(false);
    }
} // namespace winrt::Flense::implementation
