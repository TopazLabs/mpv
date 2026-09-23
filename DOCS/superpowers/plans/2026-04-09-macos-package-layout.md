# macOS Package Layout Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `ci/build-macos.sh` and `ci/build-macos-minimal.sh` emit a full in-repo macOS package directory with `bin`, `include`, `lib`, and related assets so the output is easy to consume as a package.

**Architecture:** Keep the current Meson build/install flow unchanged, then add a shared packaging helper that assembles a final package directory from the Meson install prefix plus Conan-provided runtime/development assets. Both macOS build scripts call the same helper with script-specific defaults so package assembly stays consistent between the standard and minimal builds.

**Tech Stack:** Bash, Python 3 stdlib (`pathlib`, `shutil`, `sys`), Meson install output, Conan output directories

---

### Task 1: Add Shared macOS Packaging Helper

**Files:**
- Create: `ci/package-macos.sh`
- Test: manual verification via `bash ci/package-macos.sh ...`

- [ ] **Step 1: Write the helper with deterministic package assembly**

```bash
#!/usr/bin/env bash

set -euo pipefail

INSTALL_PREFIX="${1:?install prefix required}"
CONAN_OUTPUT_DIR="${2:?conan output dir required}"
PACKAGE_OUTPUT_DIR="${3:?package output dir required}"

python3 - "${INSTALL_PREFIX}" "${CONAN_OUTPUT_DIR}" "${PACKAGE_OUTPUT_DIR}" <<'PY'
from pathlib import Path
import shutil
import sys

install_prefix = Path(sys.argv[1]).resolve()
conan_output_dir = Path(sys.argv[2]).resolve()
package_output_dir = Path(sys.argv[3]).resolve()

if not install_prefix.exists():
    raise SystemExit(f"Install prefix does not exist: {install_prefix}")

if package_output_dir.exists():
    shutil.rmtree(package_output_dir)

package_output_dir.mkdir(parents=True, exist_ok=True)

def copy_children(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    for child in src.iterdir():
        target = dst / child.name
        if child.is_dir() and not child.is_symlink():
            shutil.copytree(child, target, symlinks=True, dirs_exist_ok=True)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            if target.exists() or target.is_symlink():
                if target.is_dir() and not target.is_symlink():
                    shutil.rmtree(target)
                else:
                    target.unlink()
            shutil.copy2(child, target, follow_symlinks=False)

copy_children(install_prefix, package_output_dir)

for name in ("bin", "include", "lib", "etc", "share", "loader"):
    copy_children(conan_output_dir / name, package_output_dir / name)
PY
```

- [ ] **Step 2: Make the helper executable**

Run: `chmod +x ci/package-macos.sh`
Expected: command succeeds with no output

- [ ] **Step 3: Smoke-check argument validation**

Run: `bash ci/package-macos.sh`
Expected: non-zero exit with `install prefix required`

- [ ] **Step 4: Commit**

```bash
git add ci/package-macos.sh
git commit -m "build: add macOS package assembly helper"
```

### Task 2: Package the Standard macOS Build Output

**Files:**
- Modify: `ci/build-macos.sh`
- Test: `ci/build-macos.sh`

- [ ] **Step 1: Add package output variables near the existing prefix variables**

```bash
CONAN_OUTPUT_DIR="${CONAN_OUTPUT_DIR:-./conan}"
MPV_INSTALL_PREFIX="${HOME}/out/mpv"
PACKAGE_OUTPUT_DIR="${PACKAGE_OUTPUT_DIR:-./build/package-macos-armv8}"
SUBPROJECTS_DIR="subprojects"
```

- [ ] **Step 2: Call the shared packaging helper after install and optional Conan runtime activation**

```bash
"${MESON[@]}" compile -C build -j4
"${MESON[@]}" install -C build

if [[ -f "${CONAN_OUTPUT_DIR}/conanrun.sh" ]] ; then
    . "${CONAN_OUTPUT_DIR}/conanrun.sh"
fi

./ci/package-macos.sh "${MPV_INSTALL_PREFIX}" "${CONAN_OUTPUT_DIR}" "${PACKAGE_OUTPUT_DIR}"
```

- [ ] **Step 3: Keep the existing `build/builds-arm` runtime staging for local smoke tests**

```bash
# Keep a runnable copy of the Conan runtime inside build/ so mpv can
# be launched directly from that directory.
python3 - "${CONAN_OUTPUT_DIR}" "build/builds-arm" <<'PY'
# existing runtime-copy code stays here unchanged
PY
```

- [ ] **Step 4: Add a short success echo so callers know where the package landed**

```bash
echo "Packaged macOS build at ${PACKAGE_OUTPUT_DIR}"
```

- [ ] **Step 5: Run the build to verify packaging works**

Run: `./ci/build-macos.sh`
Expected: build succeeds, `build/package-macos-armv8` exists, and the script prints `Packaged macOS build at ./build/package-macos-armv8`

- [ ] **Step 6: Confirm package contents**

Run: `ls "./build/package-macos-armv8"`
Expected: includes `bin`, `include`, `lib`; may also include `etc`, `share`, and `loader`

- [ ] **Step 7: Commit**

```bash
git add ci/build-macos.sh
git commit -m "build: package standard macOS output"
```

### Task 3: Package the Minimal macOS Build Output

**Files:**
- Modify: `ci/build-macos-minimal.sh`
- Test: `ci/build-macos-minimal.sh`

