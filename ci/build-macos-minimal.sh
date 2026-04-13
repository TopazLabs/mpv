#!/usr/bin/env bash

set -e

. ./ci/build-common.sh

MACOS_ARCH="${MACOS_ARCH:-arm64}"
MACOS_MIN_VERSION="11.0"

case "${MACOS_ARCH}" in
    arm64)
        DEFAULT_CONAN_OUTPUT_DIR="./conan"
        DEFAULT_BUILD_DIR="build-minimal"
        DEFAULT_MPV_INSTALL_PREFIX="${HOME}/out/mpv-minimal"
        DEFAULT_PACKAGE_OUTPUT_DIR="./build-minimal/package-macos-armv8"
        RUNTIME_LAYOUT_DIR="builds-arm"
        CONAN_HOST_PROFILE="./build-scripts/profile_mac_armv8"
        MESON_CROSS_ARGS=()
        DEFAULT_SWIFT_FLAGS="-target arm64-apple-macos11.0"
        ;;
    x86_64)
        DEFAULT_CONAN_OUTPUT_DIR="./conan-x86_64"
        DEFAULT_BUILD_DIR="build-minimal-x86_64"
        DEFAULT_MPV_INSTALL_PREFIX="${HOME}/out/mpv-minimal-x86_64"
        DEFAULT_PACKAGE_OUTPUT_DIR="./build-minimal-x86_64/package-macos-x86_64"
        RUNTIME_LAYOUT_DIR="builds-x86"
        CONAN_HOST_PROFILE="./build-scripts/profile_mac14.0"
        MESON_CROSS_ARGS=(--cross-file "ci/x86_64-cross-file.txt")
        DEFAULT_SWIFT_FLAGS="-target x86_64-apple-macos11.0"
        ;;
    *)
        echo "Unsupported MACOS_ARCH '${MACOS_ARCH}'. Expected 'arm64' or 'x86_64'." >&2
        exit 1
        ;;
esac

CONAN_OUTPUT_DIR="${CONAN_OUTPUT_DIR:-${DEFAULT_CONAN_OUTPUT_DIR}}"
BUILD_DIR="${BUILD_DIR:-${DEFAULT_BUILD_DIR}}"
MPV_INSTALL_PREFIX="${MPV_INSTALL_PREFIX:-${DEFAULT_MPV_INSTALL_PREFIX}}"
PACKAGE_OUTPUT_DIR="${PACKAGE_OUTPUT_DIR:-${DEFAULT_PACKAGE_OUTPUT_DIR}}"
SWIFT_FLAGS="${SWIFT_FLAGS:-${DEFAULT_SWIFT_FLAGS}}"
SUBPROJECTS_DIR="subprojects"
LIBASS_WRAP="${SUBPROJECTS_DIR}/libass.wrap"

if command -v meson >/dev/null 2>&1 ; then
    MESON=(meson)
elif python3 -c "import mesonbuild.mesonmain" >/dev/null 2>&1 ; then
    MESON=(python3 -m mesonbuild.mesonmain)
else
    echo "Meson was not found. Install it or make it available on PATH before building." >&2
    exit 1
fi

if [[ -d "${BUILD_DIR}" ]] ; then
    rm -rf "${BUILD_DIR}"
fi

if [[ -d "${MPV_INSTALL_PREFIX}" ]] ; then
    rm -rf "${MPV_INSTALL_PREFIX}"
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

DEPENDENCY_OUTPUT_DIR=

if [[ -f "${CONAN_OUTPUT_DIR}/conanbuild.sh" ]] ; then
    . "${CONAN_OUTPUT_DIR}/conanbuild.sh"
    DEPENDENCY_OUTPUT_DIR="${CONAN_OUTPUT_DIR}"
    if [[ -d "${CONAN_OUTPUT_DIR}/lib/pkgconfig" ]] ; then
        python3 - "${CONAN_OUTPUT_DIR}" <<'PY'
from pathlib import Path
import sys

output_dir = Path(sys.argv[1]).resolve()
pkgconfig_dir = output_dir / "lib" / "pkgconfig"

for pc in pkgconfig_dir.glob("*.pc"):
    text = pc.read_text()
    lines = text.splitlines()
    changed = False
    for idx, line in enumerate(lines[:4]):
        if line.startswith("prefix="):
            expected = f"prefix={output_dir}"
            if line != expected:
                lines[idx] = expected
                changed = True
        elif line.startswith("libdir=") and "./builds" in line:
            lines[idx] = "libdir=${prefix}/lib"
            changed = True
        elif line.startswith("includedir=") and "./builds" in line:
            lines[idx] = "includedir=${prefix}/include"
            changed = True
    if changed:
        pc.write_text("\n".join(lines) + "\n")
