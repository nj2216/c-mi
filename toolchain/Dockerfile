FROM ubuntu:24.04

ARG QT_VERSION=6.8.3
ARG FFMPEG_VERSION=7.1.1
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    autoconf automake build-essential ca-certificates cmake curl file \
    libdbus-1-dev libfontconfig1-dev libfreetype6-dev libgl-dev \
    libglib2.0-dev libice-dev libicu-dev libinput-dev libjpeg-dev \
    libpng-dev libsm-dev libx11-dev libx11-xcb-dev libxcb1-dev \
    libxcb-cursor-dev libxcb-glx0-dev libxcb-icccm4-dev \
    libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev \
    libxcb-render-util0-dev libxcb-render0-dev libxcb-shape0-dev \
    libxcb-shm0-dev libxcb-sync-dev libxcb-util-dev libxcb-xfixes0-dev \
    libxcb-xinerama0-dev libxcb-xkb-dev libxext-dev libxi-dev \
    libxkbcommon-dev libxkbcommon-x11-dev libxrender-dev libxshmfence-dev \
    libzstd-dev nasm ninja-build perl pkg-config python3 tar yasm zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp/build

RUN curl -fsSLO https://download.qt.io/official_releases/qt/6.8/${QT_VERSION}/submodules/qtbase-everywhere-src-${QT_VERSION}.tar.xz \
    && tar -xf qtbase-everywhere-src-${QT_VERSION}.tar.xz \
    && cd qtbase-everywhere-src-${QT_VERSION} \
    && ./configure -static -release -opensource -confirm-license \
        -prefix /opt/cmi-static/qt -nomake tests -nomake examples \
        -qt-zlib -qt-pcre -qt-freetype -qt-harfbuzz -qt-libpng -qt-libjpeg \
        -no-openssl -xcb -xcb-xlib -opengl desktop \
    && cmake --build . --parallel \
    && cmake --install .

RUN curl -fsSLO https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.xz \
    && tar -xf ffmpeg-${FFMPEG_VERSION}.tar.xz \
    && cd ffmpeg-${FFMPEG_VERSION} \
    && ./configure --prefix=/opt/cmi-static --disable-shared --enable-static \
        --disable-programs --disable-doc --disable-debug --disable-network \
        --disable-autodetect --enable-pic \
    && make -j"$(nproc)" \
    && make install

COPY toolchain/static-linux.cmake /opt/cmi-static/static-linux.cmake
COPY . /src
WORKDIR /src

RUN cmake -S . -B /tmp/cmi-build -GNinja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/cmi-static/static-linux.cmake \
        -DCMI_STATIC=ON \
    && cmake --build /tmp/cmi-build \
    && install -Dm755 /tmp/cmi-build/c-mi /opt/cmi-static/bin/c-mi

FROM scratch
COPY --from=0 /opt/cmi-static/bin/c-mi /c-mi
ENTRYPOINT ["/c-mi"]
