## 2026-04-09

- macOS local build now prefers Conan-generated env/pkg-config output instead of the old sysroot-only path.
- macOS build bootstraps `libass` as a Meson subproject; `freetype`/`harfbuzz` are no longer carried in the Conan recipe.
- Added local macOS build notes documenting the current Conan install command, build entrypoint, and smoke test.
- Defaulted the shared build flow to `-Dgpl=false` and disabled explicit GPL enables in Windows/Mingw scripts.
- macOS build now accepts either a `meson` binary on `PATH` or the Python `mesonbuild` module.
- macOS local build now normalizes copied pkg-config paths and creates a `builds-arm -> conan` symlink so the smoke test can load current FFmpeg dylibs.
- Added a reference note for the older macOS Meson flow, including the `fixDeps.py` post-install step.
- macOS build now stages Conan runtime libs under `build/builds-arm/` so `build/mpv` can be run directly from the build directory.
- Expanded the macOS build note with rationale, source-of-truth files, generated paths, and troubleshooting guidance.
- Clarified in the macOS build note that repo-root `builds-arm` is a symlink, while `build/builds-arm/` is a copied runtime folder.
