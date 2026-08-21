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
install -Dm755 toolchain/install.sh "$STAGE_DIR/install.sh"

tar -C "$DIST_DIR" -czf "$DIST_DIR/$STAGE_NAME.tar.gz" "$STAGE_NAME"
rm -rf "$STAGE_DIR"

echo "Package ready: $DIST_DIR/$STAGE_NAME.tar.gz"
echo "Users install with: tar -xzf $STAGE_NAME.tar.gz && ./$STAGE_NAME/install.sh"
