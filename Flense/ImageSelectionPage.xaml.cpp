#include "pch.h"

#include "DialogService.h"
#include "ImageSelectionPage.xaml.h"
#if __has_include("ImageSelectionPage.g.cpp")
#include "ImageSelectionPage.g.cpp"
#endif

using namespace winrt;
using namespace winrt::Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Flense::implementation
{

    fire_and_forget ImageSelectionPage::OnNavigatedTo(const Navigation::NavigationEventArgs& /* e */)
    {
        auto lifetime = get_strong();

        winrt::hstring failure;

        try
        {
            m_loadAsyncAction = ViewModel().LoadImageList();
            co_await m_loadAsyncAction;
        }
        catch (const std::exception& ex)
        {
            failure = winrt::to_hstring(ex.what());
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

        if (!failure.empty())
        {
            Controls::ContentDialog dialog;
            dialog.XamlRoot(XamlRoot());
            dialog.Title(box_value(L"Couldn't load images"));
            dialog.Content(box_value(failure));
            dialog.CloseButtonText(L"OK");

            co_await DialogService::Instance().ShowAsync(dialog);
        }
    }
} // namespace winrt::Flense::implementation
