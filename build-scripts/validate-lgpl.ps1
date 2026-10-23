# Validates that a Windows mpv build is LGPL: -Dgpl=false, FFmpeg configured
# without GPL/nonfree parts, no GPL-only libraries shipped, and every LGPL
# dependency linked dynamically (as a DLL) rather than statically.
#
# Usage: build-scripts/validate-lgpl.ps1 [-BuildDir build-win] [-OutDir build-win/package-windows-x86_64]
# Needs dumpbin (run from a VS developer shell; build-win.ps1 does this).
# Exits non-zero if any check fails.

param(
    [string]$BuildDir = "build-win",
    [string]$OutDir = "build-win/package-windows-x86_64"
)

$ErrorActionPreference = "Stop"
$script:failures = 0

function Pass($msg) { Write-Host "  [PASS] $msg" -ForegroundColor Green }
function Fail($msg) { Write-Host "  [FAIL] $msg" -ForegroundColor Red; $script:failures++ }
function Check($cond, $msg) { if ($cond) { Pass $msg } else { Fail $msg } }

function Read-Ascii($path) {
    [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($path))
}

function Get-Dependents($dll) {
    (& dumpbin /nologo /dependents $dll) |
        Where-Object { $_ -match '^\s+\S+\.dll\s*$' } |
        ForEach-Object { $_.Trim().ToLower() }
}

$bin = Join-Path $OutDir "bin"
$mpvDll = Join-Path $bin "mpv-2.dll"
if (-not (Test-Path $mpvDll)) { throw "$mpvDll not found" }
if (-not (Get-Command dumpbin -ErrorAction SilentlyContinue)) {
    throw "dumpbin not on PATH; run from a VS developer shell"
}

Write-Host "== 1. meson configuration"
$opts = Get-Content (Join-Path $BuildDir "meson-info/intro-buildoptions.json") -Raw | ConvertFrom-Json
$gplOpt = $opts | Where-Object { $_.name -eq "gpl" -and -not $_.subproject }
Check ($gplOpt -and $gplOpt.value -eq $false) "meson option gpl=false"
$configH = Get-Content (Join-Path $BuildDir "config.h") -Raw
Check ($configH -match '#define HAVE_GPL 0') "config.h: HAVE_GPL 0"
$fullconfig = [regex]::Match($configH, '#define FULLCONFIG "([^"]*)"').Groups[1].Value
Check ($fullconfig -and ($fullconfig -split ' ') -notcontains 'gpl') "config.h: 'gpl' not in enabled features"

Write-Host "== 2. mpv-2.dll"
$mpvText = Read-Ascii $mpvDll
Check ($mpvText.Contains($fullconfig)) "mpv-2.dll embeds the validated feature list"
Check ($mpvText -match '-Dgpl=false') "mpv-2.dll configuration contains -Dgpl=false"
Check ($mpvText -notmatch '-Dgpl=true') "mpv-2.dll configuration does not contain -Dgpl=true"

Write-Host "== 3. FFmpeg license"
$ffDlls = Get-ChildItem $bin -Filter "*.dll" | Where-Object { $_.Name -match '^(avcodec|avdevice|avfilter|avformat|avutil|swresample|swscale)-\d+\.dll$' }
Check ($ffDlls.Count -ge 5) "found FFmpeg DLLs: $($ffDlls.Name -join ', ')"
foreach ($dll in $ffDlls) {
    $t = Read-Ascii $dll.FullName
    Check ($t.Contains("LGPL version 2.1 or later")) "$($dll.Name): license is LGPL version 2.1 or later"
    Check (-not ($t -match '(?<!L)GPL version [23] or later|nonfree and unredistributable|LGPL version 3')) "$($dll.Name): no GPL/LGPLv3/nonfree license string"
}
$avutil = $ffDlls | Where-Object Name -like 'avutil-*' | Select-Object -First 1
if ($avutil) {
    $cfg = [regex]::Match((Read-Ascii $avutil.FullName), '--toolchain[^\x00]*').Value
    foreach ($bad in '--enable-gpl', '--enable-nonfree', '--enable-version3', 'libx264', 'libx265', 'postproc', 'rubberband') {
        Check (-not $cfg.Contains($bad)) "FFmpeg configure line has no $bad"
    }
}

Write-Host "== 4. no GPL-only libraries shipped"
$gplFiles = Get-ChildItem $OutDir -Recurse -File | Where-Object { $_.Name -match '^(lib)?(postproc|x264|x265|rubberband|xvidcore|dvdnav|dvdread|cdio|jack|caca)' }
Check (-not $gplFiles) "no GPL-only libraries ($($gplFiles.Name -join ', '))"

Write-Host "== 5. dynamic linking"
$mpvDeps = Get-Dependents $mpvDll
foreach ($lib in 'avcodec', 'avformat', 'avutil', 'avfilter', 'swresample', 'swscale', 'libplacebo', 'ass') {
    Check ($mpvDeps | Where-Object { $_ -match "^$lib-\d+\.dll$" }) "mpv-2.dll imports $lib DLL"
}
$assDll = Get-ChildItem $bin -Filter "ass-*.dll" | Select-Object -First 1
if ($assDll) {
    $assDeps = Get-Dependents $assDll.FullName
    foreach ($lib in 'freetype', 'harfbuzz', 'fribidi', 'iconv') {
        Check ($assDeps | Where-Object { $_ -match "^(lib)?$lib" }) "$($assDll.Name) imports $lib DLL"
    }
} else { Fail "ass-*.dll not shipped" }
$plDll = Get-ChildItem $bin -Filter "libplacebo-*.dll" | Select-Object -First 1
if ($plDll) {
    $plDeps = Get-Dependents $plDll.FullName
    foreach ($lib in 'shaderc', 'spirv-cross') {
        Check ($plDeps | Where-Object { $_ -match "^$lib" }) "$($plDll.Name) imports $lib DLL"
    }
} else { Fail "libplacebo-*.dll not shipped" }
# LGPL code must not be compiled into mpv-2.dll: its symbols have to be imports.
$exports = (& dumpbin /nologo /exports $mpvDll) -join "`n"
foreach ($sym in 'avcodec_open2', 'av_frame_alloc', 'pl_renderer_create', 'ass_library_init') {
    Check ($exports -notmatch "\b$sym\b") "mpv-2.dll does not define/export $sym"
}
$imports = (& dumpbin /nologo /imports $mpvDll) -join "`n"
Check ($imports -match 'avcodec-\d+\.dll[\s\S]*?avcodec_send_packet') "avcodec_* resolved from avcodec DLL"
Check ($imports -match 'libplacebo-\d+\.dll[\s\S]*?pl_') "pl_* resolved from libplacebo DLL"

Write-Host "== 6. import library"
$implib = Join-Path $OutDir "lib/mpv.lib"
$members = (& lib /nologo /list $implib) | ForEach-Object { $_.Trim().ToLower() } | Select-Object -Unique
Check (($members -contains "mpv-2.dll") -and ($members -notcontains "mpv.exe")) "lib/mpv.lib links against mpv-2.dll (not mpv.exe)"

Write-Host "== 7. C runtime"
Check ($mpvDeps | Where-Object { $_ -match '^vcruntime\d+' }) "mpv-2.dll uses the dynamic CRT (/MD), same as FFmpeg"

Write-Host ""
if ($script:failures -gt 0) {
    Write-Host "LGPL VALIDATION FAILED ($script:failures check(s))" -ForegroundColor Red
    exit 1
}
Write-Host "LGPL VALIDATION PASSED" -ForegroundColor Green
exit 0
