# Building mpv (Topaz)

How to build this mpv fork as `libmpv` for Topaz apps (videoenhanceai), on **Windows** and
**macOS**. The two platforms use different scripts and package layouts; each has its own section
below. Read the section for your platform top to bottom before building.

| | Windows | macOS |
| --- | --- | --- |
| Entry point | `build-scripts/build-win.ps1` (PowerShell) | `ci/build-macos-minimal.sh` (bash) |
| Compiler | clang-cl + MSVC link, `/MD` | Apple clang |
| Conan profile | `build-scripts/profile_win2022` | `build-scripts/profile_mac_armv8` (arm64), `profile_mac14.0` (Intel) |
| Conan install | done by the script, `--build=never`, pinned by videoenhanceai's lockfile | run by hand before the script |
| Build dir | `build-win/` | `build-minimal/` (`build-minimal-x86_64/` for Intel) |
| Package to publish | `build-win/package-windows-x86_64-slim` | `build-minimal/package-macos-armv8-slim` (`…-x86_64-slim`) |
| `mpv_dll` version used by videoenhanceai | `0.41.0-20260923-1` | `0.41.0-20260923` |
| LGPL check | `build-scripts/validate-lgpl.ps1`, runs automatically | manual: `mpv -v` (see macOS section) |

Shared by both platforms: `build-scripts/conanfile.py` (Windows-only parts are inside
`if self.settings.os == "Windows"`), `meson.build` / `meson.options`, and the topaz-ffmpeg
package from topaz-conan. Publishing goes through
`topaz-conan-dev/prebuilt/mpv_dll/<version>/<platform folder>`.

## LGPL (both platforms)

Builds must stay LGPL. Never turn GPL on as a side effect of a build change.

- `meson.options` defaults `gpl` to `false`; both build scripts also pass `-Dgpl=false`.
  (`ci/build-common.sh` adds it for the mac flow; the mingw/Windows CI scripts no longer re-enable GPL.)
- topaz-ffmpeg is configured without `--enable-gpl` / `--enable-nonfree`.
- LGPL libraries must be linked dynamically: FFmpeg, libplacebo, fribidi, libiconv.
- A correct build prints `-Dgpl=false` in `Configuration:` of `mpv -v`, and `gpl` is **not** in
  `List of enabled features:` (mpv lists `gpl` there only when it is enabled).

---

# Windows

## Quick start

From a normal PowerShell in the repo root:

```powershell
powershell -ExecutionPolicy Bypass -File build-scripts/build-win.ps1 -Clean
```

The script must end with `LGPL VALIDATION PASSED` and a working `mpv.com -v` run. Without `-Clean`
it reuses `conan/` and `build-win/` (incremental rebuild).

Options:

- `-Clean`: delete `conan/` and `build-win/` first.
- `-AppLockfile <path>`: videoenhanceai's Windows lockfile. Default:
  `../videoenhanceai/deploy/lockfiles/conanlock_win2022` (videoenhanceai checked out next to this repo).
- `-BuildDir <dir>`: default `build-win`.

## Prerequisites

- Visual Studio 2022 with the **"C++ Clang Compiler for Windows"** component
  (`Microsoft.VisualStudio.Component.VC.Llvm.Clang`). mpv does not build with plain `cl`; the script
  enters the VS developer shell itself.
- conan 2, logged in to the `topaz-conan` remote.
- Python 3 on `PATH` (meson uses it; libplacebo's code generator needs it).
- git and network access (libass and libplacebo are cloned as meson subprojects).
- meson, ninja, pkgconf and nasm are **not** needed on `PATH`; they come from conan `tool_requires`.

## What the script does

1. `conan install build-scripts/conanfile.py` with `profile_win2022` (host and build),
   `--lockfile <AppLockfile> --lockfile-partial`, `--build=never`, output in `conan/`.
2. Rewrites topaz-ffmpeg's `.pc` files (shipped with a relative `output-conan` prefix) to point at
   `conan/lib3rdparty/topaz-ffmpeg`.
3. Loads `conan/conanbuild.ps1` (meson/ninja/pkgconf/nasm), sets `PKG_CONFIG_PATH` to the conan
   `.pc` files, and adds conan's libiconv to `INCLUDE`/`LIB` (meson's `dependency('iconv')` ignores
   pkg-config).
