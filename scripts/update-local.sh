#!/usr/bin/env bash
set -euo pipefail

REPO_DIR="${1:-$HOME/linus/coding/vibe-coding/lite-md}"
PREFIX="${2:-$HOME/.local}"

if [[ ! -d "$REPO_DIR/.git" ]]; then
    echo "Repository not found: $REPO_DIR"
    echo "Usage: $0 [repo_dir] [install_prefix]"
    exit 1
fi

echo "[1/4] Pull latest source"
git -C "$REPO_DIR" pull --ff-only

echo "[2/4] Configure"
cmake -S "$REPO_DIR" -B "$REPO_DIR/build" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX"

echo "[3/4] Build"
if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
else
    JOBS=4
fi
cmake --build "$REPO_DIR/build" -j"$JOBS"

echo "[4/4] Install"
cmake --install "$REPO_DIR/build"

if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$PREFIX/share/applications" || true
fi

if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f "$PREFIX/share/icons/hicolor" || true
fi

echo "Update complete: $PREFIX/bin/miter"
