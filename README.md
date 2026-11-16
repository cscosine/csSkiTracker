# csskitracker

## Download

The latest release, ready to run: extract the archive, then run `bin/skiTrackerApp` (`bin\skiTrackerApp.exe` on Windows).

### Default release

Built with MSVC 2022 (Windows), gcc (Ubuntu), Apple Clang with Ninja Multi-Config (macOS).

| Platform | Download |
|---|---|
| Windows 10 / 11 | [x64](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-windows-v10-x64-generic-msvc-default-msvc2022-app-Release.zip) |
| Ubuntu 22.04 | [x64](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu22.04-x64-generic-gcc-default-ninjamulticonfig-app-Release.tar.gz) · [arm64](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu22.04-arm64-generic-gcc-default-ninja-app-Release.tar.gz) |
| Ubuntu 24.04 | [x64](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu24.04-x64-generic-gcc-default-ninjamulticonfig-app-Release.tar.gz) · [arm64](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu24.04-arm64-generic-gcc-default-ninja-app-Release.tar.gz) |
| Ubuntu 26.04 | [x64](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu26.04-x64-generic-gcc-default-ninjamulticonfig-app-Release.tar.gz) · [arm64](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu26.04-arm64-generic-gcc-default-ninja-app-Release.tar.gz) |
| macOS 14.4 or newer | [Apple Silicon](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-macos-macos15-arm64-generic-appleclang-default-ninjamulticonfig-app-Release.tar.gz) |

Linux: the archive lists the system packages the application needs in `system-requirements.txt` (one `apt install`
line).

The builds are not signed yet, so Windows and macOS warn the first time:

- **Windows** (SmartScreen, "Windows protected your PC"): before extracting, right-click the `.zip` → Properties →
  *Unblock*; or, at the warning, *More info* → *Run anyway*.
- **macOS** (Gatekeeper): after extracting, in Terminal `xattr -dr com.apple.quarantine <extracted folder>`; or run it
  once, then System Settings → Privacy & Security → *Open Anyway*.

### Alternative versions

The same application built with the other compilers and generators of CI; the builds of a platform are
equivalent.

| Windows x64 | MSVC | clang-cl |
|---|---|---|
| Visual Studio 2022 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-windows-v10-x64-generic-msvc-default-msvc2022-app-Release.zip) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-windows-v10-x64-generic-msvcclang-default-msvc2022-app-Release.zip) |
| Visual Studio 2026 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-windows-v10-x64-generic-msvc-default-msvc2026-app-Release.zip) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-windows-v10-x64-generic-msvcclang-default-msvc2026-app-Release.zip) |

| Ubuntu | gcc | clang |
|---|---|---|
| 22.04 x64 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu22.04-x64-generic-gcc-default-ninjamulticonfig-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu22.04-x64-generic-clang-default-ninjamulticonfig-app-Release.tar.gz) |
| 22.04 arm64 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu22.04-arm64-generic-gcc-default-ninja-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu22.04-arm64-generic-clang-default-ninja-app-Release.tar.gz) |
| 24.04 x64 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu24.04-x64-generic-gcc-default-ninjamulticonfig-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu24.04-x64-generic-clang-default-ninjamulticonfig-app-Release.tar.gz) |
| 24.04 arm64 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu24.04-arm64-generic-gcc-default-ninja-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu24.04-arm64-generic-clang-default-ninja-app-Release.tar.gz) |
| 26.04 x64 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu26.04-x64-generic-gcc-default-ninjamulticonfig-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu26.04-x64-generic-clang-default-ninjamulticonfig-app-Release.tar.gz) |
| 26.04 arm64 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu26.04-arm64-generic-gcc-default-ninja-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-linux-ubuntu26.04-arm64-generic-clang-default-ninja-app-Release.tar.gz) |

| macOS arm64 (runs from 14.4) | Ninja Multi-Config | Xcode |
|---|---|---|
| built on macOS 15 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-macos-macos15-arm64-generic-appleclang-default-ninjamulticonfig-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-macos-macos15-arm64-generic-appleclang-default-xcode-app-Release.tar.gz) |
| built on macOS 26 | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-macos-macos26-arm64-generic-appleclang-default-ninjamulticonfig-app-Release.tar.gz) | [⬇](https://github.com/cscosine/csSkiTracker/releases/latest/download/csSkiTracker-0.1.0-csv1-macos-macos26-arm64-generic-appleclang-default-xcode-app-Release.tar.gz) |

## Licenses

csSkiTracker is released under the [MIT License](LICENSE). The deployments of the releases also contain, or are built with:

- **Qt 6.12** ([source](https://download.qt.io/official_releases/qt/6.12/), built by
  [csqt6](https://github.com/cscosine/csqt6) without the GPL-only modules): [LGPL v3](https://www.gnu.org/licenses/lgpl-3.0.html),
  as separate shared libraries that can be replaced.
- **OpenCV 5** ([fork](https://github.com/cscosine/opencv), built by [csopencvcpu](https://github.com/cscosine/csopencvcpu)):
  [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0); on Windows its libraries include libjpeg-turbo (this
  software is based in part on the work of the Independent JPEG Group), libpng, libtiff, OpenJPEG, libwebp and zlib,
  under their permissive licenses.
- **Eigen** ([source](https://gitlab.com/libeigen/eigen)): [MPL 2.0](https://www.mozilla.org/MPL/2.0/), unmodified, header-only.
- The libraries of [csfoundation](https://github.com/cscosine/csfoundation) and the third-party libraries of
  [third_party_base_libs](https://github.com/cscosine/third_party_base_libs) (fmt, cpptrace, libassert, magic_enum,
  ...: MIT and similar permissive licenses).
- Windows: the Visual C++ runtime DLLs, redistributed under the Microsoft Visual Studio redistribution terms.
- Linux: the system libraries listed in `system-requirements.txt` (installed from Ubuntu, not shipped), among them
  FFmpeg, used by OpenCV to read videos.

The archives do not include the license texts of these components yet; a `licenses/` folder in every deployment is
planned.
