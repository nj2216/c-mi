# c~mi

**c~mi** (pronounced *"See Me"*) is a native Linux camera application targeting
Xubuntu/XFCE. It talks to webcams over raw V4L2, renders the live preview with
OpenGL, and encodes photos/video with FFmpeg libraries — no GStreamer, no
libcamera, no GNOME portals.

## Stack

- C++17, Qt6 Widgets (no QML)
- Raw V4L2 (`ioctl` + `mmap` buffer ring) for capture
- libavcodec / libavformat / libswscale / libswresample / libavdevice for encode+mux
- libudev for hotplug detection
- QSettings (INI backend) for per-device control presets
- CMake + pkg-config

## Build

Dependencies (Debian/Ubuntu/Xubuntu):

```sh
sudo apt install build-essential cmake pkg-config \
    qt6-base-dev libgl1-mesa-dev \
    libv4l-dev libudev-dev \
    libavcodec-dev libavformat-dev libavutil-dev \
    libswscale-dev libswresample-dev libavdevice-dev
```

Then:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/c-mi
```

Install (binary, `.desktop` entry, and XDG hicolor icon):

```sh
sudo cmake --install build
```

The desktop file registers under `AudioVideo;Video;Photography;` so it shows up
in the XFCE Whisker menu, and the icon installs into the hicolor theme so it
follows XDG icon theme lookup.

## Features

- Enumerates all `/dev/video*` capture nodes at startup and hot-reloads on
  udev add/remove events.
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
