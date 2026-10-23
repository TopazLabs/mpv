#!/usr/bin/env bash
# macOS LGPL build of libmpv (libmpv.2.dylib) + mpv, same dependency layout as Windows.
#
#   build-scripts/build-mac.sh [--arch arm64|x86_64] [--clean] [--app-lockfile <path>]
#
# - All deps come from conan (--build=never), pinned by videoenhanceai's mac lockfile, so
#   topaz-ffmpeg, zlib, lcms, libiconv, ... are the exact binaries the app uses. A missing
#   binary is an error: build and publish it from topaz-conan-src (see BUILDING.md).
# - zlib and lcms are static (as in the app); FFmpeg, freetype, harfbuzz, fribidi and
#   libiconv are shared conan packages.
# - libass and libplacebo are meson subprojects, built shared (libass.dylib, libplacebo.dylib).
# - Packages (build-scripts/package-mac.py):
#     <build>/package-macos-<arch>       full, self-contained runtime (local testing)
#     <build>/package-macos-<arch>-slim  only the files published to topaz-conan (mpv_dll)
# - The package is checked by validate-lgpl-mac.sh; the script fails if it is not LGPL.
set -euo pipefail

ARCH=arm64
CLEAN=0
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_LOCKFILE=""
MACOS_MIN_VERSION="11.0"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --arch) ARCH="$2"; shift 2 ;;
        --clean) CLEAN=1; shift ;;
        --app-lockfile) APP_LOCKFILE="$2"; shift 2 ;;
        *) echo "unknown argument: $1" >&2; exit 1 ;;
    esac
done

cd "${ROOT}"
case "${ARCH}" in
    arm64)
        PKG_ARCH=armv8
        HOST_PROFILE=build-scripts/profile_mac_armv8
        CONAN_DIR=conan
        BUILD_DIR=build-mac
        CROSS=()
        SWIFT_TARGET="arm64-apple-macos${MACOS_MIN_VERSION}"
        ;;
    x86_64)
        PKG_ARCH=x86_64
        HOST_PROFILE=build-scripts/profile_mac14.0
        CONAN_DIR=conan-x86_64
        BUILD_DIR=build-mac-x86_64
        CROSS=(--cross-file ci/x86_64-cross-file.txt)
        SWIFT_TARGET="x86_64-apple-macos${MACOS_MIN_VERSION}"
        ;;
    *) echo "unsupported --arch '${ARCH}' (arm64 or x86_64)" >&2; exit 1 ;;
esac
BUILD_PROFILE=build-scripts/profile_mac_armv8
APP_LOCKFILE="${APP_LOCKFILE:-${ROOT}/../videoenhanceai/deploy/lockfiles/conanlock_mac_${PKG_ARCH}}"
[[ -f "${APP_LOCKFILE}" ]] || { echo "videoenhanceai lockfile not found: ${APP_LOCKFILE} (pass --app-lockfile)" >&2; exit 1; }
PACKAGE_DIR="${BUILD_DIR}/package-macos-${PKG_ARCH}"

if [[ "${CLEAN}" == 1 ]]; then
    rm -rf "${CONAN_DIR}" "${BUILD_DIR}"
fi

# 1. conan deps. Never built here: every binary must come from topaz-conan, built and
#    published by topaz-conan-src.
conan install build-scripts/conanfile.py \
    -pr:h "${HOST_PROFILE}" -pr:b "${BUILD_PROFILE}" \
    --lockfile "${APP_LOCKFILE}" --lockfile-partial \
    --build=never -of "${CONAN_DIR}" ||
    { echo "conan install failed. If a binary is missing, build and publish it from topaz-conan-src; do not use --build=missing." >&2; exit 1; }
CONAN_ABS="$(cd "${CONAN_DIR}" && pwd)"
LIB3="${CONAN_ABS}/lib3rdparty"

# 2. topaz-ffmpeg: its dylibs and .pc files use a relative ./builds-<arch> prefix. Stage a
#    copy with real @rpath install names so everything linked against it is relocatable.
python3 build-scripts/package-mac.py stage-ffmpeg "${LIB3}/topaz-ffmpeg"
FFMPEG_STAGE="${LIB3}/topaz-ffmpeg"

# 3. build environment. Only conan locations are searched: PKG_CONFIG_LIBDIR (not _PATH)
#    keeps MacPorts/Homebrew .pc files out, and no /opt/local include or lib dir is added.
export PKG_CONFIG_LIBDIR="${FFMPEG_STAGE}/lib/pkgconfig:${CONAN_ABS}"
unset PKG_CONFIG_PATH
export MACOSX_DEPLOYMENT_TARGET="${MACOS_MIN_VERSION}"
unset CFLAGS CXXFLAGS OBJCFLAGS LDFLAGS
# Compiler/linker flags go into a meson machine file: for a cross build (x86_64) meson
# ignores CFLAGS/LDFLAGS for the host machine.
# - meson's dependency('iconv') ignores pkg-config: point the compiler at conan's libiconv
# - the build-tree binaries find conan's dylibs through rpaths (stripped when packaging)
FLAGS_FILE="${CONAN_ABS}/meson-flags.ini"
python3 - "${FLAGS_FILE}" "${MACOS_MIN_VERSION}" "${LIB3}" <<'PY'
import glob, sys
out, minver, lib3 = sys.argv[1:]
cflags = [f"-mmacosx-version-min={minver}", f"-I{lib3}/libiconv/include"]
ldflags = [f"-mmacosx-version-min={minver}", f"-L{lib3}/libiconv/lib"]
ldflags += [f"-Wl,-rpath,{d}" for d in sorted(glob.glob(f"{lib3}/*/lib"))]
lines = ["[built-in options]"]
objc = ["-Wno-error=deprecated", "-Wno-error=deprecated-declarations"]
for lang in ("c", "cpp", "objc", "objcpp"):
    lines.append(f"{lang}_args = {cflags + (objc if lang == 'objc' else [])!r}")
    lines.append(f"{lang}_link_args = {ldflags!r}")
