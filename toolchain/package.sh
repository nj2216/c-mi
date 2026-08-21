#!/usr/bin/env bash
# Packages a built c-mi binary plus its desktop/icon files into a
# distributable tarball, bundling install.sh for a non-sudo user install.
set -euo pipefail

BUILD_DIR="${BUILD_DIR:-build}"
BINARY="${BINARY:-$BUILD_DIR/c-mi}"
VERSION="${VERSION:-$(sed -n 's/^project(c-mi VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)}"
ARCH="$(uname -m)"
DIST_DIR="${DIST_DIR:-dist}"
STAGE_NAME="c-mi-${VERSION}-linux-${ARCH}"
STAGE_DIR="$DIST_DIR/$STAGE_NAME"

if [[ ! -x "$BINARY" ]]; then
    echo "error: $BINARY not found or not executable; build it first (see README)" >&2
    exit 1
fi

rm -rf "$STAGE_DIR"
install -Dm755 "$BINARY" "$STAGE_DIR/bin/c-mi"
install -Dm644 data/c-mi.desktop "$STAGE_DIR/share/applications/c-mi.desktop"
install -Dm644 data/icons/c-mi.svg "$STAGE_DIR/share/icons/hicolor/scalable/apps/c-mi.svg"
install -Dm644 /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf "$STAGE_DIR/lib/c-mi/DejaVuSansMono.ttf"
install -Dm755 toolchain/install.sh "$STAGE_DIR/install.sh"

# Bundle shared library dependencies that aren't guaranteed to exist on the
# target machine (e.g. libxcb-cursor), so the binary works even without
# those runtime packages installed. Base system libraries (libc, GL, core
# X11) are excluded since bundling GL in particular would fight the host's
# driver.
BASELINE_LIBS='^(linux-vdso\.so|ld-linux|libc\.so|libm\.so|libdl\.so|libpthread\.so|librt\.so|libresolv\.so|libutil\.so|libanl\.so|libGL\.so|libGLX\.so|libGLdispatch\.so|libEGL\.so|libnss_|libselinux\.so)'
ldd "$BINARY" 2>/dev/null | awk '$2 == "=>" && $3 != "" {print $1, $3}' |
while read -r lib_name lib_path; do
    if [[ -f "$lib_path" ]] && ! [[ "$lib_name" =~ $BASELINE_LIBS ]]; then
        install -Dm755 "$lib_path" "$STAGE_DIR/lib/c-mi/$lib_name"
    fi
done

tar -C "$DIST_DIR" -czf "$DIST_DIR/$STAGE_NAME.tar.gz" "$STAGE_NAME"
rm -rf "$STAGE_DIR"

echo "Package ready: $DIST_DIR/$STAGE_NAME.tar.gz"
echo "Users install with: tar -xzf $STAGE_NAME.tar.gz && ./$STAGE_NAME/install.sh"
