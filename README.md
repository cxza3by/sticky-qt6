# Sticky! 📝

A minimalist sticky-note app that stays on top of other windows.
Built with **C++ + Qt6**.

![Qt](https://img.shields.io/badge/Qt-6-blue?logo=qt&style=for-the-badge)
[![GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-green.svg?style=for-the-badge)](LICENSE)

## Possibilities

- Notes always at the top (WindowStaysOnTopHint)
- Choosing bright colors for the sticker (light theme)
- Dark theme (Catppuccin Mocha palette)
- Several note tabs
- Auto-save on every text change
- Config and data is stored in `~/.config/sticky/`

---

## ⚙️ Build

### Installing dependencies

| ОС | Command |
|----|---------|
| **Ubuntu / Debian** | `sudo apt install cmake qt6-base-dev libqt6widgets6` |
| **Arch Linux** | `sudo pacman -S cmake qt6-base` |
| **Fedora** | `sudo dnf install cmake qt6-qtbase-devel` |

### Quick Launch

```bash
cd src/cpp
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/sticky
```

### Full compiling cycle + installation

```bash
cd src/cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

---

## 📦 Creating a package

### CPack + CMake

#### DEB (Debian / Ubuntu)

```bash
cd src/cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cpack --config CPackConfig.cmake
```

Or in one command:

```bash
cd src/cpp && cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCPACK_GENERATOR="DEB" \
    -DCPACK_DEBIAN_PACKAGE_ARCHITECTURE=amd64
cmake --build build
cd build && cpack
```

#### RPM (Fedora / CentOS / openSUSE)

```bash
cd src/cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCPACK_GENERATOR="RPM" \
    -DCPACK_RPM_PACKAGE_ARCHITECTURE=x86_64
cmake --build build
cd build && cpack
```

For zstd compression within RPM, this is done automatically thanks to the
`CPACK_RPM_PACKAGE_CompressionType "zstd"` setting in `CMakeLists.txt`.

#### Universal archive (.tar.gz)

```bash
cd src/cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCPACK_GENERATOR="TGZ"
cmake --build build
cd build && cpack
```
