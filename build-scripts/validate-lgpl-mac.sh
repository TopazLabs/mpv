#!/usr/bin/env bash
# Validates that a macOS mpv build is LGPL: -Dgpl=false, FFmpeg configured without
# GPL/nonfree parts, no GPL-only libraries shipped, every LGPL dependency linked
# dynamically, and nothing picked up from MacPorts/Homebrew.
#
# Usage: build-scripts/validate-lgpl-mac.sh [build dir] [full package dir]
# Exits non-zero if any check fails.
set -uo pipefail

BUILD_DIR="${1:-build-mac}"
PKG="${2:-${BUILD_DIR}/package-macos-armv8}"
FAILURES=0

pass() { echo "  [PASS] $1"; }
fail() { echo "  [FAIL] $1"; FAILURES=$((FAILURES + 1)); }
check() { if eval "$1"; then pass "$2"; else fail "$2"; fi; }

LIB="${PKG}/lib"
MPV_LIB="$(ls "${LIB}"/libmpv.[0-9]*.dylib 2>/dev/null | head -1)"
ASS_LIB="$(ls "${LIB}"/libass.[0-9]*.dylib 2>/dev/null | head -1)"
PL_LIB="$(ls "${LIB}"/libplacebo.[0-9]*.dylib 2>/dev/null | head -1)"
[[ -n "${MPV_LIB}" && -n "${ASS_LIB}" && -n "${PL_LIB}" ]] || { echo "libmpv/libass/libplacebo not found in ${LIB}" >&2; exit 1; }

deps() { otool -L "$1" | tail -n +2 | awk '{print $1}'; }
defines() { nm -gU "$1" 2>/dev/null | awk '{print $NF}'; }
strs() { strings -a "$1"; }

echo "== 1. meson configuration"
GPL_OPT="$(python3 - "${BUILD_DIR}" <<'PY'
import json, sys
for o in json.load(open(f"{sys.argv[1]}/meson-info/intro-buildoptions.json")):
    if o["name"] == "gpl" and not o.get("subproject"):
        print(str(o["value"]).lower())
PY
)"
check '[[ "${GPL_OPT}" == "false" ]]' "meson option gpl=false"
check "grep -q '#define HAVE_GPL 0' '${BUILD_DIR}/config.h'" "config.h: HAVE_GPL 0"
FULLCONFIG="$(sed -n 's/^#define FULLCONFIG "\(.*\)"/\1/p' "${BUILD_DIR}/config.h")"
check '[[ -n "${FULLCONFIG}" && " ${FULLCONFIG} " != *" gpl "* ]]' "config.h: 'gpl' not in enabled features"

echo "== 2. libmpv"
MPV_STRS="$(strs "${MPV_LIB}")"
check 'grep -qF -- "-Dgpl=false" <<<"${MPV_STRS}"' "libmpv configuration contains -Dgpl=false"
check '! grep -qF -- "-Dgpl=true" <<<"${MPV_STRS}"' "libmpv configuration does not contain -Dgpl=true"

echo "== 3. FFmpeg license"
FF_LIBS=("${LIB}"/libav{codec,filter,format,util}.[0-9]*.dylib "${LIB}"/libsw{resample,scale}.[0-9]*.dylib)
check '[[ ${#FF_LIBS[@]} -ge 5 && -e "${FF_LIBS[0]}" ]]' "found FFmpeg dylibs ($(basename -a "${FF_LIBS[@]}" 2>/dev/null | tr '\n' ' '))"
for f in "${FF_LIBS[@]}"; do
    [[ -e "$f" && ! -L "$f" ]] || continue
    S="$(strs "$f")"
    check 'grep -q "LGPL version 2.1 or later" <<<"${S}"' "$(basename "$f"): license is LGPL version 2.1 or later"
    check '! grep -qE "(^|[^L])GPL version [23] or later|nonfree and unredistributable|LGPL version 3" <<<"${S}"' "$(basename "$f"): no GPL/LGPLv3/nonfree license string"
done
AVUTIL="$(ls "${LIB}"/libavutil.[0-9]*.dylib | head -1)"
CFG="$(strs "${AVUTIL}" | grep -m1 -- '--toolchain' || true)"
for bad in --enable-gpl --enable-nonfree --enable-version3 libx264 libx265 postproc rubberband; do
    check '[[ "${CFG}" != *"${bad}"* ]]' "FFmpeg configure line has no ${bad}"
