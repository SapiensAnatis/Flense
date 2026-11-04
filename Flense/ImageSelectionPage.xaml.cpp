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
        else if (!failure.empty())
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

    void ImageSelectionPage::SelectImage(const winrt::Flense::DockerEngineImage& image)
    {
        Frame().Navigate(xaml_typename<Flense::ImageDetailsPage>(), box_value(image));
    }

    void ImageSelectionPage::ImageList_ItemInvoked(const Controls::ItemsView& /* sender */,
                                                   const Controls::ItemsViewItemInvokedEventArgs& args)
    {
        SelectImage(args.InvokedItem().as<Flense::DockerEngineImage>());
    }

    void ImageSelectionPage::ImageList_SelectionChanged(const Controls::ItemsView& sender,
                                                        const Controls::ItemsViewSelectionChangedEventArgs& /* args */)
    {
        // This can be called mid-load in which case OpenImageButton returns nullptr and the below would crash
        if (const auto button = OpenImageButton())
        {
            button.IsEnabled(sender.SelectedItem() != nullptr);
        }
    }

    void ImageSelectionPage::OpenImageButton_Loaded(const winrt::Windows::Foundation::IInspectable& /* sender */,
                                                    const RoutedEventArgs& /* e */)
    {
        OpenImageButton().IsEnabled(ImageList().SelectedItem() != nullptr);
    }

    void ImageSelectionPage::OpenImageButton_Click(const winrt::Windows::Foundation::IInspectable& /* sender */,
                                                   const RoutedEventArgs& /* e */)
    {
        if (const auto selectedItem = ImageList().SelectedItem())
        {
            SelectImage(selectedItem.as<Flense::DockerEngineImage>());
        }
    }

    void ImageSelectionPage::CancelButton_Click(const winrt::Windows::Foundation::IInspectable& /* sender */,
                                                const RoutedEventArgs& /* e */)
    {
        if (Frame().CanGoBack())
        {
            Frame().GoBack();
        }
    }
} // namespace winrt::Flense::implementation
