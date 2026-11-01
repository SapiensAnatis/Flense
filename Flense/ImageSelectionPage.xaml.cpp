#include "pch.h"

#include "DialogService.h"
#include "ImageSelectionPage.xaml.h"
#if __has_include("ImageSelectionPage.g.cpp")
#include "ImageSelectionPage.g.cpp"
#endif

import std;

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{
    fire_and_forget ImageSelectionPage::OnNavigatedTo(const Navigation::NavigationEventArgs& /* e */)
    {
        m_navigatedAway = false;

        auto lifetime = get_strong();

        m_loadingDebounceTimer = DispatcherQueue().CreateTimer();
        m_loadingDebounceTimer.Interval(std::chrono::milliseconds{250});
        m_loadingDebounceTimer.IsRepeating(false);
        m_loadingDebounceTimer.Tick([weak = get_weak()](const auto&, const auto&) {
            if (auto self = weak.get())
            {
                self->ShowLoading(self->m_loading);
            }
        });

        SetLoading(true);

        winrt::hstring failure;
        bool cancelled = false;

        try
        {
            m_loadAsyncAction = ViewModel().LoadImageList();
            co_await m_loadAsyncAction;
        }
        catch (const winrt::hresult_canceled&)
        {
            cancelled = true;
        }
        catch (const winrt::hresult_error& e)
        {
            failure = e.message();
        }
        catch (...)
        {
            failure = L"An unknown error occurred while loading images.";
        }

        m_loadAsyncAction = nullptr;

        if (m_navigatedAway)
        {
            co_return;
        }

        if (cancelled)
        {
            if (Frame().CanGoBack())
            {
                Frame().GoBack();
            }
        }
        else if (failure.empty())
        {
            SetLoading(false);
        }
        else
        {
            Controls::ContentDialog dialog;
            dialog.XamlRoot(Frame().XamlRoot());
            dialog.Title(box_value(L"Couldn't load images"));
            dialog.Content(box_value(failure));
            dialog.CloseButtonText(L"OK");

            const auto shown = co_await DialogService::Instance().ShowAsync(dialog) != nullptr;

            if (shown && Frame().CanGoBack())
            {
                Frame().GoBack();
            }
        }
    }

    void ImageSelectionPage::OnNavigatedFrom(const Navigation::NavigationEventArgs& /* e */)
    {
        m_navigatedAway = true;
        m_loadingDebounceTimer.Stop();
        CancelLoading();
    }

    void ImageSelectionPage::CancelLoadingButton_Click(const IInspectable& /* sender */, const RoutedEventArgs& /* e */)
    {
        CancelLoading();
    }

    void ImageSelectionPage::CancelLoading()
    {
        if (m_loadAsyncAction)
        {
            m_loadAsyncAction.Cancel();
        }
    }

    // The loading panel only changes once the loading state has differed from it for the debounce interval, so a
    // quick load never shows it and a slow one never flashes it away.
    void ImageSelectionPage::SetLoading(const bool loading)
    {
        m_loading = loading;
        m_loadingDebounceTimer.Stop();

        if (m_loading != m_showingLoading)
        {
            m_loadingDebounceTimer.Start();
        }
    }

    void ImageSelectionPage::ShowLoading(const bool loading)
    {
        m_showingLoading = loading;
        LoadingPanel().Visibility(loading ? Visibility::Visible : Visibility::Collapsed);
        ImageList().Visibility(loading ? Visibility::Collapsed : Visibility::Visible);
    }
} // namespace winrt::Flense::implementation