done

echo "== 4. no GPL-only libraries shipped"
GPL_FILES="$(find "${PKG}" -type f \( -name 'libpostproc*' -o -name 'libx264*' -o -name 'libx265*' -o -name 'librubberband*' -o -name 'libxvidcore*' -o -name 'libdvdnav*' -o -name 'libdvdread*' -o -name 'libcdio*' -o -name 'libjack*' -o -name 'libcaca*' \) | tr '\n' ' ')"
check '[[ -z "${GPL_FILES}" ]]' "no GPL-only libraries (${GPL_FILES})"

echo "== 5. dynamic linking"
MPV_DEPS="$(deps "${MPV_LIB}")"
for l in libavcodec libavformat libavutil libavfilter libswresample libswscale libplacebo libass; do
    check 'grep -q "/${l}\.[0-9.]*dylib" <<<"${MPV_DEPS}"' "libmpv imports ${l} dylib"
done
ASS_DEPS="$(deps "${ASS_LIB}")"
for l in libfreetype libharfbuzz libfribidi; do
    check 'grep -q "/${l}\.[0-9.]*dylib" <<<"${ASS_DEPS}"' "$(basename "${ASS_LIB}") imports ${l} dylib"
done
check 'grep -q "/libiconv\.[0-9.]*dylib" <<<"${MPV_DEPS}"' "libmpv imports conan's libiconv dylib"
check '! grep -q "^/usr/lib/libiconv" <<<"${ASS_DEPS}${MPV_DEPS}"' "libmpv/libass do not use the system libiconv"
# LGPL code must not be compiled into libmpv/libass: its symbols have to be imports
MPV_DEFS="$(defines "${MPV_LIB}")"
for sym in avcodec_open2 av_frame_alloc pl_renderer_create ass_library_init FT_Init_FreeType hb_buffer_create fribidi_get_par_embedding_levels; do
    check '! grep -qx "_${sym}" <<<"${MPV_DEFS}"' "libmpv does not define ${sym}"
done
ASS_DEFS="$(defines "${ASS_LIB}")"
for sym in FT_Init_FreeType hb_buffer_create fribidi_get_par_embedding_levels; do
    check '! grep -qx "_${sym}" <<<"${ASS_DEFS}"' "$(basename "${ASS_LIB}") does not define ${sym}"
done
check 'grep -qx "_pl_renderer_create" <<<"$(defines "${PL_LIB}")"' "$(basename "${PL_LIB}") defines the libplacebo API"
check 'grep -qx "_ass_library_init" <<<"${ASS_DEFS}"' "$(basename "${ASS_LIB}") defines the libass API"

echo "== 6. nothing from the host"
LEAKS=""
for f in "${PKG}"/bin/mpv "${LIB}"/*.dylib; do
    [[ -L "$f" ]] && continue
    for d in $(deps "$f"); do
        case "$d" in
            /usr/lib/*|/System/Library/*|@loader_path/*|@executable_path/*) ;;
            "$f") ;;
            *) LEAKS="${LEAKS} $(basename "$f"):${d}" ;;
        esac
    done
    for rp in $(otool -l "$f" | awk '/cmd LC_RPATH/{getline; getline; print $2}'); do
        LEAKS="${LEAKS} $(basename "$f"):rpath:${rp}"
    done
done
check '[[ -z "${LEAKS}" ]]' "every dependency is system or package-relative, no rpaths (${LEAKS})"
# (meson may still probe host tools such as sdl2-config for disabled features; what counts
# is that no compile or link command of the build uses a host prefix)
check '! grep -E "^[[:space:]]*(ARGS|LINK_ARGS|LINK_PATH)[[:space:]]*=" "${BUILD_DIR}/build.ninja" | grep -qE "/opt/local|/usr/local|/opt/homebrew"' "no compile/link command uses MacPorts/Homebrew paths"

echo "== 7. architecture / deployment target"
MINOS="$(otool -l "${MPV_LIB}" | awk '/LC_BUILD_VERSION/{f=1} f&&/minos/{print $2; exit}')"
check '[[ "${MINOS}" == "11.0" ]]' "libmpv minos is 11.0 (${MINOS})"

echo
if [[ ${FAILURES} -gt 0 ]]; then
    echo "LGPL VALIDATION FAILED (${FAILURES} check(s))"
    exit 1
fi
echo "LGPL VALIDATION PASSED"
