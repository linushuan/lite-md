#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_REPO_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

REPO_DIR="${1:-$DEFAULT_REPO_DIR}"
BUILD_DIR="${2:-$REPO_DIR/build-macos}"

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This script must be run on macOS."
    exit 1
fi

if [[ ! -d "$REPO_DIR/.git" ]]; then
    echo "Repository not found: $REPO_DIR"
    echo "Usage: $0 [repo_dir] [build_dir]"
    exit 1
fi

echo "[1/3] Configure"
cmake -S "$REPO_DIR" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=OFF

echo "[2/3] Build"
cmake --build "$BUILD_DIR" -j"$(sysctl -n hw.ncpu)"

echo "[3/3] Package"
cpack --config "$BUILD_DIR/CPackConfig.cmake" -G DragNDrop

printf "\nPackages generated in: %s\n" "$BUILD_DIR"
