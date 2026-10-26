#include "pch.h"

#include "ImageDetailsViewModel.h"
#if __has_include("ImageDetailsViewModel.g.cpp")
#include "ImageDetailsViewModel.g.cpp"
#endif

#include "ImageLayerWrapper.h"
#include "TitleBarService.h"

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

        auto updateProgress = [dispatcher, weak = get_weak()](const std::uint64_t bytesProcessed,
                                                              const std::uint64_t totalBytes) {
            if (totalBytes == 0)
            {
                return;
            }

            // Reserve the last 5% for building the image once every byte has been read.
            const double percent = (static_cast<double>(bytesProcessed) / static_cast<double>(totalBytes)) * 95.0;

            dispatcher.TryEnqueue([weak, percent] {
                if (auto self = weak.get())
                {
                    if (percent > self->LoadingProgress)
                    {
                        self->LoadingProgress = percent;
                    }
                }
            });
        };

        const std::filesystem::path archivePath{std::wstring_view{ImageArchivePath()}};

        LoadingProgress = 0;
        IsLoading = true;

        std::stop_source stopSource;
        cancellation.callback([&stopSource] { stopSource.request_stop(); });

        co_await winrt::resume_background();

        winrt::hstring failure;
        winrt::hstring failureDetails;

        try
        {
            auto details = ::Flense::Core::ParseImageArchive(archivePath, updateProgress, stopSource.get_token());

            auto parsedLayers = details.layers | std::views::transform([](const auto& layer) {
                                    return winrt::make<implementation::ImageLayerWrapper>(layer);
                                }) |
                                std::ranges::to<std::vector>();

            co_await wil::resume_foreground(dispatcher);

            Layers =
                winrt::single_threaded_observable_vector<winrt::Flense::ImageLayerWrapper>(std::move(parsedLayers));

            const winrt::hstring name =
                details.repoTag.transform([](const std::string& value) { return winrt::to_hstring(value); })
                    .value_or(winrt::hstring{archivePath.filename().wstring()});

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
        catch (const std::filesystem::filesystem_error& error)
        {
            failure = L"The provided file couldn't be read. It may have been moved, renamed or deleted since you "
                      L"picked it.";
            failureDetails = winrt::to_hstring(error.what());
        }
        catch (const winrt::hresult_error& error)
        {
            failure = L"Something went wrong while reading the provided image.";
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
