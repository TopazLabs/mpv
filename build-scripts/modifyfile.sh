#!/usr/bin/env bash

sed -i '' '/meson setup build \\/a\
    --cross-file ci/x86_64-cross-file.txt \\\
    ' /Users/joshyoo/Desktop/Topaz-Code/mpv/ci/build-macos.sh
