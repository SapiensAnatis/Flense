#pragma once
#include "pch.h"          // needed because this is /FI-injected into XAML-generated files

import winrt.Windows.Foundation;
import winrt.Windows.Foundation.Collections;
import winrt.Windows.Storage;
import winrt.Windows.Storage.Streams;
import winrt.Windows.Storage.Pickers;
import winrt.Windows.ApplicationModel.Activation;
import winrt.Windows.UI.Xaml.Interop;
import winrt.Microsoft.UI;
import winrt.Microsoft.UI.Composition;
import winrt.Microsoft.UI.Composition.SystemBackdrops;
import winrt.Microsoft.UI.Content;
import winrt.Microsoft.UI.Windowing;
import winrt.Microsoft.UI.Dispatching;
import winrt.Microsoft.UI.Xaml;
import winrt.Microsoft.UI.Xaml.Controls;
import winrt.Microsoft.UI.Xaml.Controls.Primitives;
import winrt.Microsoft.UI.Xaml.Data;
import winrt.Microsoft.UI.Xaml.Interop;
import winrt.Microsoft.UI.Xaml.Markup;
import winrt.Microsoft.UI.Xaml.Media;
import winrt.Microsoft.UI.Xaml.Navigation;
import winrt.Microsoft.UI.Xaml.Shapes;
import winrt.Microsoft.UI.Xaml.XamlTypeInfo;
import winrt.Flense;

#define WINRT_IMPORT_MODULE
// Re-light WIL's cppwinrt support: including the now-inert winrt headers
// defines their include guards, which is what wil/cppwinrt_helpers.h keys off.
// wil/cppwinrt.h itself is deliberately not included here: it pulls in
// <inspectable.h>, whose global ::IInspectable is ambiguous with
// winrt::Windows::Foundation::IInspectable wherever that namespace is
// opened with a using-directive. Only cppwinrt_helpers.h is actually
// needed for wil::resume_foreground.
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <wil/cppwinrt_helpers.h>

// Microsoft.UI.Interop.h is a hand-written WindowsAppSDK header, not a cppwinrt
// projection - it has no backing WinMD namespace, so it has no module of its own.
// WINRT_IMPORT_MODULE still makes its internal winrt/Microsoft.UI.h include inert.
#include <winrt/Microsoft.UI.Interop.h>
