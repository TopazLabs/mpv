# Building mpv (Topaz)

How to build this mpv fork as `libmpv` for Topaz apps (videoenhanceai), on **Windows** and
**macOS**. The two platforms use different scripts and package layouts; each has its own section
below. Read the section for your platform top to bottom before building.

| | Windows | macOS |
| --- | --- | --- |
| Entry point | `build-scripts/build-win.ps1` (PowerShell) | `build-scripts/build-mac.sh` (bash) |
| Compiler | clang-cl + MSVC link, `/MD` | Apple clang |
| Conan profile | `build-scripts/profile_win2022` | `build-scripts/profile_mac_armv8` (arm64), `profile_mac14.0` (Intel) |
| Conan install | done by the script, `--build=never`, pinned by videoenhanceai's lockfile | done by the script, same |
| Build dir | `build-win/` | `build-mac/` (`build-mac-x86_64/` for Intel) |
| Package to publish | `build-win/package-windows-x86_64-slim` | `build-mac/package-macos-armv8-slim` (`build-mac-x86_64/package-macos-x86_64-slim`) |
| `mpv_dll` version | `0.41.0-20260923-1` | `0.41.0-20260923-1` |
| LGPL check | `build-scripts/validate-lgpl.ps1`, runs automatically | `build-scripts/validate-lgpl-mac.sh`, runs automatically |

Shared by both platforms: `build-scripts/conanfile.py` (platform-only parts are inside
`if self.settings.os == ...`), `meson.build` / `meson.options`, and the topaz-ffmpeg
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
    -n harfbuzz -v 8.3.0-topaz -s --skip-default-config --normalize-conandata --skip-existing
```

- `--skip-existing`: before each `topaz_config.yml` entry's `conan create`, the script computes that
  entry's package id and skips it when the latest recipe revision on the remote already has it, or
  has the same configuration (settings, options, requires) built with another `compiler.cppstd`,
  which consumers get through Conan's cppstd compatibility. It then uploads only the package ids it
  built in that run, never other local binaries. Without it, every entry is rebuilt and the upload
  sends every matching local binary, so an existing package id gets a new package revision.

- `-s` builds without uploading. Before publishing, check that the exported recipe revision is the
  one already on topaz-conan and that the package id matches the table (or is the intended new one).
- harfbuzz needs `--normalize-conandata`: its published revision was exported with a CRLF
  `conandata.yml`; without the flag conan-src exports a different recipe revision.
- harfbuzz's `topaz_config.yml` also builds its static variant, which needs freetype's default
  (static) binary: build freetype first without `--skip-default-config`.
- Publishing: **never pass `--allow-new-recipe`**, and never re-upload an existing package id. Use
  `--skip-existing`. Without it the script uploads with `--package-query <profile settings>`, i.e.
  every local binary of that recipe that matches the profile, so you would have to publish from a
  clean conan cache (or CI agent) that only holds the intended binary.

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

Same layout as Windows: conan supplies every binary dependency, libass and libplacebo are meson
subprojects built as shared libraries, and the build ends with an automatic LGPL validation.

## Quick start

```bash
# arm64 (default)
build-scripts/build-mac.sh --clean

