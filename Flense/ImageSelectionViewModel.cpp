#include "pch.h"

#include "DockerEngineImage.h"
#include "ImageSelectionViewModel.h"
#include "StringComparison.h"
#if __has_include("ImageSelectionViewModel.g.cpp")
#include "ImageSelectionViewModel.g.cpp"
#endif

#include <ranges>
#include <vector>

import Flense.Core.DockerClient;
import Flense.Core.Shared;

using namespace winrt;
using namespace winrt::Microsoft::UI::Dispatching;
using namespace winrt::Microsoft::UI::Xaml::Data;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;

namespace winrt::Flense::implementation
{
    IAsyncAction ImageSelectionViewModel::LoadImageList()
    {
        auto lifetime = get_strong();
        auto dispatcher = DispatcherQueue::GetForCurrentThread();

        IsLoading = true;

        auto cancellation = co_await get_cancellation_token();

        std::stop_source stopSource;
        // The callback can run after this frame's locals are destroyed, so it holds its own handle to the stop state.
        cancellation.callback([stopSource]() mutable { stopSource.request_stop(); });

        co_await winrt::resume_background();

        ::Flense::Core::DockerClient dockerClient;

        std::vector<::Flense::Core::DockerEngineImage> images;
        try
        {
            images = dockerClient.ListImages(stopSource.get_token());
        }
        catch (const ::Flense::Core::OperationCancelledError&)
        {
            throw winrt::hresult_canceled();
        }
        catch (const std::system_error& ex)
        {
            // The engine's named pipe only exists while the daemon is running.
            if (ex.code() == std::error_code(ERROR_FILE_NOT_FOUND, std::system_category()))
            {
                throw winrt::hresult_error(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
                                           L"Couldn't connect to Docker. Ensure Docker Desktop is running.");
            }

            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(ex.what()));
        }
        catch (const std::exception& ex)
        {
            // WIL's C++/WinRT integration turns a std::exception escaping a coroutine into a generic
            // ERROR_UNHANDLED_EXCEPTION and discards its message, so convert it while we still have the text.
            throw winrt::hresult_error(E_FAIL, winrt::to_hstring(ex.what()));
        }

        auto localAllImages = images | std::views::transform([](const auto& image) {
                                  return winrt::make<implementation::DockerEngineImage>(image);
                              }) |
                              std::ranges::to<std::vector>();

        co_await wil::resume_foreground(dispatcher);

        // Threading discipline: only read/write to member variables from UI thread
        m_allImages = std::move(localAllImages);

        ApplySearchFilter();

        HasLoaded = true;
        IsLoading = false;
    }

    void ImageSelectionViewModel::SelectImage(const winrt::Flense::DockerEngineImage& image)
    {
        (void)image;
    }

    winrt::hstring ImageSelectionViewModel::SearchQuery()
    {
        return m_searchQuery;
    }

    void ImageSelectionViewModel::SearchQuery(const winrt::hstring& value)
    {
        if (m_searchQuery != value)
        {
            m_searchQuery = value;
            RaisePropertyChanged(L"SearchQuery");
            m_searchDebouncer.Debounce();
        }
    }

    void ImageSelectionViewModel::ApplySearchFilter()
    {
        auto filteredImages = m_allImages | std::views::filter([this](const winrt::Flense::DockerEngineImage& image) {
                                  return ContainsCaseInsensitive(image.RepoTag(), m_searchQuery);
                              }) |
                              std::ranges::to<std::vector>();
        Images.ReplaceAll(filteredImages);
    }
} // namespace winrt::Flense::implementation
