#!/usr/bin/env bash

set -e

. ./ci/build-common.sh

FFMPEG_SYSROOT="${HOME}/deps/sysroot"
CONAN_OUTPUT_DIR="${CONAN_OUTPUT_DIR:-./conan}"
BUILD_DIR="${BUILD_DIR:-build-minimal}"
MPV_INSTALL_PREFIX="${HOME}/out/mpv-minimal"
PACKAGE_OUTPUT_DIR="${PACKAGE_OUTPUT_DIR:-./build-minimal/package-macos-armv8}"
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
elif [[ -d "${FFMPEG_SYSROOT}/lib/pkgconfig" ]] ; then
    DEPENDENCY_OUTPUT_DIR="${FFMPEG_SYSROOT}"
    PKG_CONFIG_PATHS="${FFMPEG_SYSROOT}/lib/pkgconfig"
else
    echo "No Conan output found at ${CONAN_OUTPUT_DIR} and no legacy sysroot at ${FFMPEG_SYSROOT}." >&2
    echo "Run 'conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ./build-scripts/profile_mac_armv8 -of ./conan' first." >&2
    exit 1
fi

PKG_CONFIG_PATH="${PKG_CONFIG_PATHS}:${PKG_CONFIG_PATH}" \
DYLD_LIBRARY_PATH="${DEPENDENCY_OUTPUT_DIR}/lib:${DYLD_LIBRARY_PATH}" \
CFLAGS="-I${DEPENDENCY_OUTPUT_DIR}/include ${CFLAGS}" \
CC="${CC}" CXX="${CXX}" \
"${MESON[@]}" setup "${BUILD_DIR}" --force-fallback-for=libass $common_args \
  -Dprefix="${MPV_INSTALL_PREFIX}" \
  -Dtests=false \
  -Dobjc_args="-Wno-error=deprecated -Wno-error=deprecated-declarations" \
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
  -Ddrm=disabled \
  -Dwayland=disabled \
  -Dx11=disabled

"${MESON[@]}" compile -C "${BUILD_DIR}" -j4
"${MESON[@]}" install -C "${BUILD_DIR}"

if [[ -f "${DEPENDENCY_OUTPUT_DIR}/conanrun.sh" ]] ; then
    . "${DEPENDENCY_OUTPUT_DIR}/conanrun.sh"
fi

./ci/package-macos.sh "${MPV_INSTALL_PREFIX}" "${DEPENDENCY_OUTPUT_DIR}" "${PACKAGE_OUTPUT_DIR}"
echo "minimal macOS package directory: ${PACKAGE_OUTPUT_DIR}"

python3 - "${DEPENDENCY_OUTPUT_DIR}" "${BUILD_DIR}/builds-arm" <<'PY'
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

ln -sfn "${DEPENDENCY_OUTPUT_DIR}" ./builds-arm

"./${BUILD_DIR}/mpv" -v --no-config
