# Flense

Flense is a WinUI 3 app for analysing Docker images, written in C++/WinRT (not C#).

## Instructions

Before invoking build commands or skills, establish whether you are in a container. If you are in a container, the `DEVCONTAINER` 
environment variable will be set.

If you are not in a container, you can use the skills from `winui@win-dev-skills` to run the app for testing as required. You must make sure the 
app is launched from the build directory you are using (see below).

If you are in a container, you cannot run the app as there is no GUI stack, so your feedback loop is restricted to building the app.

You can build the app using the script at `.\Scripts\build-app.ps1`. Regardless of whether you are in a container or not, you should always pass -OutputDirectory C:\build to avoid conflicting with Visual Studio builds on the host.

For example:

```
.\Scripts\build-app.ps1 -OutputDirectory C:\build
.\Scripts\build-app.ps1 -Configuration Release -OutputDirectory C:\build
```

## Code style

1. Avoid adding comments, unless the code is unusual and benefits from an explanation of either _why_ it was written this way, or in exceptional 
   cases _what_ it does where the _what_ is difficult to parse.

## C++ modules

`Flense.Core` is one module split into partitions, laid out to keep rebuilds small:

1. `X.ixx` declares interface partition `:X`, which `Flense.Core.ixx` must `export import`.
2. `X.Impl.cpp` (or `X.Windows.cpp`) declares implementation partition `:X.Impl`, importing only what it uses.
3. Never write `module Flense.Core;` units: they depend on the whole interface, so rebuild on any change.
4. `Internal.X` partitions are for Core only. One imported by an interface partition must itself be an exported
   interface partition; one used only by implementations stays a non-interface partition (`:Internal.FilesystemParsing`).
5. MSVC rejects defining an exported free function in an implementation partition (C7619): define it in the interface
   partition or make it a static member.
6. `import std;` for the standard library; other headers go in the global module fragment.

## C++/WinRT best practices

1. Never call `InitializeComponent()` from a XAML type's constructor — C++/WinRT now calls it automatically and
   safely after construction; an explicit call can corrupt memory if it throws. Leave the constructor empty (see
   `MainWindow`).
2. To touch XAML properties during initialization, override `InitializeComponent()` instead: call the `...T::InitializeComponent()`
   base first, then your logic (`MainWindow` does this for `ExtendsContentIntoTitleBar`/`SetTitleBar`).
3. If a class derives from another class that also has markup (composable bases), inherit from `ComponentConnectorT`
   and call `ComponentConnectorT::InitializeComponent()` instead of `...T::InitializeComponent()`, since `Connect`/
   `GetBindingConnector` now dispatch to the most-derived override.