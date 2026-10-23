#!/usr/bin/env bash
# Kept for CI/muscle memory: the macOS build now lives in build-scripts/build-mac.sh.
# MACOS_ARCH=arm64|x86_64 maps to --arch; extra arguments are passed through.
set -e
exec "$(dirname "$0")/../build-scripts/build-mac.sh" --arch "${MACOS_ARCH:-arm64}" "$@"
