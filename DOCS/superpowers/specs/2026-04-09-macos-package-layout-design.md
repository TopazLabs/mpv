## Goal

Make both `ci/build-macos.sh` and `ci/build-macos-minimal.sh` produce a reusable in-repo macOS package directory with a layout similar to:

- `bin/`
- `include/`
- `lib/`
- `etc/`
- `share/`
- `loader/` when present

The package should be easy to consume from Conan or other tooling without depending on the current ad-hoc runtime copy under `build*/builds-arm`.

## Current State

`meson install` already installs mpv into a prefix, but the scripts only stage Conan runtime content into `build/builds-arm` or `build-minimal/builds-arm`, and only copy `bin/` and `lib/`.

That means the current output is sufficient for local smoke-testing, but it does not create a full package root containing headers, pkg-config files, and additional shared assets.

## Recommended Approach

Keep the existing build flow intact and add a packaging step after install.

The packaging step should:

1. Create a package root inside the repo.
2. Copy the Meson install output into that package root.
3. Overlay Conan-provided development/runtime directories into the same package root.
4. Preserve symlinks, especially under `lib/`.

This keeps build behavior stable while producing a second, packaging-friendly artifact.

## Package Output

Default output locations:

- `build/package-macos-armv8` for `ci/build-macos.sh`
- `build-minimal/package-macos-armv8` for `ci/build-macos-minimal.sh`

These defaults should be overridable with a shared environment variable such as `PACKAGE_OUTPUT_DIR`, with each script keeping its own default when the variable is unset.

## Packaging Inputs

The final package directory is assembled from two sources.

Meson install output contributes:

- `bin/`
- `include/mpv/`
- `lib/libmpv*`
- any `share/` or `etc/` content installed by mpv

Conan output contributes, when present:

- `bin/`
- `include/`
- `lib/`
- `etc/`
- `share/`
- `loader/`

The package step should copy only directories that exist, so both scripts remain resilient to optional/minimal dependency sets.

## Script Structure

Add a shared helper in `ci/`, for example `ci/package-macos.sh`, and call it from both build scripts after:

1. `meson install`
2. optional `conanrun.sh`

Suggested helper inputs:

- Meson install prefix path
- Conan output directory
- Final package output directory

The helper should remove and recreate the package output directory each run so the result is deterministic.

## Compatibility

Keep the existing `build*/builds-arm` staging behavior for now.

That path still serves the current smoke-test use case of running the locally built `mpv` binary directly from the build directory. The new package directory is a separate artifact for reuse and packaging.

## Error Handling

The helper should:

- fail if the Meson install prefix does not exist
- tolerate missing optional Conan subdirectories
- preserve symlinks while copying
- avoid mixing stale package contents from prior runs

## Testing

Verification should stay lightweight:

1. Run each build script.
2. Confirm the expected package directory exists.
3. Confirm it contains `bin/`, `include/`, and `lib/`.
4. Confirm `lib/pkgconfig/` is present when Conan provides pkg-config files.
5. Confirm `build/mpv` or `build-minimal/mpv` smoke test behavior still works as before.

## Non-Goals

- Replacing Meson install rules with manual file selection
- Removing the existing `build*/builds-arm` runtime layout in this change
- Reworking the Conan recipe itself unless packaging exposes a blocking issue
