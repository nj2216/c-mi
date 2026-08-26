#!/usr/bin/env bash
# ==============================================================================
# c~mi Installer (No sudo required)
# Native Linux Camera Application
#
# Quick Install:
#   curl -fsSL https://raw.githubusercontent.com/nj2216/c-mi/main/install.sh | bash
#
# Specific Version:
#   curl -fsSL https://raw.githubusercontent.com/nj2216/c-mi/main/install.sh | bash -s -- -v v0.1.0
#
# Uninstall:
#   curl -fsSL https://raw.githubusercontent.com/nj2216/c-mi/main/install.sh | bash -s -- --uninstall
# ==============================================================================

set -euo pipefail

REPO="${REPO:-nj2216/c-mi}"
PREFIX="${PREFIX:-$HOME/.local}"
REQUESTED_VERSION="${VERSION:-}"
UNINSTALL=false
VERBOSE=false

# Formatting / Colors
if [[ -t 1 ]] && [[ -z "${NO_COLOR:-}" ]]; then
    BOLD="\033[1m"
    GREEN="\033[32m"
    YELLOW="\033[33m"
    RED="\033[31m"
    CYAN="\033[36m"
    RESET="\033[0m"
else
    BOLD=""
    GREEN=""
    YELLOW=""
    RED=""
    CYAN=""
    RESET=""
fi

log_info()    { echo -e "${CYAN}[c~mi]${RESET} $*"; }
log_success() { echo -e "${GREEN}${BOLD}[c~mi] ✓${RESET} $*"; }
log_warn()    { echo -e "${YELLOW}${BOLD}[c~mi] !${RESET} $*" >&2; }
log_error()   { echo -e "${RED}${BOLD}[c~mi] ✗${RESET} $*" >&2; }

show_help() {
    cat << EOF
c~mi Installer

USAGE:
  install.sh [OPTIONS]

OPTIONS:
  -p, --prefix <DIR>     Installation directory prefix (default: \$HOME/.local)
  -v, --version <TAG>    Install a specific release version (e.g. v0.1.0)
  -r, --repo <OWNER/REPO> GitHub repository (default: nj2216/c-mi)
  -u, --uninstall        Uninstall c~mi from prefix
  -h, --help             Show this help message

EXAMPLES:
  # Install latest release
  ./install.sh

  # Install to custom directory without root
  ./install.sh --prefix /opt/my-apps

  # Uninstall
  ./install.sh --uninstall

EOF
}

# Parse CLI arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--prefix)
            PREFIX="$2"
            shift 2
            ;;
        -v|--version)
            REQUESTED_VERSION="$2"
            shift 2
            ;;
        -r|--repo)
            REPO="$2"
            shift 2
            ;;
        -u|--uninstall)
            UNINSTALL=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

BIN_DIR="$PREFIX/bin"
LIB_DIR="$PREFIX/lib/c-mi"
APPS_DIR="$PREFIX/share/applications"
ICONS_DIR="$PREFIX/share/icons/hicolor/scalable/apps"

# ------------------------------------------------------------------------------
# Handle Uninstall
# ------------------------------------------------------------------------------
if [[ "$UNINSTALL" == true ]]; then
    log_info "Uninstalling c~mi from $PREFIX..."
    rm -f "$BIN_DIR/c-mi"
    rm -rf "$LIB_DIR"
    rm -f "$APPS_DIR/c-mi.desktop"
    rm -f "$ICONS_DIR/c-mi.svg"

    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APPS_DIR" 2>/dev/null || true
    command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" 2>/dev/null || true

    log_success "c~mi has been successfully uninstalled."
    exit 0
fi

# ------------------------------------------------------------------------------
# Detect Architecture & System
# ------------------------------------------------------------------------------
OS="$(uname -s | tr '[:upper:]' '[:lower:]')"
if [[ "$OS" != "linux" ]]; then
    log_error "This installer only supports Linux (detected: $OS)."
    exit 1
fi

ARCH="$(uname -m)"
case "$ARCH" in
    x86_64|amd64)
        ARCH_MATCH="x86_64"
        ;;
    aarch64|arm64)
        ARCH_MATCH="aarch64"
        ;;
    *)
        ARCH_MATCH="$ARCH"
        ;;
esac

# ------------------------------------------------------------------------------
# Helper: Download utility
# ------------------------------------------------------------------------------
download_file() {
    local url="$1"
    local output="$2"
    if command -v curl >/dev/null 2>&1; then
        curl -fSL --progress-bar -o "$output" "$url"
    elif command -v wget >/dev/null 2>&1; then
        wget -q --show-progress -O "$output" "$url"
    else
        log_error "Neither curl nor wget found. Please install curl or wget."
        exit 1
    fi
}