open(out, "w").write("\n".join(lines) + "\n")
PY
if [[ ${#CROSS[@]} -gt 0 ]]; then
    MACHINE_FILES=("${CROSS[@]}" --cross-file "${FLAGS_FILE}")
else
    MACHINE_FILES=(--native-file "${FLAGS_FILE}")
fi
# FFmpeg's remaining runtime deps (libvpx, ...) for running the freshly built binaries
set +u
. "${CONAN_ABS}/conanrun.sh"
set -u

if command -v meson >/dev/null 2>&1; then
    MESON=(meson)
else
    MESON=(python3 -m mesonbuild.mesonmain)
fi

# 4. libass / libplacebo subprojects, pinned like Windows
mkdir -p subprojects
cat > subprojects/libass.wrap <<'EOF'
[wrap-git]
url = https://github.com/libass/libass.git
revision = 0.17.4
depth = 1
EOF
cat > subprojects/libplacebo.wrap <<'EOF'
[wrap-git]
url = https://code.videolan.org/videolan/libplacebo.git
revision = v7.360.1
depth = 1
clone-recursive = true
EOF

# 5. configure + build
MESON_ARGS=(
    --buildtype=release --wrap-mode=default -Ddefault_library=shared
    --force-fallback-for=libass,libplacebo
    -Dgpl=false -Dlibmpv=true -Dcplayer=true -Dtests=false
    -Dgl=enabled -Diconv=enabled -Dplain-gl=enabled -Dzlib=enabled
    -Dcocoa=enabled -Dcoreaudio=enabled -Dgl-cocoa=enabled
    -Dswift-build=enabled -Dmacos-cocoa-cb=enabled "-Dswift-flags=-target ${SWIFT_TARGET}"
    -Dcplugins=disabled -Djavascript=disabled -Dlibcurl=disabled -Djpeg=disabled
    -Dlcms2=disabled -Dlibarchive=disabled -Dlibavdevice=disabled -Dlibbluray=disabled
    -Drubberband=disabled -Duchardet=disabled -Dvapoursynth=disabled -Dzimg=disabled
    -Dlua=disabled -Dvulkan=disabled -Dvideotoolbox-gl=disabled -Dvideotoolbox-pl=disabled
    -Dmacos-media-player=disabled -Dmacos-touchbar=disabled
    -Ddrm=disabled -Dwayland=disabled -Dx11=disabled
    # both subprojects default to static in their own default_options
    -Dlibass:default_library=shared -Dlibass:coretext=enabled -Dlibass:fontconfig=disabled
    -Dlibass:libunibreak=disabled
    -Dlibplacebo:default_library=shared -Dlibplacebo:demos=false -Dlibplacebo:tests=false
    -Dlibplacebo:opengl=enabled -Dlibplacebo:lcms=enabled
    -Dlibplacebo:vulkan=disabled -Dlibplacebo:d3d11=disabled -Dlibplacebo:shaderc=disabled
    -Dlibplacebo:glslang=disabled -Dlibplacebo:dovi=disabled -Dlibplacebo:libdovi=disabled
    -Dlibplacebo:xxhash=disabled -Dlibplacebo:unwind=disabled
)
if [[ -f "${BUILD_DIR}/build.ninja" ]]; then
    "${MESON[@]}" setup --reconfigure "${BUILD_DIR}" "${MACHINE_FILES[@]}" "${MESON_ARGS[@]}"
else
    "${MESON[@]}" setup "${BUILD_DIR}" "${MACHINE_FILES[@]}" "${MESON_ARGS[@]}"
fi
"${MESON[@]}" compile -C "${BUILD_DIR}"

# 6. packages: full runtime + slim (what mpv_dll publishes)
python3 build-scripts/package-mac.py package "${BUILD_DIR}" "${CONAN_ABS}" "${PACKAGE_DIR}"
echo "Success: macOS package directory: ${PACKAGE_DIR}"

# 7. LGPL validation (fails the build)
build-scripts/validate-lgpl-mac.sh "${BUILD_DIR}" "${PACKAGE_DIR}"

python3 build-scripts/package-mac.py slim "${PACKAGE_DIR}" "${PACKAGE_DIR}-slim"
echo "Success: macOS slim package directory: ${PACKAGE_DIR}-slim"

# 8. runtime test from the full package (arm64 only: the Intel build is cross-compiled)
if [[ "${ARCH}" == "arm64" ]]; then
    echo "--> Testing macOS build runtime"
    "${PACKAGE_DIR}/bin/mpv" -v --no-config
fi
