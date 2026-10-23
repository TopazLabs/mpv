#!/usr/bin/env python3
"""macOS packaging helpers for build-scripts/build-mac.sh.

  package-mac.py stage-ffmpeg <topaz-ffmpeg dir>
      topaz-ffmpeg ships dylibs and .pc files with a relative ./builds-<arch> prefix.
      Give its dylibs @rpath install names and point its .pc files at the real folder.

  package-mac.py package <build dir> <conan dir> <package dir>
      Full, self-contained runtime: bin/mpv, libmpv, libass, libplacebo and every
      non-system dylib they need, all relocated to @loader_path / @executable_path.

  package-mac.py slim <package dir> <slim dir>
      Only what mpv_dll publishes: libmpv, libass, libplacebo, headers, licenses.
      FFmpeg, freetype, harfbuzz and fribidi are requirements of the
      mpv_dll recipe and come from their own topaz-conan packages.
"""
from __future__ import annotations

import glob
import os
import re
import shutil
import stat
import subprocess
import sys
from pathlib import Path

SYSTEM_PREFIXES = ("/usr/lib/", "/System/Library/")
# a dependency resolved from one of these is host contamination, never acceptable
FORBIDDEN_PREFIXES = ("/opt/local/", "/usr/local/", "/opt/homebrew/")
REPO = Path(__file__).resolve().parent.parent


def run(*args: str) -> str:
    return subprocess.check_output(args, text=True)


def is_macho(path: Path) -> bool:
    if path.is_symlink() or not path.is_file():
        return False
    return "Mach-O" in run("file", "-b", str(path))


def make_writable(path: Path) -> None:
    mode = path.stat().st_mode
    if not mode & stat.S_IWUSR:
        path.chmod(mode | stat.S_IWUSR)


def deps_of(path: Path) -> list[str]:
    out = run("otool", "-L", str(path)).splitlines()[1:]
    return [line.strip().split(" (compatibility version", 1)[0] for line in out if line.strip()]


def rpaths_of(path: Path) -> list[str]:
    out = run("otool", "-l", str(path)).splitlines()
    res = []
    for i, line in enumerate(out):
        if line.strip() == "cmd LC_RPATH":
            m = re.match(r"\s*path (.*) \(offset", out[i + 2])
            if m:
                res.append(m.group(1))
    return res


def sign(path: Path) -> None:
    subprocess.check_call(["codesign", "--force", "-s", "-", str(path)], stderr=subprocess.DEVNULL)


# ---------------------------------------------------------------- stage-ffmpeg
def stage_ffmpeg(ff: Path) -> None:
    lib = ff / "lib"
    for a in lib.glob("*.a"):
        if re.match(r"lib(avcodec|avdevice|avfilter|avformat|avutil|swresample|swscale)\.a", a.name):
            a.unlink()  # the linker must pick the dylibs
    for f in sorted(lib.glob("*.dylib")):
        if f.is_symlink():
            continue
        make_writable(f)
        # keep the soname (libavcodec.62.dylib), not the fully versioned file name
        soname = Path(run("otool", "-D", str(f)).splitlines()[1].strip()).name
        subprocess.check_call(["install_name_tool", "-id", f"@rpath/{soname}", str(f)])
        for dep in deps_of(f):
            if dep.startswith("./builds-"):
                subprocess.check_call(["install_name_tool", "-change", dep, f"@rpath/{Path(dep).name}", str(f)])
        sign(f)
    for pc in (lib / "pkgconfig").glob("*.pc"):
        text = pc.read_text()
        new = re.sub(r"\./builds-(arm|x86)\b", str(ff.resolve()), text)
        if new != text:
            pc.write_text(new)
    print(f"staged {ff}")


# --------------------------------------------------------------------- package
def find_built(build: Path, pattern: str) -> list[Path]:
    """Real (non-symlink) dylibs of a build output, newest soversion only."""
    hits = [Path(p) for p in glob.glob(str(build / pattern), recursive=True)]
    return [p for p in hits if p.is_file() and not p.is_symlink()]


