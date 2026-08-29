# Flense

Flense is a WinUI 3 app for analysing Docker images, written in C++/WinRT (not C#).

## Instructions

You are running in a container, so running the app is not possible as there is no GUI stack. However, you can validate your changes using 
build-app.ps1 in the repository root.

You should always pass -OutputDirectory C:\build to avoid conflicting with Visual Studio builds on the host.

For example:

```
.\Scripts\build-app.ps1 -OutputDirectory C:\build
.\Scripts\build-app.ps1 -Configuration Release -OutputDirectory C:\build
```

## Code style

1. Avoid adding comments, unless the code is unusual and benefits from an explanation of either _why_ it was written this way, or in exceptional 
   cases _what_ it does where the _what_ is difficult to parse.

## C++/WinRT best practices

1. Never call `InitializeComponent()` from a XAML type's constructor — C++/WinRT now calls it automatically and
   safely after construction; an explicit call can corrupt memory if it throws. Leave the constructor empty (see
   `MainWindow`).
2. To touch XAML properties during initialization, override `InitializeComponent()` instead: call the `...T::InitializeComponent()`
   base first, then your logic (`MainWindow` does this for `ExtendsContentIntoTitleBar`/`SetTitleBar`).
3. If a class derives from another class that also has markup (composable bases), inherit from `ComponentConnectorT`
   and call `ComponentConnectorT::InitializeComponent()` instead of `...T::InitializeComponent()`, since `Connect`/
   `GetBindingConnector` now dispatch to the most-derived override.