# Intel (cross-compiled from Apple Silicon, own conan-x86_64/ and build-mac-x86_64/)
build-scripts/build-mac.sh --arch x86_64 --clean
```

The script must end with `LGPL VALIDATION PASSED` and (arm64) a working `mpv -v --no-config` run.
Without `--clean` it reuses `conan*/` and `build-mac*/`. `ci/build-macos-minimal.sh` is a thin
wrapper (`MACOS_ARCH=x86_64` maps to `--arch`).

Options:

- `--arch arm64|x86_64`: default `arm64`. Profiles: `profile_mac_armv8` (build and arm64 host),
  `profile_mac14.0` (Intel host; the name is misleading).
- `--clean`: delete `conan*/` and `build-mac*/` first.
- `--app-lockfile <path>`: videoenhanceai's lockfile. Default:
  `../videoenhanceai/deploy/lockfiles/conanlock_mac_<armv8|x86_64>`.

## Prerequisites

- Xcode (Swift, Apple clang), macOS deployment target `11.0`.
- conan 2, logged in to the `topaz-conan` remote.
- `meson` and `ninja` on `PATH`, Python 3, git and network access (libass and libplacebo are
  cloned as meson subprojects). `nasm` is optional (libass x86 asm on Intel).

## What the script does

1. `conan install build-scripts/conanfile.py` with `--lockfile <AppLockfile> --lockfile-partial
   --build=never`, output in `conan/` (`conan-x86_64/`). Only the packages mpv builds against are
   copied into `conan/lib3rdparty/<name>` (topaz-ffmpeg, libiconv, fribidi, harfbuzz, freetype, zlib,
   lcms);
   topaz-ffmpeg's own runtime (videoai, aiengine, opencv, ...) is not copied or linked.
2. `package-mac.py stage-ffmpeg`: topaz-ffmpeg's dylibs and `.pc` files use a relative
   `./builds-<arch>` prefix. The staged copy gets `@rpath/<soname>` install names and absolute
   `.pc` paths. No `builds-arm` symlinks are needed any more.
3. `PKG_CONFIG_LIBDIR` is limited to conan's `.pc` files, so MacPorts/Homebrew libraries are never
   found; nothing from `/opt/local`, `/usr/local` or `/opt/homebrew` is added to include or link
   paths. Compiler/linker flags (deployment target, conan's libiconv, build rpaths) are written to
   `conan*/meson-flags.ini` and passed as a meson machine file: in the Intel cross build meson
   ignores `CFLAGS`/`LDFLAGS` for the host.
4. Writes `subprojects/libass.wrap` (libass `0.17.4`) and `subprojects/libplacebo.wrap`
   (libplacebo `v7.360.1`, with submodules), the same versions as Windows. No local edits of the
   subprojects are needed or supported.
5. `meson setup` with `-Ddefault_library=shared`, `-Dgpl=false`, `-Dlibmpv=true`, the forced
   fallbacks for libass/libplacebo, and `-Dlibass:default_library=shared
   -Dlibplacebo:default_library=shared`; then `meson compile`.
6. `package-mac.py package`: full package, every non-system dylib relocated to `@loader_path`
   (`@executable_path/../lib` for `bin/mpv`), build rpaths removed, ad-hoc re-signed. Fails on
   any dependency that resolves to a host library.
7. `validate-lgpl-mac.sh`, then `package-mac.py slim`, then `mpv -v --no-config` (arm64).

How dependencies are found:

| Used by | Dependency | Source |
| --- | --- | --- |
| mpv | FFmpeg (`libavcodec.62`, ...) | topaz-ffmpeg, shared |
| mpv | zlib | conan `zlib/1.2.13`, static (same binary as videoenhanceai) |
| mpv, libass | libiconv | conan `libiconv/1.17`, shared (same binary as videoenhanceai on macOS) |
| libplacebo | lcms2 | conan `lcms/2.17`, static (same binary as videoenhanceai), OpenGL backend only |
| libass | freetype, harfbuzz, fribidi | conan, shared (CoreText is the font provider) |
| mpv | libass, libplacebo | meson subprojects, forced shared |

Disabled in mpv: `tests`, `cplugins`, `javascript`, `libcurl`, `jpeg`, `lcms2`, `libarchive`,
`libavdevice`, `libbluray`, `rubberband`, `uchardet`, `vapoursynth`, `zimg`, `lua`, `vulkan`,
`videotoolbox-gl`, `videotoolbox-pl`, `macos-media-player`, `macos-touchbar`, `drm`, `wayland`, `x11`.
Kept: `libass`, `libplacebo`, `cocoa`, `coreaudio`, `gl`, `gl-cocoa`, `plain-gl`, `iconv`, `zlib`,
`swift-build`, `macos-cocoa-cb`. In libplacebo every optional dependency is disabled except OpenGL
and lcms (vulkan, shaderc, glslang, d3d11, dovi, libdovi, xxhash, unwind), so meson cannot pick
anything up from the host.

## Outputs

| | arm64 | x86_64 |
| --- | --- | --- |
| build directory | `build-mac/` | `build-mac-x86_64/` |
| full package | `build-mac/package-macos-armv8` | `build-mac-x86_64/package-macos-x86_64` |
| slim package (publish) | `build-mac/package-macos-armv8-slim` | `build-mac-x86_64/package-macos-x86_64-slim` |

| Package | Contents | Use |
| --- | --- | --- |
| full | `bin/mpv`, `lib/` with libmpv, libass, libplacebo and every runtime dylib (FFmpeg, videoai/tvai, opencv, freetype, harfbuzz, fribidi, libiconv, ...), headers, licenses | local testing; self-contained |
| slim | `lib/libmpv.2.dylib`, `lib/libass.9.dylib`, `lib/libplacebo.360.dylib` (+ unversioned symlinks), `include/mpv/{client,stream_cb,render_gl,render}.h`, `LICENSE.LGPL`, `Copyright`, `licenses/` (libass, libplacebo, zlib, lcms) | goes into `mpv_dll` |

The slim package has no `bin/mpv` (apps only use libmpv) and no dependency dylibs: those are
requirements of the `mpv_dll` recipe and come from their own topaz-conan packages. Every
cross-reference is `@loader_path/<soname>`, so the app must put all of these dylibs in one folder,
as it already does for FFmpeg.

## LGPL validation

`build-scripts/validate-lgpl-mac.sh` runs on the full package after every build and fails the
build unless:

- meson `gpl=false`, `config.h` has `HAVE_GPL 0`, and `libmpv` embeds `-Dgpl=false` and a feature
  list without `gpl`;
- every FFmpeg dylib embeds "LGPL version 2.1 or later", and FFmpeg's configure line has no
  `--enable-gpl`, `--enable-nonfree`, `--enable-version3`, x264, x265 or postproc;
- no GPL-only libraries (postproc, x264, x265, rubberband, dvdnav, cdio, jack, caca, ...) are shipped;
- `libmpv` imports FFmpeg, libplacebo and libass as dylibs and defines none of their symbols;
  `libass` imports freetype, harfbuzz and fribidi as dylibs and defines none of their symbols;
  iconv is conan's libiconv dylib, not Apple's `/usr/lib/libiconv.2.dylib`;
- every dependency of every shipped binary is a system library or package-relative, with no
  rpaths, and no compile/link command of the build uses a MacPorts/Homebrew path;
- `libmpv` targets macOS `11.0`.

Run it on its own: `build-scripts/validate-lgpl-mac.sh build-mac build-mac/package-macos-armv8`.

## Dependency policy

Same as Windows: every dependency binary is built and published by topaz-conan-src; mpv only
consumes binaries from topaz-conan. `build-mac.sh` uses `--build=never`; never fix a missing binary
with `--build=missing`, `conan create` in this repo, or a hand-copied dylib.

- Versions and revisions come from videoenhanceai's lockfile, so mpv and the app resolve the same
  topaz-ffmpeg, zlib and lcms binaries.
- mpv-only shared builds in topaz-conan-src (`conan_create_args_mac` in each recipe's
  `topaz_config.yml`; package ids as published 2026-10-23):

| Package | Options | armv8 | x86_64 |
| --- | --- | --- | --- |
| `freetype/2.13.2-topaz` | `shared=True`, `with_png/with_bzip2/with_brotli/with_zlib=False` | `c2d19731…` | `a611eab4…` |
| `harfbuzz/8.3.0-topaz` | `shared=True`, `with_glib=False` (+ the freetype options) | `97d256a2…` | `1665434c…` |
| `fribidi/1.0.13-topaz` | `shared=True` (already published) | `40a20ef7…` | `986e14b3…` |
| `libiconv/1.17` | `shared=True` (already published; same binary as videoenhanceai) | `da36e180…` | `d1a40c97…` |

Building and publishing them in topaz-conan-src (from its repo root, on an Apple Silicon Mac; the
Intel binaries are cross-built with `-ph _util/conan/profile_mac14.0`):

```bash
bash run_build_and_publish.sh -u "" -c "" -r topaz-conan \
    -pb _util/conan/profile_mac_armv8 -ph _util/conan/profile_mac_armv8 \
    -n freetype -v 2.13.2-topaz --skip-existing