def package(build: Path, conan: Path, out: Path) -> None:
    build, conan, out = build.resolve(), conan.resolve(), out.resolve()
    shutil.rmtree(out, ignore_errors=True)
    for d in ("bin", "lib", "include/mpv", "include/ass", "licenses"):
        (out / d).mkdir(parents=True)

    search = [p for p in sorted((conan / "lib3rdparty").glob("*/lib")) if p.is_dir()]
    # FFmpeg's own runtime (videoai/tvai, aiengine, opencv, ...): conan's generated .pc
    # files name every package folder in the graph
    for pc in sorted(conan.glob("*.pc")):
        m = re.search(r"^prefix=(.*)$", pc.read_text(), re.M)
        if m and (Path(m.group(1)) / "lib").is_dir():
            search.append(Path(m.group(1)) / "lib")
    for d in os.environ.get("DYLD_LIBRARY_PATH", "").split(":"):
        if d and Path(d).is_dir() and Path(d).resolve() not in [s.resolve() for s in search]:
            search.append(Path(d))

    libs_src = (
        find_built(build, "libmpv.*.dylib")
        + find_built(build, "subprojects/libass/**/libass.*.dylib")
        + find_built(build, "subprojects/libplacebo/**/libplacebo.*.dylib")
    )
    libs_src = [p for p in libs_src if re.fullmatch(r"lib\w+\.\d+(\.\d+)*\.dylib", p.name)]
    if len(libs_src) != 3:
        sys.exit(f"error: expected libmpv, libass and libplacebo dylibs in {build}, found {libs_src}")
    mpv_bin = build / "mpv"
    if not mpv_bin.is_file():
        sys.exit(f"error: {mpv_bin} not found")

    shutil.copy2(mpv_bin, out / "bin" / "mpv")
    staged: dict[str, Path] = {}
    for src in libs_src:
        shutil.copy2(src, out / "lib" / src.name)
        staged[src.name] = out / "lib" / src.name

    def resolve(dep: str, owner: Path) -> Path | None:
        """Where a non-system dependency really lives; None for system libs."""
        if dep.startswith(SYSTEM_PREFIXES):
            return None
        if dep.startswith(FORBIDDEN_PREFIXES):
            sys.exit(f"error: {owner.name} links host library {dep} (MacPorts/Homebrew leak)")
        name = Path(dep).name
        if name in staged:
            return staged[name]
        for d in search:
            if (d / name).exists():
                return d / name
        sys.exit(f"error: cannot resolve {dep} (needed by {owner.name}); searched {[str(s) for s in search]}")

    work = [out / "bin" / "mpv"] + list(staged.values())
    seen: set[str] = set()
    while work:
        f = work.pop()
        if str(f) in seen:
            continue
        seen.add(str(f))
        for dep in deps_of(f):
            if Path(dep).name == f.name:
                continue
            found = resolve(dep, f)
            if found is None or Path(dep).name in staged:
                continue
            dst = out / "lib" / Path(dep).name
            shutil.copy2(found.resolve(), dst)
            make_writable(dst)
            if is_macho(dst):
                staged[dst.name] = dst
                work.append(dst)

    for f in [out / "bin" / "mpv"] + list(staged.values()):
        make_writable(f)
        for rp in rpaths_of(f):
            subprocess.check_call(["install_name_tool", "-delete_rpath", rp, str(f)])
        in_lib = f.parent.name == "lib"
        if in_lib:
            subprocess.check_call(["install_name_tool", "-id", f"@loader_path/{f.name}", str(f)])
        base = "@loader_path" if in_lib else "@executable_path/../lib"
        for dep in deps_of(f):
            if dep.startswith(SYSTEM_PREFIXES) or Path(dep).name == f.name:
                continue
            new = f"{base}/{Path(dep).name}"
            if dep != new:
                subprocess.check_call(["install_name_tool", "-change", dep, new, str(f)])
        sign(f)

    # unversioned symlinks next to the real files, e.g. libmpv.dylib -> libmpv.2.dylib
    for name in list(staged):
        m = re.match(r"(lib[\w-]+?)\.\d+(\.\d+)*\.dylib$", name)
        if m and m.group(0) == name:
            link = out / "lib" / f"{m.group(1)}.dylib"
            if not link.exists() and name.count(".") == 2:
                link.symlink_to(name)

    for h in ("client", "stream_cb", "render_gl", "render"):
        shutil.copy2(REPO / "include/mpv" / f"{h}.h", out / "include/mpv")
    for h in ("ass.h", "ass_types.h"):
        shutil.copy2(REPO / "subprojects/libass/libass" / h, out / "include/ass")
    for f in ("LICENSE.LGPL", "Copyright"):
        shutil.copy2(REPO / f, out)
    shutil.copy2(REPO / "subprojects/libass/COPYING", out / "licenses/libass.txt")
    shutil.copy2(REPO / "subprojects/libplacebo/LICENSE", out / "licenses/libplacebo.txt")
    for lic in (conan / "lib3rdparty").glob("*/licenses"):
        shutil.copytree(lic, out / "licenses" / lic.parent.name)


def slim(full: Path, out: Path) -> None:
    full, out = full.resolve(), out.resolve()
    shutil.rmtree(out, ignore_errors=True)
    for d in ("lib", "include/mpv", "licenses"):
        (out / d).mkdir(parents=True)
    for pattern in ("libmpv*.dylib", "libass*.dylib", "libplacebo*.dylib"):
        for f in (full / "lib").glob(pattern):
            shutil.copy2(f, out / "lib" / f.name, follow_symlinks=False)
    for h in (full / "include/mpv").glob("*.h"):
        shutil.copy2(h, out / "include/mpv")
    for f in ("LICENSE.LGPL", "Copyright"):
        shutil.copy2(full / f, out)
    for f in ("libass.txt", "libplacebo.txt"):
        shutil.copy2(full / "licenses" / f, out / "licenses")
    # statically linked: zlib into libmpv, lcms into libplacebo
    for n in ("lcms", "zlib"):
        shutil.copytree(full / "licenses" / n, out / "licenses" / n)


if __name__ == "__main__":
    cmd, *args = sys.argv[1:]
    if cmd == "stage-ffmpeg":
        stage_ffmpeg(Path(args[0]))
    elif cmd == "package":
        package(*map(Path, args))
    elif cmd == "slim":
        slim(*map(Path, args))
    else:
        sys.exit(__doc__)
