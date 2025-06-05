#!/usr/bin/env bash

set -e

MPV_INSTALL_PREFIX="$(pwd)/build/out/mpv"
MPV_VARIANT="1"

if [[ -d "./build/${MPV_VARIANT}" ]] ; then
    rm -rf "./build/${MPV_VARIANT}"
fi

# --werror \ turned this off because of some annoying thing with lcms2.h
# --cross-file ci/x86_64-cross-file.txt \ # add this if you want to cross compile
# --cross-file ci/x86_64-cross-file.txt \
meson setup build \
    --wrap-mode=forcefallback \
    -Dprefix="${MPV_INSTALL_PREFIX}" \
    -Ddefault_library=shared \
    -D{c_args,objc_args}="-Wno-error=deprecated -Wno-error=deprecated-declarations" \
    -D{libmpv,mac}=true \
    -D{gl,iconv,lcms2,plain-gl,zlib}=enabled \
    -Djpeg=disabled \
    -Dtests=false \
    -Dlibass:mac=true \
    -Dlibass:asm=enabled \
    -Dlibass:fontconfig=disabled \
    -Dlcms2:fastfloat=true \
    -Dlcms2:jpeg=disabled \
    -Dlcms2:tiff=disabled \
    -Dlibusb:tests=false \
    -Dlibusb:examples=false \
    -Dlibplacebo:mac=true \
    -Dlibplacebo:demos=false \
    -Dlibplacebo:lcms=enabled \
    -Dlibplacebo:shaderc=enabled \
    -Dlibplacebo:d3d11=disabled \
    -Dlibplacebo:dovi=disabled \
    -Dlibplacebo:libdovi=disabled \
    -Dlibplacebo:xxhash=disabled \
    -Dlibplacebo:vulkan=enabled \
    -Dxxhash:inline-all=true \
    -Dxxhash:cli=false \
    -Ddrm=disabled \
    -Dlibarchive=disabled \
    -Drubberband=disabled \
    -Dwayland=disabled \
    -Dx11=disabled \
    -Dwin32-smtc=disabled \
    -Dvulkan=enabled \
    -Dlua=disabled \
    -D{cocoa,coreaudio,gl-cocoa,videotoolbox-gl,videotoolbox-pl}=enabled \
    -D{swift-build,macos-cocoa-cb,macos-media-player,macos-touchbar}=enabled 

meson compile -C build -j4
meson install -C build

python build-scripts/fixDeps.py $MPV_INSTALL_PREFIX/lib/*.dylib
