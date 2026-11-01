#include "pch.h"

#include "DockerEngineImage.h"
#include "ImageSelectionViewModel.h"
#if __has_include("ImageSelectionViewModel.g.cpp")
#include "ImageSelectionViewModel.g.cpp"
#endif

#include <ranges>
#include <vector>

import Flense.Core.DockerClient;

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

        co_await winrt::resume_background();

        ::Flense::Core::DockerClient dockerClient;

        std::vector<::Flense::Core::DockerEngineImage> images;
        try
        {
            images = dockerClient.ListImages();
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

        auto winrtImages = images | std::views::transform([](const auto& image) {
                               return winrt::make<implementation::DockerEngineImage>(image);
                           }) |
                           std::ranges::to<std::vector>();

        co_await wil::resume_foreground(dispatcher);

        Images = winrt::single_threaded_observable_vector<winrt::Flense::DockerEngineImage>(std::move(winrtImages));
    }
} // namespace winrt::Flense::implementation
