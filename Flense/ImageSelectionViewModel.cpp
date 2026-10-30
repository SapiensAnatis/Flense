#include "pch.h"

#include "DockerEngineImage.h"
#include "ImageSelectionViewModel.h"
#if __has_include("ImageSelectionViewModel.g.cpp")
#include "ImageSelectionViewModel.g.cpp"
#endif

#include <ranges>
#include <vector>

import Flense.Core;

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

        auto images = dockerClient.ListImages();

        auto winrtImages = images | std::views::transform([](const auto& image) {
                               return winrt::make<implementation::DockerEngineImage>(image);
                           }) |
                           std::ranges::to<std::vector>();

        co_await wil::resume_foreground(dispatcher);

        Images = winrt::single_threaded_observable_vector<winrt::Flense::DockerEngineImage>(std::move(winrtImages));
    }
} // namespace winrt::Flense::implementation
