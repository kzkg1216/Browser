# MiniBrowser

A minimal (MVP) C++ web browser for Windows.
The window and toolbar are built with the Win32 API, and rendering is
delegated to **WebView2** (the Chromium-based engine used by Microsoft Edge).

## Features (MVP)

- Web page rendering
- Address bar (press Enter to navigate; `https://` is added when the
  scheme is omitted)
- Back / Forward / Reload buttons
- Window title follows the page title
- High DPI support

## Architecture

```
+--------------------------------------------------+
| MiniBrowser (Win32 window)                       |
| +---+ +---+ +---+ +----------------------------+ |
| | < | | > | | R | | Address bar (EDIT)         | |
| +---+ +---+ +---+ +----------------------------+ |
| +----------------------------------------------+ |
| | WebView2 (Chromium/Blink engine)             | |
| |   Renders HTML/CSS and executes JavaScript   | |
| +----------------------------------------------+ |
+--------------------------------------------------+
```

- **Code we own**: the UI — window, toolbar, address bar (`src/main.cpp`)
- **Engine (WebView2)**: all HTML/CSS/JS processing is delegated to it.
  Windows 10/11 ships with the WebView2 runtime, so the distribution is
  just a small exe plus `WebView2Loader.dll`.

## Building

### Prerequisites

- Windows 10 / 11
- Visual Studio 2022 ("Desktop development with C++" workload)
- CMake 3.20 or later (the one bundled with Visual Studio works)

The WebView2 SDK is downloaded automatically from NuGet at configure
time; no manual installation is required.

### Steps

The supported targets are **x86** and **arm64**. From a developer
command prompt:

```bat
:: x86 build
cmake -B build-x86 -A Win32
cmake --build build-x86 --config Release

:: arm64 build (cross-compiles fine from an x64 machine)
cmake -B build-arm64 -A ARM64
cmake --build build-arm64 --config Release
```

Run:

```bat
build-x86\Release\MiniBrowser.exe
```

### CI/CD

GitHub Actions (`.github/workflows/build.yml`) builds both x86 and
arm64 on `windows-latest` for every push / PR and uploads a zip
(exe + `WebView2Loader.dll`) as an artifact.

### Releases

Pushing a tag that starts with `v` builds both architectures and
automatically creates a GitHub Release with
`MiniBrowser-x86.zip` / `MiniBrowser-arm64.zip` attached:

```bat
git tag v0.1.0
git push origin v0.1.0
```

## Ideas for future work

- Tabs (multiple WebView2 instances with switching)
- Bookmarks and history
- Search from the address bar (send non-URL input to a search engine)
- Download UI and context menu customization
- Handling new-window requests (`add_NewWindowRequested`)
