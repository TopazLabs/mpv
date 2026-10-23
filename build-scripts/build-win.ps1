# Windows LGPL build of libmpv (mpv-2.dll) + mpv.exe with clang-cl.
#
#   powershell -ExecutionPolicy Bypass -File build-scripts/build-win.ps1 [-Clean]
#
# - All deps come from conan (profile_win2022, /MD), including the custom
#   topaz-ffmpeg and its runtime (videoai/tvai, ...), same as the macOS flow.
#   Versions are pinned by videoenhanceai's Windows lockfile (-AppLockfile), so
#   topaz-ffmpeg, zlib, lcms, ... are the exact revisions/binaries the app uses.
#   zlib and lcms are static (as in the app); freetype, harfbuzz, fribidi,
#   libiconv, shaderc and spirv-cross are shared.
# - libass and libplacebo are meson subprojects, built shared.
# - Packages (same idea as ci/build-macos-minimal.sh):
#     build-win/package-windows-x86_64       full, self-contained runtime
#     build-win/package-windows-x86_64-slim  only the files published to topaz-conan
# - The package is checked by validate-lgpl.ps1; the script fails if it is not LGPL.

param(
    [string]$BuildDir = "build-win",
    [string]$AppLockfile = "$PSScriptRoot\..\..\videoenhanceai\deploy\lockfiles\conanlock_win2022",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "..")
Set-Location $root
if (-not (Test-Path $AppLockfile)) { throw "videoenhanceai lockfile not found: $AppLockfile (pass -AppLockfile)" }

function Invoke-Native {
    & $args[0] @($args | Select-Object -Skip 1)
    if ($LASTEXITCODE -ne 0) { throw "$($args[0]) failed with exit code $LASTEXITCODE" }
}

# MSVC x64 environment; mpv only supports clang-cl on Windows (VS "C++ Clang" component)
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Llvm.Clang -property installationPath
if (-not $vs) { throw "Visual Studio with the C++ Clang component (Microsoft.VisualStudio.Component.VC.Llvm.Clang) is required" }
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Import-Module (Join-Path $vs "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
    Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -DevCmdArguments "-arch=x64 -host_arch=x64" | Out-Null
}
$env:PATH = "$vs\VC\Tools\Llvm\x64\bin;$env:PATH"
$env:CC = "clang-cl"
$env:CXX = "clang-cl"
$env:WINDRES = "llvm-rc"  # meson.build passes --codepage=65001, which rc.exe rejects

if ($Clean) {
    Remove-Item -Recurse -Force conan, $BuildDir -ErrorAction SilentlyContinue
}

# 1. conan deps. Never built here: every binary must come from topaz-conan, built and
#    published by topaz-conan-src (see BUILDING.md "Windows dependency policy").
conan install build-scripts/conanfile.py `
    "-pr:h" build-scripts/profile_win2022 "-pr:b" build-scripts/profile_win2022 `
    -c tools.env.virtualenv:powershell=powershell.exe `
    --lockfile $AppLockfile --lockfile-partial `
    --build=never -of conan
if ($LASTEXITCODE -ne 0) {
    throw "conan install failed. If a binary is missing, build and publish it from topaz-conan-src (BUILDING.md, Windows dependency policy); do not use --build=missing."
}

# The .pc files shipped in topaz-ffmpeg use a relative prefix (output-conan)
$ffDir = Join-Path $root "conan/lib3rdparty/topaz-ffmpeg"
$ffPrefix = $ffDir -replace '\\', '/'
Get-ChildItem (Join-Path $ffDir "lib/pkgconfig") -Filter *.pc | ForEach-Object {
    (Get-Content $_.FullName -Raw).Replace("output-conan", $ffPrefix) | Set-Content -NoNewline -Encoding ascii $_.FullName
}

# 2. build environment: meson, ninja, pkgconf, nasm from conan
. (Join-Path $root "conan/conanbuild.ps1")
$env:PKG_CONFIG_PATH = "$ffDir\lib\pkgconfig;$root\conan"
# meson's dependency('iconv') only looks at system paths, not pkg-config
$iconvDir = Join-Path $root "conan/lib3rdparty/libiconv"
$env:INCLUDE = "$iconvDir\include;$env:INCLUDE"
$env:LIB = "$iconvDir\lib;$env:LIB"

# 3. libass / libplacebo subprojects
New-Item -ItemType Directory -Force subprojects | Out-Null
@"
[wrap-git]
url = https://github.com/libass/libass.git
revision = 0.17.4
depth = 1
"@ | Set-Content -Encoding ascii subprojects/libass.wrap
@"
[wrap-git]
url = https://code.videolan.org/videolan/libplacebo.git
revision = v7.360.1
depth = 1
clone-recursive = true
"@ | Set-Content -Encoding ascii subprojects/libplacebo.wrap

# 4. configure + build
$mesonArgs = @(
    "--buildtype=release", "--wrap-mode=default", "-Ddefault_library=shared",
    "-Dlibmpv=true", "-Dcplayer=true", "-Dgpl=false", "-Dtests=false",
    "-Dd3d11=enabled", "-Dvulkan=disabled", "-Dlua=disabled", "-Djavascript=disabled",
    "-Dlibarchive=disabled", "-Drubberband=disabled", "-Dwin32-smtc=disabled", "-Djpeg=disabled",
    # both subprojects default to static in their own default_options
    "-Dlibass:default_library=shared", "-Dlibass:fontconfig=disabled",
    "-Dlibplacebo:default_library=shared",
    "-Dlibplacebo:demos=false", "-Dlibplacebo:d3d11=enabled", "-Dlibplacebo:shaderc=enabled",
    "-Dlibplacebo:lcms=enabled", "-Dlibplacebo:vulkan=disabled", "-Dlibplacebo:dovi=disabled",
    "-Dlibplacebo:libdovi=disabled", "-Dlibplacebo:xxhash=disabled"
)
if (Test-Path (Join-Path $BuildDir "build.ninja")) {
    Invoke-Native meson setup --reconfigure $BuildDir @mesonArgs
} else {
    Invoke-Native meson setup $BuildDir @mesonArgs
}
Invoke-Native meson compile -C $BuildDir