4. Writes `subprojects/libass.wrap` (libass `0.17.4`) and `subprojects/libplacebo.wrap`
   (libplacebo `v7.360.1`).
5. `meson setup` with `CC=CXX=clang-cl`, `WINDRES=llvm-rc`, `--wrap-mode=default`,
   `-Ddefault_library=shared`, `-Dgpl=false`, `-Dlibmpv=true`, d3d11 enabled, vulkan/lua/javascript/
   libarchive/rubberband/win32-smtc/jpeg disabled; then `meson compile`.
6. Stages the full package, regenerates `lib/mpv.lib` (see Gotchas), runs `validate-lgpl.ps1`,
   stages the slim package, and runs `mpv.com -v --no-config`.

How dependencies are found: there are no Windows-specific changes in `meson.build`. Every
`dependency()` call (mpv, libplacebo, libass) is resolved by pkg-config against conan's output;
meson falls back to a subproject only for libass and libplacebo, which are not conan packages.

| Used by | Dependency | Source |
| --- | --- | --- |
| mpv | FFmpeg (`avcodec-62`, …), zimg | topaz-ffmpeg (and its zimg) |
| mpv | zlib | conan `zlib/1.2.13`, static (same binary as videoenhanceai) |
| libplacebo | shaderc, spirv-cross | conan, shared |
| libplacebo | lcms2 | conan `lcms/2.17`, static (same binary as videoenhanceai) |
| libass | freetype, harfbuzz, fribidi, libiconv | conan, shared |
| mpv | libass, libplacebo | meson subprojects, forced shared (`-D<sub>:default_library=shared`) |

## Outputs

| Path | Contents | Use |
| --- | --- | --- |
| `build-win/package-windows-x86_64` | `mpv-2.dll`, `mpv.exe`, `mpv.com`, every runtime DLL from conan (FFmpeg, videoai/tvai, …), headers, `mpv.lib`, licenses | local testing; self-contained |
| `build-win/package-windows-x86_64-slim` | `bin/mpv-2.dll`, `bin/ass-9.dll`, `bin/libplacebo-360.dll`, `lib/mpv.lib`, `include/mpv/*.h`, `LICENSE.LGPL`, `Copyright`, `licenses/` (libass, libplacebo, zlib, lcms) | goes into `mpv_dll` |

The slim package has no `mpv.exe` (apps only use libmpv) and no dependency DLLs: those are
requirements of the `mpv_dll` recipe and come from their own topaz-conan packages.

## LGPL validation

`build-scripts/validate-lgpl.ps1` runs on the full package after every build and fails the build
unless:

- meson `gpl=false`, `config.h` has `HAVE_GPL 0`, and `mpv-2.dll` embeds `-Dgpl=false` and a feature
  list without `gpl`;
- every FFmpeg DLL embeds "LGPL version 2.1 or later", and FFmpeg's configure line has no
  `--enable-gpl`, `--enable-nonfree`, `--enable-version3`, x264, x265 or postproc;
- no GPL-only libraries (postproc, x264, x265, rubberband, dvdnav, cdio, jack, caca, …) are shipped;
- `mpv-2.dll` imports FFmpeg, libplacebo and libass as DLLs and defines none of their symbols;
  `ass-*.dll` imports freetype, harfbuzz, fribidi and iconv as DLLs; `libplacebo-*.dll` imports
  shaderc and spirv-cross as DLLs;
- `lib/mpv.lib` links against `mpv-2.dll` (not `mpv.exe`);
- `mpv-2.dll` uses the dynamic CRT (`/MD`), like FFmpeg.

Run it on its own from a VS developer shell:

```powershell
build-scripts/validate-lgpl.ps1 -BuildDir build-win -OutDir build-win/package-windows-x86_64
```

## Dependency policy

All dependencies are standardized across repos (mpv, videoenhanceai, …): **every dependency binary
is built and published by topaz-conan-src**; mpv only consumes binaries from topaz-conan.

- `build-win.ps1` uses `--build=never`. A missing binary is an error. Never fix it with
  `--build=missing`, `conan create` in this repo, or a hand-copied DLL; build it in topaz-conan-src.
