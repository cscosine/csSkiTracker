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