# 5. full package: mpv + every runtime DLL from conan
$pkg = Join-Path $BuildDir "package-windows-x86_64"
Remove-Item -Recurse -Force $pkg -ErrorAction SilentlyContinue
foreach ($d in "bin", "lib", "include/mpv", "include/ass", "licenses") { New-Item -ItemType Directory -Force (Join-Path $pkg $d) | Out-Null }
Copy-Item "$BuildDir/mpv-2.dll", "$BuildDir/mpv.exe", "$BuildDir/mpv.com" (Join-Path $pkg "bin")
# The libmpv and mpv.exe targets are both named "mpv", and mpv.exe exports the client
# API, so link.exe writes mpv.exe's import library over libmpv's mpv.lib. Regenerate
# mpv.lib from mpv-2.dll's exports so consumers link against the DLL.
$exports = (& dumpbin /nologo /exports "$BuildDir/mpv-2.dll") |
    Where-Object { $_ -match '^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]{8}\s+(mpv_\w+)' } | ForEach-Object { $Matches[1] }
if (-not $exports) { throw "no mpv_* exports found in mpv-2.dll" }
$def = Join-Path $BuildDir "mpv-2.def"
@("LIBRARY mpv-2.dll", "EXPORTS") + $exports | Set-Content -Encoding ascii $def
Invoke-Native lib /nologo /machine:x64 "/def:$def" ("/out:" + (Join-Path $pkg "lib/mpv.lib"))
Remove-Item (Join-Path $pkg "lib/mpv.exp")
Get-ChildItem "$BuildDir/subprojects" -Recurse -Include "ass-*.dll", "libplacebo-*.dll" | Copy-Item -Destination (Join-Path $pkg "bin")
Get-ChildItem "$BuildDir/subprojects" -Recurse -Include "ass.lib", "libplacebo.lib" | Copy-Item -Destination (Join-Path $pkg "lib")
Copy-Item "conan/bin/*" (Join-Path $pkg "bin")
Copy-Item include/mpv/*.h (Join-Path $pkg "include/mpv")
Copy-Item subprojects/libass/libass/ass.h, subprojects/libass/libass/ass_types.h (Join-Path $pkg "include/ass")
Copy-Item LICENSE.LGPL, Copyright $pkg
Copy-Item subprojects/libass/COPYING (Join-Path $pkg "licenses/libass.txt")
Copy-Item subprojects/libplacebo/LICENSE (Join-Path $pkg "licenses/libplacebo.txt")
Get-ChildItem conan/lib3rdparty -Directory | ForEach-Object {
    $lic = Join-Path $_.FullName "licenses"
    if (Test-Path $lic) { Copy-Item -Recurse $lic (Join-Path $pkg "licenses/$($_.Name)") }
}
Write-Host "Success: Windows package directory: $pkg"

# 6. LGPL validation (fails the build)
& (Join-Path $PSScriptRoot "validate-lgpl.ps1") -BuildDir $BuildDir -OutDir $pkg
if ($LASTEXITCODE -ne 0) { throw "LGPL validation failed" }

# 7. slim package: only the files published to topaz-conan (mpv_dll). It holds what
#    this repo builds; topaz-ffmpeg, freetype, harfbuzz, fribidi, libiconv, shaderc and
#    spirv-cross are requirements of the mpv_dll recipe and come from their own packages.
$slim = "$pkg-slim"
Remove-Item -Recurse -Force $slim -ErrorAction SilentlyContinue
foreach ($d in "bin", "lib", "include/mpv", "licenses") { New-Item -ItemType Directory -Force (Join-Path $slim $d) | Out-Null }
# apps only use libmpv; mpv.exe stays in the full package for local testing
foreach ($f in "mpv-2.dll", "ass-*.dll", "libplacebo-*.dll") { Copy-Item (Join-Path $pkg "bin/$f") (Join-Path $slim "bin") }
Copy-Item (Join-Path $pkg "lib/mpv.lib") (Join-Path $slim "lib")
Copy-Item include/mpv/client.h, include/mpv/stream_cb.h, include/mpv/render_gl.h, include/mpv/render.h (Join-Path $slim "include/mpv")
Copy-Item LICENSE.LGPL, Copyright $slim
# libass/libplacebo, plus zlib and lcms which are linked statically into mpv-2.dll/libplacebo
Copy-Item (Join-Path $pkg "licenses/libass.txt"), (Join-Path $pkg "licenses/libplacebo.txt") (Join-Path $slim "licenses")
foreach ($n in "lcms", "zlib") { Copy-Item -Recurse (Join-Path $pkg "licenses/$n") (Join-Path $slim "licenses/$n") }
Write-Host "Success: Windows slim package directory: $slim"

# 8. runtime test from the full package
Write-Host "--> Testing Windows build runtime"
Invoke-Native (Join-Path $pkg "bin/mpv.com") -v --no-config