- Versions and revisions come from videoenhanceai's lockfile (`-AppLockfile`), so mpv and the app
  resolve the same topaz-ffmpeg, zlib, lcms, … binaries. Deps that only mpv uses are pinned in
  `build-scripts/conanfile.py` and in the `mpv_dll` recipe.
- These three profiles must stay identical (msvc 194, cppstd 17, `/MD`, Release):
  mpv `build-scripts/profile_win2022`, videoenhanceai `deploy/win/profile_win2022`,
  topaz-conan-src `_util/conan/profile_win2022`.
- `tensorrt_rtx=True` for topaz-ffmpeg, as in videoenhanceai's `deploy/conanfile.py`.

mpv-only shared builds in topaz-conan-src (options in each recipe's `topaz_config.yml`,
`conan_create_args_win`; package ids as published 2026-10-22):

| Package | Options | Package id |
| --- | --- | --- |
| `freetype/2.13.2-topaz` | `shared=True`, `with_png/with_bzip2/with_brotli/with_zlib=False` | `328df2c7…` |
| `harfbuzz/8.3.0-topaz` | `shared=True`, `with_glib=False` (+ the freetype options) | `850c3ad2…` |
| `fribidi/1.0.13-topaz` | `shared=True` | `cc6fa260…` |
| `shaderc/2024.1` | `shared=True` | `c8e5dee5…` |
| `spirv-cross/1.3.268.0` | `shared=True` | `4065b4ff…` |
| `libiconv/1.17` | `shared=True` (already published; also used by imageenhance) | `19e318a8…` |

Building one in topaz-conan-src (from its repo root, on Windows):

```bash
pipenv run python _util/scripts/build_and_publish.py --conan-user '""' --conan-channel '""' \
    -pb _util/conan/profile_win2022 -ph _util/conan/profile_win2022 \
    -n harfbuzz -v 8.3.0-topaz -s --skip-default-config --normalize-conandata
```

- `-s` builds without uploading. Before publishing, check that the exported recipe revision is the
  one already on topaz-conan and that the package id matches the table (or is the intended new one).
- harfbuzz needs `--normalize-conandata`: its published revision was exported with a CRLF
  `conandata.yml`; without the flag conan-src exports a different recipe revision.
- harfbuzz's `topaz_config.yml` also builds its static variant, which needs freetype's default
  (static) binary: build freetype first without `--skip-default-config`.
- Publishing: **never pass `--allow-new-recipe`**, and never re-upload an existing package id. The
  pipeline uploads with `--package-query <profile settings>`, i.e. every local binary of that recipe
  that matches the profile, so publish from a clean conan cache (or CI agent) that only holds the
  intended binary.

## Release procedure (mpv_dll, Windows)

1. **Deps**: if a dependency changed, build (and publish) it from topaz-conan-src as above.
2. **Build**: `build-scripts/build-win.ps1 -Clean` must end with `LGPL VALIDATION PASSED`.
3. **Package**: copy `build-win/package-windows-x86_64-slim/*` into
   `topaz-conan-dev/prebuilt/mpv_dll/<version>/windows_x86_64_2022`. Use a **new** `mpv_dll` version
   whenever its `conanfile.py` changes: a new recipe revision under an existing version hides that
   version's already-published (macOS) binaries.
4. **Recipe**: keep the `mpv_dll` recipe's Windows `requirements()` / `configure()` in sync with
   `build-scripts/conanfile.py` (same refs, shared options, `tensorrt_rtx=True`). Requirements are
   runtime-only (`headers=False, libs=False, run=True`); consumers link only `mpv.lib`.
5. **Test in videoenhanceai before uploading**:
   - export locally (from the version folder):
     `conan export-pkg conanfile.py --name mpv_dll --version <version> -pr:h <app>/deploy/win/profile_win2022 -pr:b <app>/deploy/win/profile_win2022 --lockfile <app>/deploy/lockfiles/conanlock_win2022 --lockfile-partial -c user.path:folder_dir=windows_x86_64_2022`
   - bump `mpv_dll` for Windows in `deploy/conanfile.py`;
   - update the lockfile without moving other pins:
     `conan lock create deploy/conanfile.py -pr:h deploy/win/profile_win2022 -pr:b deploy/win/profile_win2022 --lockfile <committed lockfile> --lockfile-partial --lockfile-clean --lockfile-out deploy/lockfiles/conanlock_win2022`;
   - update `deploy/windows-installer/MPV.wxs` if the DLL set changed (its FFmpeg 7 entries belong
     to Qt Multimedia, not mpv; keep them);
   - build the app in a fresh build dir and check: `tutils.dll` imports `mpv-2.dll`; every DLL that
     `mpv-2.dll`, `ass-*.dll` and `libplacebo-*.dll` import is in the app's output folder; a libmpv
     API test (create, `wid`, `vo=gpu-next`, `loadfile`, like `tutils/mpvplayer.cpp`) plays a file.
