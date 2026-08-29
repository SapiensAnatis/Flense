# Migrating Flense to C++20 Modules

## Context

Can Flense move from textual `#include` of the C++/WinRT projection to C++20 named modules
(`import winrt.Microsoft.UI.Xaml;`), given it is a WinUI 3 app?

**Yes, and this repo is unusually well positioned for it.** C++/WinRT 3.0 ships first-class module
support (`CppWinRTBuildModule`), and the NuGet package carries a migration guide —
`packages/Microsoft.Windows.CppWinRT.3.0.260715.1/modules.md` — that was explicitly hardened against a
WinUI/XAML codebase (Windows Terminal, ~250 files, 15+ projects). Every prerequisite it lists is already
satisfied here:

| Prerequisite | Flense today |
|---|---|
| MSVC v145 toolset (VS 2026) | `v145` in both vcxproj; container has MSVC 14.51.36231 |
| C++/WinRT 3.x with module support | `Microsoft.Windows.CppWinRT` 3.0.260818.1 |
| `/std:c++20` or later | `stdcpp23` in `Directory.Build.props` |
| `import std;` support | `std.ixx` present in the MSVC toolset |

The honest caveat up front: **the payoff at Flense's size is modest.** The guide's benefits (shared IFCs
across projects, smaller intermediates) mostly accrue to large multi-project solutions. Flense is ~20
translation units in one WinRT project plus a small non-WinRT static lib, and the existing PCH already
amortises the expensive winrt parsing. Treat this as a modernisation/future-proofing exercise with a nice
side-effect (no macro leakage into TUs), not a build-time win. The work is contained and cleanly
revertible.

## Scope

Two projects, only one of which is affected:

- **`Flense/`** — the WinUI 3 app (`Application`, `CppWinRTOptimized=true`,
  `CppWinRTRootNamespaceAutoMerge=true`). 14 IDL-backed runtime classes, 5 XAML pages, ~20 `.cpp`. This is
  the whole job.
- **`Flense.Core/`** — static library, **zero winrt usage** (`Flense.Core/pch.h` contains only
  `#define NOMINMAX`). Nothing to do for winrt modules. It's a separate, optional `import std;` exercise
  (Phase 6).

`Flense.Benchmarks/` contains only stale build artefacts, no sources, and isn't in `Flense.slnx`. Ignore it.

## Recommended architecture: single-project, no module builder

`modules.md` recommends a dedicated **module builder** static library for multi-project solutions.
**Skip it here.** Only `Flense` consumes WinRT, so there is nothing to share; a builder project would add a
vcxproj, a solution `BuildDependency`, and a class of `CppWinRTConsumeModule` / ambiguous-IFC (C7684)
problems for no benefit. Use the guide's "Quick Start — Single Project" path: `CppWinRTBuildModule=true` on
`Flense` alone, which generates `.ixx` for the platform, reference *and* component projections.

## Phases

### Phase 0 — Baseline

Record a clean build time and confirm green before touching anything:

```
.\Scripts\build-app.ps1 -OutputDirectory C:\build
```

Do this on a **clean** output directory (stale IFCs are the single biggest source of confusing errors
later — the guide is emphatic about this).

### Phase 0.5 — Fix `IntDir` before anything else (blocking prerequisite)

`Directory.Build.props:17` sets, when `FlenseOutputDirectory` is passed:

```xml
<IntDir>$(FlenseOutputDirectory)\$(MSBuildProjectName)\obj\</IntDir>
```

That path has **no `$(Configuration)`/`$(Platform)` component**, so Debug and Release currently share one
intermediate folder. Today that's merely wasteful; with modules it's a correctness problem — the CppWinRT
targets export `$(IntDir)` as the IFC directory (`CppWinRTGetModuleOutputs`) and MSBuild writes `.ifc`
files there, so Debug and Release IFCs would overwrite each other, and the guide is explicit that mixing
Debug/Release module IFCs produces mismatched codegen and iterator-debug levels. Fix first:

```xml
<BaseIntermediateOutputPath>$(FlenseOutputDirectory)\$(MSBuildProjectName)\obj\$(Platform)\$(Configuration)\</BaseIntermediateOutputPath>
<IntDir>$(FlenseOutputDirectory)\$(MSBuildProjectName)\obj\$(Platform)\$(Configuration)\</IntDir>
<GeneratedFilesDir>$(FlenseOutputDirectory)\$(MSBuildProjectName)\Generated Files\$(Platform)\$(Configuration)\</GeneratedFilesDir>
```

