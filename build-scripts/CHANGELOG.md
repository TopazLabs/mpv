## 2026-04-09

- macOS local build now prefers Conan-generated env/pkg-config output instead of the old sysroot-only path.
- macOS build bootstraps `libass` as a Meson subproject; `freetype`/`harfbuzz` are no longer carried in the Conan recipe.
- Added local macOS build notes documenting the current Conan install command, build entrypoint, and smoke test.