6. **Upload** `mpv_dll` (only new package ids), then set the `mpv_dll` timestamp in the app
   lockfile to the remote's.

## Gotchas

- **`mpv.lib` points at `mpv.exe`.** libmpv and `mpv.exe` are both meson targets named `mpv`;
  `mpv.exe` exports the client API, so link.exe overwrites libmpv's `mpv.lib` with the exe's import
  library. Apps linking it crash in `mpv_create()`. The script regenerates `lib/mpv.lib` from
  `mpv-2.dll`'s exports and the validator checks it. Playing files with `mpv.exe` does not catch
  this; always test through the libmpv API.
- **libass and libplacebo default to static** in their own `default_options`; the script forces
  `-Dlibass:default_library=shared -Dlibplacebo:default_library=shared`.
- **`mpv.exe` started without a console** uses the pseudo-GUI profile (`idle=once`) and stays open
  after the file ends. Use `mpv.com` or `--idle=no` for scripted tests.
- **"Main binary package … missing"** during `conan install` refers to skipped binaries (static or
  tool deps nothing links); it is not an error.
- **GPU-decoded frames and `vf`**: with `hwdec=d3d11va`, CPU-only lavfi filters cannot run and mpv
  disables them; this is normal mpv behavior.

## Generated paths (Windows)

`conan/`, `build-win/` (ignored by git), `subprojects/*.wrap` and the cloned
`subprojects/libass`, `subprojects/libplacebo`.

---

# macOS

## Quick start

```bash
# arm64 (default)
conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ./build-scripts/profile_mac_armv8 -of ./conan
bash ./ci/build-macos-minimal.sh

# Intel (cross-compiled from Apple Silicon). Delete ./conan when switching architectures,
# otherwise arm64 and x86_64 libraries get mixed.
rm -rf ./conan
conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ./build-scripts/profile_mac14.0 -of ./conan
CONAN_OUTPUT_DIR=./conan MACOS_ARCH=x86_64 ./ci/build-macos-minimal.sh
```

`conan install` writes conan metadata, env scripts, copied headers/libs and generated `.pc` files
into `./conan`; the build script reads `CONAN_OUTPUT_DIR` (default `./conan`). The target is
macOS `11.0`. The Intel path uses `ci/x86_64-cross-file.txt` for meson and stages FFmpeg runtime
libraries under `builds-x86`.

The script ends by running `./build-minimal/mpv -v --no-config`.

## Main files

- `ci/build-macos-minimal.sh`: the build entry point.
- `build-scripts/conanfile.py`: conan consumer recipe (shared with Windows).
- `build-scripts/profile_mac_armv8`, `build-scripts/profile_mac14.0`: conan profiles.
- `ci/build-common.sh`: shared meson args, forces `-Dgpl=false`.
- `ci/package-macos.sh`: builds the full package and rewrites dylib references.
- `osdep/mac/meson.build`: Swift bridge wiring.

## Dependencies

- From conan (`build-scripts/conanfile.py`): topaz-ffmpeg, zlib, harfbuzz (static, no glib; brings
  freetype), fribidi (shared), libiconv (shared).
- `libass` is built as a meson subproject: the script writes `subprojects/libass.wrap` (libass
  `master`) and passes `--force-fallback-for=libass`.

## Outputs

| | arm64 | x86_64 |
| --- | --- | --- |
| build directory | `build-minimal/` | `build-minimal-x86_64/` |
| install prefix | `~/out/mpv-minimal` | `~/out/mpv-minimal-x86_64` |
| full package | `build-minimal/package-macos-armv8` | `build-minimal-x86_64/package-macos-x86_64` |
| slim package (publish) | `build-minimal/package-macos-armv8-slim` | `build-minimal-x86_64/package-macos-x86_64-slim` |

