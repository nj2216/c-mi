#!/usr/bin/env bash
# Packages a built c-mi binary plus its desktop/icon files into a
# distributable tarball, bundling install.sh for a non-sudo user install.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build}"
BINARY="${BINARY:-$BUILD_DIR/c-mi}"
VERSION="${VERSION:-$(sed -n 's/^project(c-mi VERSION \([0-9.]*\).*/\1/p' "$ROOT_DIR/CMakeLists.txt")}"
ARCH="$(uname -m)"
DIST_DIR="${DIST_DIR:-$ROOT_DIR/dist}"
STAGE_NAME="c-mi-${VERSION}-linux-${ARCH}"
STAGE_DIR="$DIST_DIR/$STAGE_NAME"

if [[ ! -x "$BINARY" ]]; then
    echo "error: $BINARY not found or not executable; build it first (see README)" >&2
    exit 1
fi

mkdir -p "$DIST_DIR"
rm -rf "$STAGE_DIR"
install -Dm755 "$BINARY" "$STAGE_DIR/bin/c-mi"
install -Dm644 "$ROOT_DIR/data/c-mi.desktop" "$STAGE_DIR/share/applications/c-mi.desktop"
install -Dm644 "$ROOT_DIR/data/icons/c-mi.svg" "$STAGE_DIR/share/icons/hicolor/scalable/apps/c-mi.svg"

if [[ -f /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf ]]; then
    install -Dm644 /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf "$STAGE_DIR/lib/c-mi/DejaVuSansMono.ttf"
fi

if [[ -f "$ROOT_DIR/install.sh" ]]; then
    install -Dm755 "$ROOT_DIR/install.sh" "$STAGE_DIR/install.sh"
elif [[ -f "$SCRIPT_DIR/install.sh" ]]; then
    install -Dm755 "$SCRIPT_DIR/install.sh" "$STAGE_DIR/install.sh"
fi

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

TARBALL="$DIST_DIR/$STAGE_NAME.tar.gz"
tar -C "$DIST_DIR" -czf "$TARBALL" "$STAGE_NAME"
rm -rf "$STAGE_DIR"

# Generate SHA256 checksum
(cd "$DIST_DIR" && sha256sum "$STAGE_NAME.tar.gz" > "$STAGE_NAME.tar.gz.sha256")

# Also create generic alias c-mi-linux-${ARCH}.tar.gz
cp -f "$TARBALL" "$DIST_DIR/c-mi-linux-${ARCH}.tar.gz"
(cd "$DIST_DIR" && sha256sum "c-mi-linux-${ARCH}.tar.gz" > "c-mi-linux-${ARCH}.tar.gz.sha256")

echo "Package ready: $TARBALL"
echo "Checksum: $(cat "$TARBALL.sha256")"
echo "Users install with: tar -xzf $STAGE_NAME.tar.gz && ./$STAGE_NAME/install.sh"
