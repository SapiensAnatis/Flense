#pragma once

#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Windows.Foundation.h>

namespace winrt::Flense::implementation
{
    /// <summary>
    /// Serialises modal dialogs, so that every ContentDialog in the app goes through one place.
    /// </summary>
    /// <remarks>
    /// WinUI throws if a dialog is shown while another is already up, so this class is also useful
    /// for preventing unexpected exceptions.
    /// </remarks>
    class DialogService
    {
      public:
        static DialogService& Instance();

        /// <summary>
        /// Shows the provided dialog, unless another is already showing.
        /// </summary>
        /// <param name="dialog">The dialog to show.</param>
        /// <returns>The button the user chose, or nullptr if the request was blocked by an existing dialog.</returns>
        winrt::Windows::Foundation::IAsyncOperation<
            winrt::Windows::Foundation::IReference<winrt::Microsoft::UI::Xaml::Controls::ContentDialogResult>>
        ShowAsync(winrt::Microsoft::UI::Xaml::Controls::ContentDialog dialog);

      private:
        bool m_showing{false};
    };
} // namespace winrt::Flense::implementation
