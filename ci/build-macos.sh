#!/usr/bin/env bash

set -e

. ./ci/build-common.sh

FFMPEG_SYSROOT="${HOME}/deps/sysroot"
CONAN_OUTPUT_DIR="${CONAN_OUTPUT_DIR:-./conan}"
MPV_INSTALL_PREFIX="${HOME}/out/mpv"
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
    PKG_CONFIG_PATHS="${FFMPEG_SYSROOT}/lib/pkgconfig"
else
    echo "No Conan output found at ${CONAN_OUTPUT_DIR} and no legacy sysroot at ${FFMPEG_SYSROOT}." >&2
    echo "Run 'conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ./build-scripts/profile_mac_armv8 -of ./conan' first." >&2
    exit 1
fi

PKG_CONFIG_PATH="${PKG_CONFIG_PATHS}:${PKG_CONFIG_PATH}" \
DYLD_LIBRARY_PATH="${CONAN_OUTPUT_DIR}/lib:${DYLD_LIBRARY_PATH}" \
CFLAGS="-I${CONAN_OUTPUT_DIR}/include ${CFLAGS}" \
CC="${CC}" CXX="${CXX}" \
"${MESON[@]}" setup build --force-fallback-for=libass $common_args \
  -Dprefix="${MPV_INSTALL_PREFIX}" \
  -Dobjc_args="-Wno-error=deprecated -Wno-error=deprecated-declarations" \
  -D{gl,iconv,lcms2,lua,jpeg,plain-gl,zlib}=enabled \
  -D{cocoa,coreaudio,gl-cocoa,videotoolbox-gl,videotoolbox-pl}=enabled \
  -D{swift-build,macos-cocoa-cb,macos-media-player,macos-touchbar,vulkan}=enabled \
  -Dswift-flags="${SWIFT_FLAGS}"

"${MESON[@]}" compile -C build -j4
"${MESON[@]}" install -C build

if [[ -f "${CONAN_OUTPUT_DIR}/conanrun.sh" ]] ; then
    . "${CONAN_OUTPUT_DIR}/conanrun.sh"
fi

# Keep a runnable copy of the Conan runtime inside build/ so mpv can
# be launched directly from that directory.
python3 - "${CONAN_OUTPUT_DIR}" "build/builds-arm" <<'PY'
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

ln -sfn "${CONAN_OUTPUT_DIR}" ./builds-arm

./build/mpv -v --no-config