PY
    fi
    PKG_CONFIG_PATHS="${CONAN_OUTPUT_DIR}:${CONAN_OUTPUT_DIR}/lib/pkgconfig"
else
    echo "No Conan output found at ${CONAN_OUTPUT_DIR}." >&2
    echo "Run 'conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ${CONAN_HOST_PROFILE} -of ${CONAN_OUTPUT_DIR}' first." >&2
    exit 1
fi

PKG_CONFIG_PATH="${PKG_CONFIG_PATHS}:${PKG_CONFIG_PATH}" \
DYLD_LIBRARY_PATH="${DEPENDENCY_OUTPUT_DIR}/lib:${DYLD_LIBRARY_PATH}" \
MACOSX_DEPLOYMENT_TARGET="${MACOS_MIN_VERSION}" \
CFLAGS="-mmacosx-version-min=${MACOS_MIN_VERSION} -I${DEPENDENCY_OUTPUT_DIR}/include ${CFLAGS}" \
LDFLAGS="-mmacosx-version-min=${MACOS_MIN_VERSION} ${LDFLAGS}" \
CC="${CC}" CXX="${CXX}" \
"${MESON[@]}" setup "${BUILD_DIR}" "${MESON_CROSS_ARGS[@]}" --force-fallback-for=libass $common_args \
  -Dprefix="${MPV_INSTALL_PREFIX}" \
  -Dtests=false \
  -Dobjc_args="-Wno-error=deprecated -Wno-error=deprecated-declarations -mmacosx-version-min=${MACOS_MIN_VERSION}" \
  -Dlibass:coretext=enabled \
  -Dlibass:fontconfig=disabled \
  -Dgl=enabled \
  -Diconv=enabled \
  -Dplain-gl=enabled \
  -Dzlib=enabled \
  -Dcocoa=enabled \
  -Dcoreaudio=enabled \
  -Dgl-cocoa=enabled \
  -Dswift-build=enabled \
  -Dmacos-cocoa-cb=enabled \
  -Dcplugins=disabled \
  -Djavascript=disabled \
  -Djpeg=disabled \
  -Dlcms2=disabled \
  -Dlibarchive=disabled \
  -Dlibavdevice=disabled \
  -Dlibbluray=disabled \
  -Drubberband=disabled \
  -Duchardet=disabled \
  -Dvapoursynth=disabled \
  -Dzimg=disabled \
  -Dlua=disabled \
  -Dvulkan=disabled \
  -Dvideotoolbox-gl=disabled \
  -Dvideotoolbox-pl=disabled \
  -Dmacos-media-player=disabled \
  -Dmacos-touchbar=disabled \
  -Dswift-flags="${SWIFT_FLAGS}" \
  -Ddrm=disabled \
  -Dwayland=disabled \
  -Dx11=disabled

"${MESON[@]}" compile -C "${BUILD_DIR}" -j4
"${MESON[@]}" install -C "${BUILD_DIR}"

if [[ -f "${DEPENDENCY_OUTPUT_DIR}/conanrun.sh" ]] ; then
    . "${DEPENDENCY_OUTPUT_DIR}/conanrun.sh"
fi

./ci/package-macos.sh "${MPV_INSTALL_PREFIX}" "${DEPENDENCY_OUTPUT_DIR}" "${PACKAGE_OUTPUT_DIR}"
echo "Success: minimal macOS package directory: ${PACKAGE_OUTPUT_DIR}"

python3 - "${DEPENDENCY_OUTPUT_DIR}" "${BUILD_DIR}/${RUNTIME_LAYOUT_DIR}" <<'PY'
from pathlib import Path
import shutil
import sys

src = Path(sys.argv[1]).resolve()
dst = Path(sys.argv[2])

if dst.exists():
    shutil.rmtree(dst)

for name in ("lib", "bin"):
    src_path = src / name
    if src_path.exists():
        shutil.copytree(src_path, dst / name, symlinks=True)
PY

ln -sfn "${DEPENDENCY_OUTPUT_DIR}" "./${RUNTIME_LAYOUT_DIR}"

echo "--> Testing minimal build runtime"
"./${BUILD_DIR}/mpv" -v --no-config