fetch_json() {
    local url="$1"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL -H "Accept: application/vnd.github.v3+json" "$url"
    elif command -v wget >/dev/null 2>&1; then
        wget -qO- --header="Accept: application/vnd.github.v3+json" "$url"
    fi
}

# ------------------------------------------------------------------------------
# Determine Install Source (Local extracted dir vs Remote GitHub download)
# ------------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" 2>/dev/null && pwd || echo "")"
LOCAL_BIN=""

if [[ -n "$SCRIPT_DIR" ]] && [[ -f "$SCRIPT_DIR/bin/c-mi" ]]; then
    LOCAL_BIN="$SCRIPT_DIR"
elif [[ -n "$SCRIPT_DIR" ]] && [[ -f "$SCRIPT_DIR/build/c-mi" ]]; then
    LOCAL_BIN="$SCRIPT_DIR/build"
elif [[ -n "$SCRIPT_DIR" ]] && [[ -f "$SCRIPT_DIR/build-static/c-mi" ]]; then
    LOCAL_BIN="$SCRIPT_DIR/build-static"
elif [[ -n "$SCRIPT_DIR" ]] && [[ -f "$SCRIPT_DIR/c-mi" ]]; then
    LOCAL_BIN="$SCRIPT_DIR"
elif [[ -f "./bin/c-mi" ]]; then
    LOCAL_BIN="$(pwd)"
elif [[ -f "./build/c-mi" ]]; then
    LOCAL_BIN="$(pwd)/build"
fi

TEMP_DIR=""
cleanup() {
    if [[ -n "$TEMP_DIR" && -d "$TEMP_DIR" ]]; then
        rm -rf "$TEMP_DIR"
    fi
}
trap cleanup EXIT INT TERM

SRC_ROOT=""

if [[ -n "$LOCAL_BIN" ]]; then
    log_info "Installing from local package at: ${LOCAL_BIN}"
    SRC_ROOT="$LOCAL_BIN"
