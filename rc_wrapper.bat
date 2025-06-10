@echo off
REM Try to auto-detect paths if not already set
if not defined WindowsSdkDir (
    for /f "tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0" /v "InstallationFolder" 2^>nul') do set "WindowsSdkDir=%%b"
)
if not defined VCToolsInstallDir (
    for /f "tokens=2*" %%a in ('reg query "HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VC7" /v "16.0" 2^>nul') do set "VCToolsInstallDir=%%b"
)

REM Fallback to hardcoded paths if auto-detection fails
if not defined WindowsSdkDir set "WindowsSdkDir=C:\Program Files (x86)\Windows Kits\10\"
if not defined WindowsSdkVersion set "WindowsSdkVersion=10.0.26100.0"
if not defined VCToolsInstallDir set "VCToolsInstallDir=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133\"

set "INCLUDE=%WindowsSdkDir%include\%WindowsSdkVersion%\um;%WindowsSdkDir%include\%WindowsSdkVersion%\shared;%WindowsSdkDir%include\%WindowsSdkVersion%\winrt;%WindowsSdkDir%include\%WindowsSdkVersion%\ucrt;%VCToolsInstallDir%include;%INCLUDE%"
"%WindowsSdkDir%bin\%WindowsSdkVersion%\x64\rc.exe" %* 