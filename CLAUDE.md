# Flense

Flense is a WinUI 3 app for analysing Docker images.

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