- [ ] **Step 1: Add the package output variable using the minimal build directory**

```bash
CONAN_OUTPUT_DIR="${CONAN_OUTPUT_DIR:-./conan}"
BUILD_DIR="${BUILD_DIR:-build-minimal}"
MPV_INSTALL_PREFIX="${HOME}/out/mpv-minimal"
PACKAGE_OUTPUT_DIR="${PACKAGE_OUTPUT_DIR:-./build-minimal/package-macos-armv8}"
SUBPROJECTS_DIR="subprojects"
```

- [ ] **Step 2: Call the shared packaging helper after install and optional Conan runtime activation**

```bash
"${MESON[@]}" compile -C "${BUILD_DIR}" -j4
"${MESON[@]}" install -C "${BUILD_DIR}"

if [[ -f "${CONAN_OUTPUT_DIR}/conanrun.sh" ]] ; then
    . "${CONAN_OUTPUT_DIR}/conanrun.sh"
fi

./ci/package-macos.sh "${MPV_INSTALL_PREFIX}" "${CONAN_OUTPUT_DIR}" "${PACKAGE_OUTPUT_DIR}"
```

- [ ] **Step 3: Leave the existing minimal runtime staging logic in place**

```bash
python3 - "${CONAN_OUTPUT_DIR}" "${BUILD_DIR}/builds-arm" <<'PY'
# existing runtime-copy code stays here unchanged
PY
```

- [ ] **Step 4: Add a short success echo**

```bash
echo "Packaged minimal macOS build at ${PACKAGE_OUTPUT_DIR}"
```

- [ ] **Step 5: Run the minimal build to verify packaging works**

Run: `./ci/build-macos-minimal.sh`
Expected: build succeeds, `build-minimal/package-macos-armv8` exists, and the script prints `Packaged minimal macOS build at ./build-minimal/package-macos-armv8`

- [ ] **Step 6: Confirm package contents**

Run: `ls "./build-minimal/package-macos-armv8"`
Expected: includes `bin`, `include`, `lib`; optional `etc`, `share`, and `loader` appear when available

- [ ] **Step 7: Commit**

```bash
git add ci/build-macos-minimal.sh
git commit -m "build: package minimal macOS output"
```

### Task 4: Ignore Package Directories and Update Build Notes

**Files:**
- Modify: `.gitignore`
- Modify: `build-scripts/BUILDING.md`
- Modify: `build-scripts/CHANGELOG.md`

- [ ] **Step 1: Ignore the generated package directories**

```gitignore
/build/package-macos-armv8/
/build-minimal/package-macos-armv8/
```

- [ ] **Step 2: Add a short note to the macOS build documentation describing the new package outputs**

```md
The macOS build scripts now emit reusable in-repo package directories:

- `./build/package-macos-armv8`
- `./build-minimal/package-macos-armv8`

Each package merges the Meson install output with Conan-provided `bin`, `include`, `lib`, and related assets when available.
```

- [ ] **Step 3: Add a concise changelog entry for the build-script area**

```md
## 2026-04-09

- macOS standard and minimal build scripts now emit full in-repo package directories for reuse, alongside the existing runtime staging layout.
```

- [ ] **Step 4: Run a quick docs sanity check**

Run: `rg "package-macos-armv8|Packaged macOS build|Packaged minimal macOS build" .gitignore build-scripts/BUILDING.md ci/build-macos.sh ci/build-macos-minimal.sh`
Expected: matches in all intended files

- [ ] **Step 5: Commit**

```bash
git add .gitignore build-scripts/BUILDING.md build-scripts/CHANGELOG.md
git commit -m "docs: describe macOS package outputs"
```

### Task 5: Final Verification

**Files:**
- Test: `ci/build-macos.sh`
- Test: `ci/build-macos-minimal.sh`

- [ ] **Step 1: Run the standard build end-to-end**

Run: `./ci/build-macos.sh`
Expected: successful build, smoke test still runs, and `build/package-macos-armv8` is recreated from scratch

- [ ] **Step 2: Verify core package directories for the standard build**

Run: `ls "./build/package-macos-armv8"`
Expected: `bin`, `include`, `lib`; optional `etc`, `share`, `loader` if supplied by install or Conan

- [ ] **Step 3: Verify pkg-config files if Conan provides them**

Run: `ls "./build/package-macos-armv8/lib/pkgconfig"`
Expected: `.pc` files such as `mpv.pc`, `libass.pc`, or `libplacebo.pc`

- [ ] **Step 4: Run the minimal build end-to-end**

Run: `./ci/build-macos-minimal.sh`
Expected: successful build, smoke test still runs, and `build-minimal/package-macos-armv8` is recreated from scratch

- [ ] **Step 5: Verify core package directories for the minimal build**

Run: `ls "./build-minimal/package-macos-armv8"`
Expected: `bin`, `include`, `lib`; optional `etc`, `share`, `loader` if supplied by install or Conan

- [ ] **Step 6: Run lints only if shell or docs tooling is available locally**

Run: `bash -n ci/package-macos.sh ci/build-macos.sh ci/build-macos-minimal.sh`
Expected: no syntax errors

- [ ] **Step 7: Commit**

```bash
git add ci/package-macos.sh ci/build-macos.sh ci/build-macos-minimal.sh .gitignore build-scripts/BUILDING.md build-scripts/CHANGELOG.md
git commit -m "build: emit reusable macOS package directories"
```