The slim package goes into `topaz-conan-dev/prebuilt/mpv_dll/<version>/macos_armv8` (or
`macos_x86_64`). It contains `bin/mpv`, `include/mpv/{client,stream_cb,render_gl,render}.h` and the
`libmpv`, `libfribidi`, `libiconv` and `libcharset` dylibs. FFmpeg comes from `topaz-ffmpeg` at
runtime.

## Verification

```bash
cd build-minimal
./mpv -v --no-config
```

Expect: mpv starts, FFmpeg library versions are printed, `Configuration:` shows `-Dgpl=false`, and
`gpl` is not in the enabled features list.

## What the minimal build disables / keeps

Disabled: `tests`, `cplugins`, `javascript`, `libcurl`, `jpeg`, `lcms2`, `libarchive`,
`libavdevice`, `libbluray`, `rubberband`, `uchardet`, `vapoursynth`, `zimg`, `lua`, `vulkan`,
`videotoolbox-gl`, `videotoolbox-pl`, `macos-media-player`, `macos-touchbar`, `drm`, `wayland`, `x11`.

Kept: `libass`, `libplacebo`, `cocoa`, `coreaudio`, `gl`, `gl-cocoa`, `plain-gl`, `iconv`, `zlib`,
`swift-build`, `macos-cocoa-cb`.

- `libass` and `libplacebo` are effectively core in this tree.
- `swift-build` is needed for the Cocoa CLI path; `macos-cocoa-cb` is kept because the target
  includes `libmpv`.

Going smaller than this is a refactor (making libplacebo/libass/Swift optional), not a flag change.

## Why the flow looks like this

- **FFmpeg `.pc` files are rewritten.** The copied FFmpeg `.pc` files in `./conan/lib/pkgconfig`
  had stale relative paths (`./builds-arm/include`, `./builds-arm/lib`); the script rewrites them to
  the real `./conan` folder before meson runs.
- **The conan include dir comes first.** The script adds `-I${CONAN_OUTPUT_DIR}/include` to `CFLAGS`
  and `meson.build` includes `./conan/include`, because `/opt/local/include` otherwise won the
  include order and caused FFmpeg header/API mismatches.
- **`builds-arm` runtime layout.** The FFmpeg dylibs expect a `builds-arm/lib/...` runtime path
  (`builds-x86` for Intel). The script creates a repo-root symlink `builds-arm -> conan` (for
  `./build-minimal/mpv` run from the repo root) and copies a real `build-minimal/builds-arm/` folder
  (for `cd build-minimal && ./mpv`).

## Tooling

- meson as a `meson` executable on `PATH` or the Python `mesonbuild` module.
- `ninja` on `PATH`.
- conan output already in `./conan` (run `conan install` first).

## Generated paths (macOS)

`conan/`, `subprojects/`, `build-minimal/` (`build-minimal-x86_64/`), `builds-arm` (`builds-x86`),
`build-minimal/builds-arm/`.

## Troubleshooting

- **Meson finds the wrong libraries/headers**: check `./conan/lib/pkgconfig/*.pc`, `PKG_CONFIG_PATH`,
  and that `./conan/include` comes before `/opt/local/include`.
- **`build-minimal/mpv` links but will not launch**: check that `build-minimal/builds-arm/` exists and
  that `builds-arm -> conan` exists at the repo root.
- **Swift or libplacebo failures**: the setup already includes fixes for Python 3.13 in libplacebo's
  generator, Swift include access to the generated `libplacebo/config.h`, and a Vulkan macro
  redefinition. If they regress, check `subprojects/libplacebo/src/vulkan/utils_gen.py`,
  `osdep/mac/meson.build` and `video/out/vulkan/common.h`.

## Previous macOS flow (reference)

The older flow used `MPV_INSTALL_PREFIX=$(pwd)/build/out/mpv`, `--cross-file ci/x86_64-cross-file.txt`
for Intel, `--wrap-mode=forcefallback`, `-Ddefault_library=shared`, `-Dtests=false`,
`-Dlua=disabled`, `-Djpeg=disabled`, explicit libass/libplacebo options in `meson setup`, and
rewrote dylibs after install with `python build-scripts/fixDeps.py $MPV_INSTALL_PREFIX/lib/*.dylib`.