`GeneratedFilesDir` needs splitting too — the `.ixx` files are generated there and globbed into `ClCompile`
by `CppWinRTAddModuleInterfaces`, and the Debug-only `DeveloperMenuBarItem` type means the two
configurations genuinely generate different sets. Verify a Debug-then-Release build pair still works before
moving on. (Host VS builds don't set `FlenseOutputDirectory` and already get per-config `IntDir` from the
default, so this only affects container builds.)

### Phase 1 — Turn on module generation, change no code

In `Flense/Flense.vcxproj`, add to the `Label="Globals"` PropertyGroup:

```xml
<CppWinRTBuildModule>true</CppWinRTBuildModule>
```

and in the global `ItemDefinitionGroup`'s `ClCompile`:

```xml
<BuildStlModules>true</BuildStlModules>
```

(`BuildStlModules` is required because the generated `.ixx` files themselves use `import std;`.)

Deliberately **do not** set `CppWinRTModuleInclude`/`Exclude` yet. Generating every reachable namespace
guarantees the transitive closure is satisfied; filtering is a Phase 5 optimisation. Build. At this point
nothing imports anything — you're just proving the `.ixx` generation and IFC compilation step works against
the WinUI/WindowsAppSDK 2.4.0 winmds. Expect this to add roughly a minute to a clean build.

### Phase 2 — Strip the PCH, add `ModulePreamble.h`

`Flense/pch.h` is the crux. `import` inside a PCH causes compiler ICEs, so all winrt content must leave it.

**Keep** in `pch.h`: `<windows.h>`, `<unknwn.h>`, `<restrictederrorinfo.h>`, `<hstring.h>` and the
`#undef GetCurrentTime`.

**Remove** from `pch.h`: all 15 `#include <winrt/*.h>` lines and `#include <wil/cppwinrt_helpers.h>` (that
WIL header is gated on winrt include guards, which the guide calls out by name).

Create `Flense/ModulePreamble.h`:

```cpp
#pragma once
#include "pch.h"          // needed because this is /FI-injected into XAML-generated files

import winrt.Windows.Foundation;
import winrt.Windows.Foundation.Collections;
import winrt.Windows.Storage;
import winrt.Windows.Storage.Streams;
import winrt.Windows.Storage.Pickers;
import winrt.Windows.ApplicationModel.Activation;
import winrt.Windows.UI.Xaml.Interop;
import winrt.Microsoft.UI.Composition;
import winrt.Microsoft.UI.Content;
import winrt.Microsoft.UI.Interop;
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
import winrt.Flense;

#define WINRT_IMPORT_MODULE
// Re-light WIL's cppwinrt support: including the now-inert winrt headers
// defines their include guards, which is what wil/cppwinrt_helpers.h keys off.
#include <winrt/Windows.Foundation.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <wil/cppwinrt.h>
#include <wil/cppwinrt_helpers.h>
```

That WIL block is not optional: `Flense/ImageDetailsViewModel.cpp:187` uses
`wil::resume_foreground(dispatcher)`, which only exists when `WINRT_Microsoft_UI_Dispatching_H` is defined.
Without the guard trick it silently disappears and you get a hard-to-read error.

The import list is a starting point derived from the current `pch.h` plus the per-file winrt includes in
`LandingPage.xaml.cpp:11-15` and `WinRtByteStream.h:3-4`. Imports are cheap — err on the side of listing
more.

### Phase 3 — Feed the preamble to the XAML-generated files

This is the part that makes WinUI harder than a plain WinRT project, and the part the guide has already
solved. The XAML compiler emits `XamlTypeInfo.g.cpp`, `XamlTypeInfo.Impl.g.cpp` and
`XamlMetaDataProvider.cpp` — they use winrt types, know nothing about modules, and are outside your
control. Inject the preamble via `/FI`. Add to `Flense/Flense.vcxproj`:

```xml
<Target Name="InjectModuleImportsIntoXamlGenerated"
        AfterTargets="ComputeXamlGeneratedCompileInputs"
        BeforeTargets="ClCompile">
  <ItemGroup>
    <ClCompile Update="@(ClCompile)"
               Condition="'%(Filename)' == 'XamlTypeInfo.g'
                       or '%(Filename)' == 'XamlTypeInfo.Impl.g'
                       or '%(Filename)' == 'XamlMetaDataProvider'
                       or '%(Filename)' == 'XamlLibMetadataProvider.g'">
      <PrecompiledHeader>NotUsing</PrecompiledHeader>
      <ForcedIncludeFiles>$(MSBuildProjectDirectory)\ModulePreamble.h</ForcedIncludeFiles>
    </ClCompile>
  </ItemGroup>
</Target>
```

Plus the identical second target with `BeforeTargets="CompileXamlGeneratedFiles"` (XAML compiles in two
passes; `Flense` is an `Application`, so pass 2 runs after the build compile phase, before `Link`). Both
targets are required.

**The `Update="@(ClCompile)"` is load-bearing** — without it MSBuild appends new items instead of editing
metadata on existing ones, and you get `LNK2005` duplicate symbols.

Leave `$(GeneratedFilesDir)module.g.cpp` alone. It only does `#include "pch.h"` and
`#include "winrt/base.h"` and imports nothing, so it stays a purely textual TU and compiles fine against
the stripped PCH.

### Phase 4 — Convert the sources

For each `.cpp` in `Flense/`, add `#include "ModulePreamble.h"` immediately after `#include "pch.h"`, and
delete the now-redundant per-file winrt includes. Three cases:

- **The bulk (~17 files)** currently name no winrt headers at all — they inherit every type implicitly from
  the PCH. They just need the preamble line.
- **`LandingPage.xaml.cpp:11-15`** is the one `.cpp` with its own winrt includes; drop them in favour of
  the preamble's imports (keep `<ShObjIdl.h>`).
- **`#include "winrt/Flense.h"`** (the self-projection) appears in `FileKindGlyphConverter.cpp`,
  `FilesystemItemColourConverter.cpp`, `MainWindow.xaml.cpp` and `FilesystemTreeNode.h` → becomes
  `import winrt.Flense;`, already in the preamble.

**`WinRtByteStream.h`** deserves attention: it's the seam where the WinRT projection meets `Flense.Core`'s
templates and concepts, and it includes `<winrt/Windows.Foundation.h>` /
`<winrt/Windows.Storage.Streams.h>` directly in the *header*, not via the PCH. Since it's included by other
headers, either include the preamble at the top of it, or leave its winrt includes inert under
`WINRT_IMPORT_MODULE`. Convert this one first — if anything is going to expose an ordering problem, it's
this file.

Do this a few files at a time and rebuild — each conversion tends to surface one or two missing imports.

#### Considered and rejected: precise per-file imports

The purer end state is a narrow `import` list in each file, with `ModulePreamble.h` reserved as the `/FI`
payload for the XAML-generated files. That would restore per-TU dependency visibility, which the PCH
currently destroys — every TU depends on every winrt namespace and there is no way to tell which
dependencies are real.

Rejected because of WIL. The `#define WINRT_IMPORT_MODULE` + guard-defines + `<wil/cppwinrt.h>` block
cannot be duplicated per file, so it would need its own wrapper header included wherever WIL is used, and
the guards must be defined before the guarded header is first included anywhere in the TU — `#pragma once`
means there is no second chance. That ordering constraint has to hold across every file individually rather
than in one place, which is a standing trap for a benefit that is real but modest at this codebase's size.

Worth revisiting if the winrt surface grows or the WIL dependency goes away.

**`WinRtByteStream.h`** deserves attention: it's the seam where the WinRT projection meets `Flense.Core`'s
templates and concepts, and it includes `<winrt/Windows.Foundation.h>` /
`<winrt/Windows.Storage.Streams.h>` directly in the *header*, not via the PCH. Since it's included by other
headers, either include the preamble at the top of it, or leave its winrt includes inert under
`WINRT_IMPORT_MODULE`. Convert this one first — if anything is going to expose an ordering problem, it's
this file.

Do this a few files at a time and rebuild — each conversion tends to surface one or two missing imports.

### Phase 5 — Prune generation for build time

Only once it's green end-to-end, cut the generation set down. Start from:

```xml
<CppWinRTModuleInclude>Windows.Foundation;Windows.Storage;Windows.ApplicationModel;Windows.UI;Microsoft.UI;Flense</CppWinRTModuleInclude>
```

then iterate. Two traps documented in the guide:

- Filtering does **not** prune `import` statements in generated modules, so any dependency that falls
  outside the filter must still be generated somewhere, or you get "could not find module".
- `Windows.Foundation.Diagnostics` depends on `Windows.Storage` and is a classic offender —
  `CppWinRTModuleExclude` it if it gets pulled in.

Clean the output directory between filter changes. If pruning turns into a fight, Phase 1's unfiltered
configuration is a perfectly valid resting place — you just pay the extra IFC compile time.

### Phase 6 — Optional: `import std;` in Flense.Core

Independent of everything above, and worth treating as a separate decision. `Flense.Core` is pure C++ with
libarchive and nlohmann/json (vcpkg). Both of those `#include` STL headers textually, and STL headers
included *after* `import std;` produce `C4348`/`C5028` — which, with `TreatWarningAsError` on `Debug|x64`
in both projects, are hard errors. The workaround is pre-including the offending STL headers in
`Flense.Core/pch.h`. Recommendation: **defer this.** It's a fight with third-party headers for very little
gain.

## Repo-specific risks

- **`TreatWarningAsError` on `Debug|x64`** (both vcxproj) turns the module/STL redefinition warnings into
  build breaks. If you get stuck mid-migration, temporarily relaxing this makes the errors readable.
- **`CppWinRTOptimized=true`** means the component projection is built with `-opt`. That's fine for a
  single project, but it's exactly why you must not later hang `CppWinRTConsumeModule=true` off any
  `ProjectReference` to `Flense`.
- **IFCs are toolset-version-specific.** The container (MSVC 14.51.36231) and the host VS must be on the
  same toolset, or the two builds will disagree. `FlenseOutputDirectory` already keeps their intermediates
  apart, which helps.
- **Win32 macro conflicts** (`GetObject`, etc.): with modules the winrt types are compiled without your
  macros applied, so conflicts surface differently. `pch.h` already handles `GetCurrentTime`; use
  `#pragma push_macro`/`pop_macro` if a new one appears.
- **The `packages/` folder is stale** (CppWinRT 3.0.260715.1, WindowsAppSDK 2.3.1) versus what the vcxproj
  pins (3.0.260818.1, 2.4.0). `/restore` in `build-app.ps1` handles this; just don't read behaviour off the
  stale package.

## Verification

After each phase:

```
.\Scripts\build-app.ps1 -OutputDirectory C:\build
.\Scripts\build-app.ps1 -Configuration Release -OutputDirectory C:\build
```

Debug is the stricter gate (`TreatWarningAsError` on `Debug|x64`) and includes the `DeveloperMenuBarItem`
XAML type that Release excludes — so build both, and use a clean output directory for any build that
follows a change to module properties.

Beyond compiling, the things to actually confirm:

1. **`Flense.winmd` is still produced** and matches the pre-migration one (the runtime classes are its
   public contract).
2. **No `LNK2019` on `InitializeComponent`** — that's the tell that Phase 3's `/FI` injection didn't take.
3. **XAML actually loads at runtime.** Module misconfiguration can produce a binary that links but fails
   XAML type resolution at startup. There's no GUI stack in the container, so this needs a run on the host.
4. Compare clean-build wall time against the Phase 0 baseline, to see whether any of this bought anything.

## Effort and exit

Realistically half a day to a day of iteration, most of it in Phases 3-5. The change surface is small and
revertible:

| File | Change |
|---|---|
| `Directory.Build.props` | per-config `IntDir` / `GeneratedFilesDir` (Phase 0.5) |
| `Flense/Flense.vcxproj` | `CppWinRTBuildModule`, `BuildStlModules`, two `/FI` targets, later the module filters |
| `Flense/pch.h` | delete lines 11-26 |
| `Flense/ModulePreamble.h` | new |
| ~20 `.cpp` + `WinRtByteStream.h`, `FilesystemTreeNode.h` | include-line swap |

If Phase 3 or 5 turns into a swamp, abandoning is a clean `git checkout` — nothing else in the repo takes a
dependency on it.
