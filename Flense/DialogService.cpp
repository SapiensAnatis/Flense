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

        auto window = m_window.get();
        if (!window)
        {
            throw winrt::hresult_error(
                E_FAIL, L"DialogService::SetWindow must be called before showing DialogService::ShowAsync");
        }

        dialog.XamlRoot(window.Content().XamlRoot());

        m_showing = true;
        const auto reset = wil::scope_exit([this] { m_showing = false; });

        co_return co_await dialog.ShowAsync();
    }

    void DialogService::SetWindow(const Window& window)
    {
        m_window = window;
    }

} // namespace winrt::Flense::implementation
