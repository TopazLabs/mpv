# macOS Local Build Notes

## Conan Install

Use the current local macOS ARM64 dependency install command:

```bash
conan install ./build-scripts/conanfile.py -u -pr:b ./build-scripts/profile_mac_armv8 -pr:h ./build-scripts/profile_mac_armv8 -of ./conan
```

This writes Conan metadata, env scripts, and copied libraries into `./conan`.

## Build

Run the macOS build script from the repo root:

```bash
./ci/build-macos.sh
```

The script defaults to `CONAN_OUTPUT_DIR=./conan`. Override `CONAN_OUTPUT_DIR` only if the Conan output folder changes.

`libass` is bootstrapped as a Meson subproject during the build. `freetype`, `fribidi`, and `harfbuzz` are not part of the current Conan recipe.

## Quick Test

The build script finishes by running:

```bash
./build/mpv -v --no-config
```

That is the current smoke test for the local macOS build flow.
