#pragma once

#include "LoadFailureDialog.g.h"

namespace winrt::Flense::implementation
{
    struct LoadFailureDialog : LoadFailureDialogT<LoadFailureDialog>
    {
        LoadFailureDialog()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }

        winrt::hstring Message() const;
        void Message(const winrt::hstring& value);

        winrt::hstring Details() const;
        void Details(const winrt::hstring& value);

        bool HasDetails() const;

      private:
        winrt::hstring m_message;
        winrt::hstring m_details;
    };
} // namespace winrt::Flense::implementation

namespace winrt::Flense::factory_implementation
{
    struct LoadFailureDialog : LoadFailureDialogT<LoadFailureDialog, implementation::LoadFailureDialog>
    {
    };
} // namespace winrt::Flense::factory_implementation
