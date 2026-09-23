#!/usr/bin/env bash

set -euo pipefail

MESON_PREFIX="${1:?meson install prefix required}"
CONAN_DIR="${2:?dependency output dir required}"
PACKAGE_DIR="${3:?package output dir required}"

if [[ ! -d "${MESON_PREFIX}" ]]; then
    echo "error: Meson install prefix does not exist: ${MESON_PREFIX}" >&2
    exit 1
fi

rm -rf "${PACKAGE_DIR}"
mkdir -p "${PACKAGE_DIR}"

cp -a "${MESON_PREFIX}/." "${PACKAGE_DIR}/"

if [[ -d "${CONAN_DIR}" ]]; then
    for name in bin include lib etc share loader; do
        src="${CONAN_DIR}/${name}"
        if [[ -d "${src}" ]]; then
            mkdir -p "${PACKAGE_DIR}/${name}"
            cp -a "${src}/." "${PACKAGE_DIR}/${name}/"
        fi
    done
fi

python3 - "${PACKAGE_DIR}" <<'PY'
from __future__ import annotations

import os
import shutil
import stat
import subprocess
import sys
from pathlib import Path

package_dir = Path(sys.argv[1]).resolve()
lib_dir = package_dir / "lib"
lib_dir.mkdir(parents=True, exist_ok=True)

SYSTEM_PREFIXES = ("/usr/lib/", "/System/Library/")


def run(*args: str) -> str:
    return subprocess.check_output(args, text=True)


def is_macho(path: Path) -> bool:
    if path.is_symlink() or not path.is_file():
        return False
    try:
        return "Mach-O" in run("file", "-b", str(path))
    except subprocess.CalledProcessError:
        return False


def make_writable(path: Path) -> None:
    mode = path.stat().st_mode
    if not mode & stat.S_IWUSR:
        path.chmod(mode | stat.S_IWUSR)


def is_within_package(path: Path) -> bool:
    try:
        path.resolve().relative_to(package_dir)
        return True
    except ValueError:
        return False


def parse_deps(path: Path) -> list[str]:
    lines = run("otool", "-L", str(path)).splitlines()[1:]
    deps = []
    for line in lines:
        line = line.strip()
        if not line:
            continue
        deps.append(line.split(" (compatibility version", 1)[0].strip())
    return deps


def resolve_dep(dep: str, owner: Path) -> Path | None:
    if dep.startswith(SYSTEM_PREFIXES):
        return None

    dep_name = Path(dep).name
    packaged_match = lib_dir / dep_name
    if packaged_match.exists():
        return packaged_match

    if dep.startswith("@loader_path/"):
        candidate = (owner.parent / dep.removeprefix("@loader_path/")).resolve()
        return candidate if candidate.exists() else (packaged_match if packaged_match.exists() else None)

    if dep.startswith("@executable_path/"):
        return packaged_match if packaged_match.exists() else None

    if dep.startswith("@rpath/"):
        return packaged_match if packaged_match.exists() else None

    if dep.startswith("."):
        candidate = (owner.parent / dep).resolve()
        return candidate if candidate.exists() else (packaged_match if packaged_match.exists() else None)

    if dep.startswith("/"):
        candidate = Path(dep)
        return candidate if candidate.exists() else None

    return packaged_match if packaged_match.exists() else None


def package_ref(owner: Path, dep_path: Path) -> str:
    rel = os.path.relpath(dep_path, owner.parent)
    return f"@loader_path/{rel}"


worklist = [path for path in package_dir.rglob("*") if is_macho(path)]
seen = set()
index = 0

while index < len(worklist):
    owner = worklist[index]
    index += 1
    owner_key = owner.resolve()
    if owner_key in seen:
        continue
    seen.add(owner_key)

    for dep in parse_deps(owner):
        resolved = resolve_dep(dep, owner)
        if resolved is None:
            if dep.startswith(SYSTEM_PREFIXES):
                continue
            raise SystemExit(f"error: unable to resolve dependency '{dep}' referenced by '{owner}'")

        if is_within_package(resolved):
            continue

        staged = lib_dir / Path(dep).name
        if not staged.exists():
            shutil.copy2(resolved.resolve(), staged, follow_symlinks=True)
            make_writable(staged)
            if is_macho(staged):
                worklist.append(staged)

all_macho_files = [path for path in package_dir.rglob("*") if is_macho(path)]

for path in all_macho_files:
    make_writable(path)

    if path.parent == lib_dir and path.suffix == ".dylib":
        new_id = f"@loader_path/{path.name}"
        subprocess.check_call(["install_name_tool", "-id", new_id, str(path)])

    for dep in parse_deps(path):
        if dep.startswith(SYSTEM_PREFIXES):
            continue

        dep_name = Path(dep).name
        if dep_name == path.name:
            continue

        resolved = resolve_dep(dep, path)
        if resolved is None:
            raise SystemExit(f"error: dependency '{dep}' for '{path}' was not staged into the package")

        target = resolved if is_within_package(resolved) else (lib_dir / dep_name)
        if not target.exists():
            raise SystemExit(f"error: packaged dependency '{target}' is missing for '{path}'")

        new_ref = package_ref(path, target)
        if dep != new_ref:
            subprocess.check_call(["install_name_tool", "-change", dep, new_ref, str(path)])
PY
