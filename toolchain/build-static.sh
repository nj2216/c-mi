#!/usr/bin/env bash
# Builds the static Qt6/FFmpeg prefix used by CMI_STATIC directly on the
# host, as an alternative to toolchain/Dockerfile. Requires sudo for package
# installation and writing to STATIC_PREFIX.
set -euo pipefail

QT_VERSION="${QT_VERSION:-6.8.3}"
FFMPEG_VERSION="${FFMPEG_VERSION:-7.1.1}"
STATIC_PREFIX="${STATIC_PREFIX:-/opt/cmi-static}"
BUILD_DIR="${BUILD_DIR:-$(mktemp -d)}"
JOBS="${JOBS:-$(nproc)}"
QT_MINOR="${QT_VERSION%.*}"

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    autoconf automake build-essential ca-certificates cmake curl file \
    libasound2-dev libdbus-1-dev libfontconfig1-dev libfreetype6-dev libgl-dev \
    libglib2.0-dev libice-dev libinput-dev libjpeg-dev \
    libpng-dev libpulse-dev libsm-dev libv4l-dev libx11-dev libx11-xcb-dev libxcb1-dev \
    libxcb-cursor-dev libxcb-glx0-dev libxcb-icccm4-dev \
    libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev \
    libxcb-render-util0-dev libxcb-render0-dev libxcb-shape0-dev \
    libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev \
    libxcb-xinerama0-dev libxcb-xkb-dev libxext-dev libxi-dev \
    libxkbcommon-dev libxkbcommon-x11-dev libxrender-dev libxshmfence-dev \
    libzstd-dev nasm ninja-build perl pkg-config python3 tar yasm zlib1g-dev

sudo mkdir -p "$STATIC_PREFIX"
sudo chown "$(id -u):$(id -g)" "$STATIC_PREFIX"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

curl -fsSLO "https://download.qt.io/official_releases/qt/${QT_MINOR}/${QT_VERSION}/submodules/qtbase-everywhere-src-${QT_VERSION}.tar.xz"
tar -xf "qtbase-everywhere-src-${QT_VERSION}.tar.xz"
(
    cd "qtbase-everywhere-src-${QT_VERSION}"
    ./configure -static -release -opensource -confirm-license \
        -prefix "$STATIC_PREFIX/qt" -nomake tests -nomake examples \
        -qt-zlib -qt-pcre -qt-freetype -qt-harfbuzz -qt-libpng -qt-libjpeg \
        -no-openssl -no-icu -xcb -xcb-xlib -opengl desktop
    cmake --build . --parallel "$JOBS"
    cmake --install .
)

curl -fsSLO "https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz"
tar -xf "ffmpeg-${FFMPEG_VERSION}.tar.xz"
(
    cd "ffmpeg-${FFMPEG_VERSION}"
    ./configure --prefix="$STATIC_PREFIX" --disable-shared --enable-static \
        --disable-programs --disable-doc --disable-debug --disable-network \
        --enable-avdevice --enable-indev=v4l2 --enable-indev=pulse --enable-indev=alsa \
        --enable-pic
    make -j"$JOBS"
    make install
)

echo
echo "Static Qt/FFmpeg prefix ready at $STATIC_PREFIX"
echo "Build c-mi with:"
echo "  cmake -S . -B build-static -DCMAKE_BUILD_TYPE=Release \\"
echo "      -DCMAKE_TOOLCHAIN_FILE=toolchain/static-linux.cmake -DCMI_STATIC=ON \\"
echo "      -DCMI_STATIC_PREFIX=$STATIC_PREFIX"
echo "  cmake --build build-static --parallel"
