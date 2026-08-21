#!/usr/bin/env bash
# Installs c-mi for the current user only (no sudo). Run this from inside an
# extracted c-mi-*-linux-* package directory, or point SRC_DIR at one.
set -euo pipefail

SRC_DIR="${SRC_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)}"
PREFIX="${PREFIX:-$HOME/.local}"

if [[ "${1:-}" == "--uninstall" ]]; then
    rm -f "$PREFIX/bin/c-mi"
    rm -rf "$PREFIX/lib/c-mi"
    rm -f "$PREFIX/share/applications/c-mi.desktop"
    rm -f "$PREFIX/share/icons/hicolor/scalable/apps/c-mi.svg"
    echo "Removed c-mi from $PREFIX"
    exit 0
fi

install -Dm755 "$SRC_DIR/bin/c-mi" "$PREFIX/bin/c-mi"
if [[ -d "$SRC_DIR/lib/c-mi" ]]; then
    mkdir -p "$PREFIX/lib/c-mi"
    cp -f "$SRC_DIR"/lib/c-mi/* "$PREFIX/lib/c-mi/"
fi
install -Dm644 "$SRC_DIR/share/applications/c-mi.desktop" "$PREFIX/share/applications/c-mi.desktop"
install -Dm644 "$SRC_DIR/share/icons/hicolor/scalable/apps/c-mi.svg" "$PREFIX/share/icons/hicolor/scalable/apps/c-mi.svg"

command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$PREFIX/share/applications" || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f "$PREFIX/share/icons/hicolor" || true

echo "Installed c-mi to $PREFIX/bin/c-mi"
case ":$PATH:" in
    *":$PREFIX/bin:"*) ;;
    *) echo "Note: $PREFIX/bin is not in your PATH. Add 'export PATH=\"$PREFIX/bin:\$PATH\"' to your shell profile." ;;
esac
echo "Run '$0 --uninstall' to remove it later."
