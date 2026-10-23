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

    namespace
    {
        winrt::hstring Describe(const ::Flense::Core::ArchiveError& error)
        {
            switch (error.Kind())
            {
            case ::Flense::Core::ArchiveErrorKind::Unreadable:
                return L"The provided image, or one of its layers, doesn't appear to be a valid archive. Make sure "
                       L"you've chosen a container image.";
            case ::Flense::Core::ArchiveErrorKind::Corrupt:
                return L"The provided image, or one of its layers, appears to be a corrupt archive. Try exporting the "
                       L"image again.";
            default:
                return L"The provided image, or one of its layers, couldn't be extracted.";
            }
        }

        winrt::hstring Describe(const ::Flense::Core::ImageFormatError& error)
        {
            switch (error.Kind())
            {
            case ::Flense::Core::ImageFormatErrorKind::NoManifest:
                return L"The provided file is a valid archive, but doesn't appear to be a container image. Make sure "
                       L"you've chosen a container image.";
            case ::Flense::Core::ImageFormatErrorKind::MissingConfiguration:
            case ::Flense::Core::ImageFormatErrorKind::MissingLayers:
                return L"The provided image is missing some of the files its metadata refers to. Try exporting the "
                       L"image again.";
            case ::Flense::Core::ImageFormatErrorKind::MalformedMetadata:
                return L"The provided image's metadata couldn't be parsed.";
            default:
                return L"The provided image couldn't be parsed.";
            }
        }

        winrt::hstring DescribeDetails(const ::Flense::Core::ImageReadError& error)
        {
            if (const std::optional<std::string>& entryPath = error.EntryPath())
            {
                return winrt::to_hstring(std::format("{}: {}", *entryPath, error.what()));
            }

            return winrt::to_hstring(error.what());
        }

        winrt::hstring DescribeDetails(const winrt::hresult_error& error)
        {
            return winrt::hstring{std::format(L"0x{:08X}: {}", static_cast<std::uint32_t>(error.code().value),
                                              std::wstring_view{error.message()})};
        }
    } // namespace

    IAsyncAction ImageDetailsViewModel::LoadAsync()
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue::GetForCurrentThread();

        auto cancellation = co_await get_cancellation_token();

        winrt::hstring failure;
        winrt::hstring failureDetails;

        try
        {
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
            }

            auto details = imageParser.Build(updateProgress, stopToken);

            auto parsedLayers = details.layers | std::views::transform([](const auto& layer) {
                                    return winrt::make<implementation::ImageLayerWrapper>(layer);
                                }) |
                                std::ranges::to<std::vector>();

            co_await wil::resume_foreground(dispatcher);

            Layers =
                winrt::single_threaded_observable_vector<winrt::Flense::ImageLayerWrapper>(std::move(parsedLayers));

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

            co_return;
        }
        catch (const winrt::hresult_canceled&)
        {
            // The page already handles cancellation by navigating away; it isn't a failure to report.
            throw;
        }
        catch (const ::Flense::Core::OperationCancelledError&)
        {
            co_return;
        }
        catch (const ::Flense::Core::ArchiveError& error)
        {
            failure = Describe(error);
            failureDetails = DescribeDetails(error);
        }
        catch (const ::Flense::Core::ImageFormatError& error)
        {
            failure = Describe(error);
            failureDetails = DescribeDetails(error);
        }
        catch (const winrt::hresult_error& error)
        {
            failure = L"The provided file couldn't be opened. It may have been moved, renamed or deleted since you "
                      L"picked it.";
            failureDetails = DescribeDetails(error);
        }
        catch (const std::exception& error)
        {
            failure = L"Something went wrong while reading the provided image.";
            failureDetails = winrt::to_hstring(error.what());
        }

        // A co_await can't appear in a handler, so the failure is carried out of the catch blocks and
        // reported here, back on the thread that owns the properties.
        co_await wil::resume_foreground(dispatcher);

        // IsLoading deliberately stays true: clearing it swaps in the content grid, and a frame of
        // empty layer list and empty tree can render before the page navigates away.
        ErrorDetails(failureDetails);
        ErrorMessage(failure);
    }
} // namespace winrt::Flense::implementation