else
    # --------------------------------------------------------------------------
    # Remote Download from GitHub Releases
    # --------------------------------------------------------------------------
    log_info "Fetching release information from GitHub (${REPO})..."

    if [[ -n "$REQUESTED_VERSION" ]]; then
        TAG="$REQUESTED_VERSION"
        [[ "$TAG" != v* ]] && TAG="v$TAG"
        API_URL="https://api.github.com/repos/${REPO}/releases/tags/${TAG}"
    else
        API_URL="https://api.github.com/repos/${REPO}/releases/latest"
    fi

    RELEASE_JSON="$(fetch_json "$API_URL" || true)"
    DOWNLOAD_URL=""
    TAG_NAME=""

    if [[ -n "$RELEASE_JSON" ]]; then
        TAG_NAME="$(echo "$RELEASE_JSON" | grep -o '"tag_name": *"[^"]*"' | head -n1 | cut -d'"' -f4 || true)"
        # Try finding asset matching linux and architecture
        DOWNLOAD_URL="$(echo "$RELEASE_JSON" | grep -o '"browser_download_url": *"[^"]*"' | cut -d'"' -f4 | grep -i "linux" | grep -i "${ARCH_MATCH}" | grep -E "\.tar\.gz$" | head -n1 || true)"
        if [[ -z "$DOWNLOAD_URL" ]]; then
            DOWNLOAD_URL="$(echo "$RELEASE_JSON" | grep -o '"browser_download_url": *"[^"]*"' | cut -d'"' -f4 | grep -E "\.tar\.gz$" | head -n1 || true)"
        fi
    fi

    # Fallback to direct URL if API rate-limited or JSON parsing failed
    if [[ -z "$DOWNLOAD_URL" ]]; then
        if [[ -n "$REQUESTED_VERSION" ]]; then
            TAG_NAME="$REQUESTED_VERSION"
        else
            TAG_NAME="latest"
        fi
        DOWNLOAD_URL="https://github.com/${REPO}/releases/download/${TAG_NAME}/c-mi-${TAG_NAME}-linux-${ARCH_MATCH}.tar.gz"
    fi

    log_info "Downloading c~mi ${TAG_NAME:-release} for ${ARCH_MATCH}..."
    TEMP_DIR="$(mktemp -d)"
    TARBALL_PATH="$TEMP_DIR/c-mi.tar.gz"

    if ! download_file "$DOWNLOAD_URL" "$TARBALL_PATH"; then
        # Try generic fallback URL
        ALT_URL="https://github.com/${REPO}/releases/latest/download/c-mi-linux-${ARCH_MATCH}.tar.gz"
        log_warn "Download failed from $DOWNLOAD_URL, trying $ALT_URL..."
        download_file "$ALT_URL" "$TARBALL_PATH" || {
            log_error "Failed to download release tarball from GitHub."
            log_error "Please check your internet connection or verify releases at: https://github.com/${REPO}/releases"
            exit 1
        }
    fi

    log_info "Extracting release package..."
    tar -xzf "$TARBALL_PATH" -C "$TEMP_DIR"

    # Locate extracted directory
    EXTRACTED_DIR="$(find "$TEMP_DIR" -mindepth 1 -maxdepth 2 -type f -name "c-mi" -exec dirname {} \; | head -n1 || true)"
    if [[ -n "$EXTRACTED_DIR" ]]; then
        if [[ "$(basename "$EXTRACTED_DIR")" == "bin" ]]; then
            SRC_ROOT="$(dirname "$EXTRACTED_DIR")"
        else
            SRC_ROOT="$EXTRACTED_DIR"
        fi
    else
        SRC_ROOT="$TEMP_DIR"
    fi
fi

# ------------------------------------------------------------------------------
# Install Files into $PREFIX
# ------------------------------------------------------------------------------
log_info "Installing c~mi into ${PREFIX}..."

mkdir -p "$BIN_DIR" "$APPS_DIR" "$ICONS_DIR"

# 1. Binary
if [[ -f "$SRC_ROOT/bin/c-mi" ]]; then
    install -Dm755 "$SRC_ROOT/bin/c-mi" "$BIN_DIR/c-mi"
elif [[ -f "$SRC_ROOT/c-mi" ]]; then
    install -Dm755 "$SRC_ROOT/c-mi" "$BIN_DIR/c-mi"
else
    log_error "Could not locate c-mi binary in package."
    exit 1
fi

# 2. Bundled Shared Libraries & Font (if any)
if [[ -d "$SRC_ROOT/lib/c-mi" ]]; then
    mkdir -p "$LIB_DIR"
    cp -rf "$SRC_ROOT"/lib/c-mi/* "$LIB_DIR/"
fi

# 3. Desktop Entry
if [[ -f "$SRC_ROOT/share/applications/c-mi.desktop" ]]; then
    install -Dm644 "$SRC_ROOT/share/applications/c-mi.desktop" "$APPS_DIR/c-mi.desktop"
elif [[ -f "$SRC_ROOT/data/c-mi.desktop" ]]; then
    install -Dm644 "$SRC_ROOT/data/c-mi.desktop" "$APPS_DIR/c-mi.desktop"
elif [[ -f "$SCRIPT_DIR/data/c-mi.desktop" ]]; then
    install -Dm644 "$SCRIPT_DIR/data/c-mi.desktop" "$APPS_DIR/c-mi.desktop"
fi

# 4. Icon
if [[ -f "$SRC_ROOT/share/icons/hicolor/scalable/apps/c-mi.svg" ]]; then
    install -Dm644 "$SRC_ROOT/share/icons/hicolor/scalable/apps/c-mi.svg" "$ICONS_DIR/c-mi.svg"
elif [[ -f "$SRC_ROOT/data/icons/c-mi.svg" ]]; then
    install -Dm644 "$SRC_ROOT/data/icons/c-mi.svg" "$ICONS_DIR/c-mi.svg"
elif [[ -f "$SCRIPT_DIR/data/icons/c-mi.svg" ]]; then
    install -Dm644 "$SCRIPT_DIR/data/icons/c-mi.svg" "$ICONS_DIR/c-mi.svg"
fi

# Update desktop and icon databases
command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$APPS_DIR" 2>/dev/null || true
command -v gtk-update-icon-cache >/dev/null 2>&1 && gtk-update-icon-cache -f -t "$PREFIX/share/icons/hicolor" 2>/dev/null || true

# ------------------------------------------------------------------------------
# Final Verification & Guidance
# ------------------------------------------------------------------------------
log_success "c~mi has been successfully installed!"
echo
echo -e "  ${BOLD}Executable:${RESET}  $BIN_DIR/c-mi"
echo -e "  ${BOLD}Desktop App:${RESET} $APPS_DIR/c-mi.desktop"
echo

case ":$PATH:" in
    *":$BIN_DIR:"*)
        echo -e "You can now run c~mi by typing ${GREEN}${BOLD}c-mi${RESET} in your terminal or launching it from your Application menu."
        ;;
    *)
        log_warn "$BIN_DIR is not in your current PATH."
        echo
        echo "To run 'c-mi' directly from terminal, add it to your PATH:"
        echo -e "  ${CYAN}echo 'export PATH=\"$BIN_DIR:\$PATH\"' >> ~/.bashrc${RESET}  # or ~/.zshrc"
        echo -e "  ${CYAN}source ~/.bashrc${RESET}"
        echo
        echo -e "Or run it directly with: ${BOLD}$BIN_DIR/c-mi${RESET}"
        ;;
esac

echo
echo -e "To uninstall anytime, run: ${CYAN}$0 --uninstall${RESET}"
echo
