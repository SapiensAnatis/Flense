#include "pch.h"

#include "DialogService.h"

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;
using namespace winrt::Windows::Foundation;

namespace winrt::Flense::implementation
{
    DialogService& DialogService::Instance()
    {
        static DialogService instance;
        return instance;
    }

    IAsyncOperation<IReference<Controls::ContentDialogResult>> DialogService::ShowAsync(Controls::ContentDialog dialog)
    {
        if (m_showing)
        {
            co_return nullptr;
        }

        m_showing = true;
        const auto reset = wil::scope_exit([this] { m_showing = false; });

        co_return co_await dialog.ShowAsync();
    }
} // namespace winrt::Flense::implementation