bash run_build_and_publish.sh -u "" -c "" -r topaz-conan \
    -pb _util/conan/profile_mac_armv8 -ph _util/conan/profile_mac_armv8 \
    -n harfbuzz -v 8.3.0-topaz --skip-existing --normalize-conandata
```

- `--skip-existing` (see Windows "Dependency policy") builds and uploads only the
  `topaz_config.yml` entries whose binary is not on topaz-conan yet. It is what makes the script
  safe here: without it the script also rebuilds every published variant (e.g. harfbuzz's static
  default) and uploads them as new package revisions. Add `-s` for a dry run.
- Never pass `--allow-new-recipe`. The script refuses an upload whose recipe revision is not the
  latest on topaz-conan.
- harfbuzz needs `--normalize-conandata` (CRLF `conandata.yml` in the published revision).
- **`sincosf`**: with newer Xcode SDKs (seen with 26.5 and the 14.4 Command Line Tools SDK), meson
  finds `sincosf` in libm but `math.h` does not declare it for the 11.0 deployment target, and
  harfbuzz fails with `use of undeclared identifier 'sincosf'`. harfbuzz's `conan_create_args_mac`
  entry therefore passes `-c 'tools.build:cxxflags=["-Dsincosf=__sincosf"]'`: `__sincosf` is the
  SDK's inline wrapper (`__sincosf_stret`, macOS 10.9+), so the math is the same. It is a build
  flag only: recipe revision and package id are unchanged. The published shared binaries were
  built this way (on SDK 14.4, `SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX14.4.sdk`);
  the older static ones came from the Azure agents (Xcode 15.4, `compiler.cppstd=17`).

## Release procedure (mpv_dll, macOS)

1. **Deps**: if a dependency changed, build and publish it from topaz-conan-src as above, then check
   that every mac binary is on the latest recipe revision
   (`remote_matrix.py freetype/2.13.2-topaz harfbuzz/8.3.0-topaz`).
2. **Build**: `build-scripts/build-mac.sh --clean` and `--arch x86_64 --clean`, both ending with
   `LGPL VALIDATION PASSED`.
3. **Package**: copy each slim package into
   `topaz-conan-dev/prebuilt/mpv_dll/<version>/macos_armv8` / `macos_x86_64`. Use a **new** `mpv_dll`
   version whenever its `conanfile.py` changes.
4. **Recipe**: keep the `mpv_dll` recipe in sync with `build-scripts/conanfile.py` (same refs, same
   shared options). Requirements are runtime-only (`headers=False, libs=False, run=True`).
5. **Test in videoenhanceai before uploading**: `conan export-pkg` from the version folder with
   `-pr:h <app>/deploy/mac/profile_mac_<arch> -pr:b <app>/deploy/mac/profile_mac_armv8 --lockfile
   <app>/deploy/lockfiles/conanlock_mac_<arch> --lockfile-partial`, bump `mpv_dll` in
   `deploy/conanfile.py`, update the lockfile, and check that every dylib that `libmpv`, `libass` and
   `libplacebo` import is in the app bundle's frameworks folder.
6. **Upload** only new package ids, then set the `mpv_dll` timestamp in the app lockfile.

## Troubleshooting

- **conan install: "Missing prebuilt package" for freetype/harfbuzz**: a shared mac binary is not
  on topaz-conan; publish it with `run_build_and_publish.sh ... --skip-existing` (see Dependency
  policy).
- **`cannot resolve @rpath/... (needed by ...)` while packaging**: a runtime dependency of
  topaz-ffmpeg is not in any conan package folder of the graph; check `conan/*.pc` and
  `conan/conanrunenv-*.sh`.
- **`links host library /opt/local/...`**: something in the build found a MacPorts/Homebrew library;
  check `PKG_CONFIG_LIBDIR` and the meson options for that dependency (disable it explicitly).
- **`--vo=gpu-next` prints "Failed initializing any suitable GPU context" in a non-GUI shell**: a
  background session has no window server. Run the player from Terminal.app; the libmpv API
  (`vo=null`) test works anywhere.
- **Stale `subprojects/libplacebo`/`libass`**: delete them; the wraps re-clone the pinned tags.
