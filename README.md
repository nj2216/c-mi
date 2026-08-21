# c~mi

**c~mi** (pronounced *"See Me"*) is a native Linux camera application targeting
Xubuntu/XFCE. It talks to webcams over raw V4L2, renders the live preview with
OpenGL, and encodes photos/video with FFmpeg libraries — no GStreamer, no
libcamera, no GNOME portals.

## Stack

- C++17, Qt6 Widgets (no QML)
- Raw V4L2 (`ioctl` + `mmap` buffer ring) for capture
- libavcodec / libavformat / libswscale / libswresample / libavdevice for encode+mux
- Kernel netlink uevents for hotplug detection
- QSettings (INI backend) for per-device control presets
- CMake + pkg-config

## Build

Dependencies (Debian/Ubuntu/Xubuntu):

```sh
sudo apt install build-essential cmake pkg-config \
    qt6-base-dev libgl1-mesa-dev \
    libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev libswresample-dev libavdevice-dev
```

Then:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/c-mi
```

### Static build

To produce a binary without runtime Qt or FFmpeg dependencies, use a toolchain
containing static archives for those libraries. Qt must be built with `-static`; the normal Ubuntu `qt6-base-dev`
package is shared-only and will be rejected by this mode.

```sh
cmake -B build-static -DCMAKE_BUILD_TYPE=Release -DCMI_STATIC=ON
cmake --build build-static -j
```

OpenGL, X11, and libc are still linked dynamically: GL has to `dlopen` the
vendor driver at runtime, so it can never be a static archive, and both are
already present on any working Linux desktop. Kernel V4L2 device access and
the camera/audio backends still depend on the host kernel and hardware;
static linking does not bundle those services or device nodes.

For a reproducible static build, use the included Docker toolchain. Docker
builds Qt6 and FFmpeg from source, then places the resulting binary in an
image named `c-mi-static`:

```sh
docker build -f toolchain/Dockerfile -t c-mi-static .
container=$(docker create c-mi-static)
docker cp "$container:/c-mi" ./c-mi
docker rm "$container"
```

The resulting `c-mi` executable is statically linked and can be copied to a
matching Linux system without installing the application libraries.

To build the static Qt6/FFmpeg prefix directly on the host instead of in
Docker, run `toolchain/build-static.sh` (needs `sudo`; builds Qt from source,
which takes a while):

```sh
./toolchain/build-static.sh
cmake -B build-static -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=toolchain/static-linux.cmake -DCMI_STATIC=ON
cmake --build build-static --parallel
```

Install (binary, `.desktop` entry, and XDG hicolor icon):

```sh
sudo cmake --install build
```

The desktop file registers under `AudioVideo;Video;Photography;` so it shows up
in the XFCE Whisker menu, and the icon installs into the hicolor theme so it
follows XDG icon theme lookup.

### Packaging a distributable build

`toolchain/package.sh` bundles a built `c-mi` binary (native or static) with
its `.desktop` entry and icon into a tarball:

```sh
./toolchain/package.sh                 # packages build/c-mi by default
BINARY=build-static/c-mi ./toolchain/package.sh   # package a static build
```

This writes `dist/c-mi-<version>-linux-<arch>.tar.gz`, containing `bin/c-mi`,
the `.desktop`/icon files, and a bundled `install.sh`.

### Installing without sudo

Users can install the package into `~/.local` without root:

```sh
tar -xzf c-mi-<version>-linux-<arch>.tar.gz
./c-mi-<version>-linux-<arch>/install.sh
```

This installs to `~/.local/bin`, `~/.local/share/applications`, and
`~/.local/share/icons/hicolor/scalable/apps` (override with `PREFIX`). Add
`~/.local/bin` to `PATH` if it isn't already, if prompted. Remove with:

```sh
./c-mi-<version>-linux-<arch>/install.sh --uninstall
```

## Features

- Enumerates all `/dev/video*` capture nodes at startup and hot-reloads on
  kernel video uevents.
- Queries formats via `VIDIOC_ENUM_FMT`; prefers MJPEG > YUYV > H264.
- Live preview: V4L2 `mmap` buffer ring (`REQBUFS → QUERYBUF → mmap →
  QBUF/DQBUF`), MJPEG decoded via libavcodec, YUYV converted to RGBA, uploaded
  as a GL texture and rendered in a `QOpenGLWidget`.
- Photo capture to JPEG via libavcodec.
- Video recording to H264/MP4 via libavformat with audio muxed from the
  default PipeWire/Pulse input (FFmpeg `pulse` demuxer) — audio is
  best-effort: recording continues video-only if no audio source is available.
- V4L2 controls (brightness, contrast, saturation, gamma, sharpness, exposure,
  white balance, gain, pan/tilt/zoom — whatever the device exposes via
  `VIDIOC_QUERYCTRL`) appear as live sliders / toggles / dropdowns wired to
  `VIDIOC_S_CTRL`.
- Per-device control presets saved in `~/.config/c-mi/c-mi.conf` (INI).
- System tray icon indicates idle / camera-in-use / recording; a persistent
  on-screen "CAMERA IN USE" banner covers devices without a hardware LED.

## Configuration

Settings live in `~/.config/c-mi/c-mi.conf` (QSettings INI). Known keys:

- `photoDir`, `videoDir` — override output directories (defaults:
  `~/Pictures/c-mi`).
- `presets/<deviceKey>/<name>/<controlId>` — saved control presets.

## Design constraints

- X11 primary; nothing X11-specific is hardcoded, so XFCE's experimental
  Wayland session (XWayland) works too.
- No dconf/GSettings, no GStreamer, no libcamera, no xdg-desktop-portal.
- UI: dark, high-contrast terminal aesthetic — JetBrains Mono (with monospace
  fallback), warm off-white `#e8e0d0` on near-black `#0d0d0d`, orange-brown
  `#c86a2e` accent for active/recording states.
