#!/usr/bin/env bash

set -e

. ./ci/build-common.sh

FFMPEG_SYSROOT="${HOME}/deps/sysroot"
CONAN_OUTPUT_DIR="${CONAN_OUTPUT_DIR:-./conan}"
MPV_INSTALL_PREFIX="${HOME}/out/mpv"
SUBPROJECTS_DIR="subprojects"
LIBASS_WRAP="${SUBPROJECTS_DIR}/libass.wrap"

if [[ -d "./build" ]] ; then
    rm -rf "./build"
fi

mkdir -p "${SUBPROJECTS_DIR}"
if [[ ! -f "${LIBASS_WRAP}" ]] ; then
    cat > "${LIBASS_WRAP}" <<'EOF'
[wrap-git]
url = https://github.com/libass/libass
revision = master
depth = 1
EOF
fi

if [[ -f "${CONAN_OUTPUT_DIR}/conanbuild.sh" ]] ; then
    . "${CONAN_OUTPUT_DIR}/conanbuild.sh"
    PKG_CONFIG_ROOT="${CONAN_OUTPUT_DIR}"
elif [[ -d "${FFMPEG_SYSROOT}/lib/pkgconfig" ]] ; then
    PKG_CONFIG_ROOT="${FFMPEG_SYSROOT}/lib/pkgconfig"
else
    echo "No Conan output found at ${CONAN_OUTPUT_DIR} and no legacy sysroot at ${FFMPEG_SYSROOT}." >&2
    echo "Run 'conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ./build-scripts/profile_mac_armv8 -of ./conan' first." >&2
    exit 1
fi

PKG_CONFIG_PATH="${PKG_CONFIG_ROOT}:${PKG_CONFIG_PATH}" \
DYLD_LIBRARY_PATH="${CONAN_OUTPUT_DIR}/lib:${DYLD_LIBRARY_PATH}" \
CC="${CC}" CXX="${CXX}" \
meson setup build --force-fallback-for=libass $common_args \
  -Dprefix="${MPV_INSTALL_PREFIX}" \
  -Dobjc_args="-Wno-error=deprecated -Wno-error=deprecated-declarations" \
  -D{gl,iconv,lcms2,lua,jpeg,plain-gl,zlib}=enabled \
  -D{cocoa,coreaudio,gl-cocoa,videotoolbox-gl,videotoolbox-pl}=enabled \
  -D{swift-build,macos-cocoa-cb,macos-media-player,macos-touchbar,vulkan}=enabled \
  -Dswift-flags="${SWIFT_FLAGS}"

meson compile -C build -j4
meson install -C build

if [[ -f "${CONAN_OUTPUT_DIR}/conanrun.sh" ]] ; then
    . "${CONAN_OUTPUT_DIR}/conanrun.sh"
fi

./build/mpv -v --no-config
