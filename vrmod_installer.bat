@echo off
SETLOCAL EnableDelayedExpansion
title VRMod Module Manager

:: Check for PowerShell 4+
FOR /F "tokens=*" %%a in ('powershell -command $PSVersionTable.PSVersion.Major 2^>nul') do set powershell_version=%%a
if defined powershell_version if !powershell_version! geq 4 GOTO update
echo This script requires Windows PowerShell 4.0+ (Windows 8.1 or later).
pause & exit

:update
:: Get latest installer batch from GitHub
powershell -command [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest https://github.com/catsethecat/vrmod-module/raw/master/vrmod_installer.bat -OutFile vrmod_installer.bat

:: Locate Steam and GMod install path
cls
set "steam_dir="
FOR /F "tokens=2* skip=2" %%a in ('reg query "HKLM\SOFTWARE\Valve\Steam" /v "InstallPath" 2^>nul') do set steam_dir=%%b
FOR /F "tokens=2* skip=2" %%a in ('reg query "HKLM\SOFTWARE\WOW6432Node\Valve\Steam" /v "InstallPath" 2^>nul') do set steam_dir=%%b

if not defined steam_dir (
    echo Steam installation path not found.
    pause & exit
)

set "gmod_dir="
if exist "%steam_dir%\steamapps\appmanifest_4000.acf" set "gmod_dir=%steam_dir%\steamapps\common\GarrysMod"

:: Check all library folders
for /f "usebackq tokens=2 skip=4" %%A in ("%steam_dir%\steamapps\libraryfolders.vdf") do (
  if exist "%%~A\steamapps\appmanifest_4000.acf" set "gmod_dir=%%~A\steamapps\common\GarrysMod"
)

if not defined gmod_dir (
    echo Garry's Mod installation path not found.
    pause & exit
)

echo.
echo Game folder detected:
echo %gmod_dir%
echo Make sure Garry's Mod is CLOSED before continuing.
echo.
echo Select an option:
echo 1) Install / Update
echo 2) Uninstall
set /p choice="> "

cls
if "%choice%"=="1" goto install
if "%choice%"=="2" goto uninstall

echo Invalid option. Please choose 1 or 2.
pause
goto :eof

:install
pushd %cd%
echo Downloading VRMod module from GitHub...
powershell -command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest 'https://github.com/catsethecat/vrmod-module/archive/master.zip' -OutFile 'vrmod.zip'"

if not exist vrmod.zip (
    echo Download failed.
    pause & exit
)

powershell -command "$hash = Get-FileHash -Algorithm SHA1 'vrmod.zip' | Select -ExpandProperty Hash; Write-Host 'SHA1:' $hash"

echo.
echo Continue with installation? (Y/N)
set /p choice="> "
if /I not "%choice%"=="Y" (
    del vrmod.zip
    exit
)

cls
echo Extracting archive...
powershell -command "Expand-Archive 'vrmod.zip' -DestinationPath 'vrmod' -Force"

echo Installing to %gmod_dir%...
xcopy /e /y /q "vrmod\vrmod-module-master\install\GarrysMod" "%gmod_dir%\"

echo Cleaning up...
rmdir /s /q vrmod
del vrmod.zip
echo.
echo ✅ Installation complete.
pause
exit

:uninstall
echo Uninstalling VRMod files from %gmod_dir%...
del "%gmod_dir%\garrysmod\lua\bin\gmcl_vrmod_win32.dll" 2>nul
del "%gmod_dir%\garrysmod\lua\bin\gmcl_vrmod_win64.dll" 2>nul
del "%gmod_dir%\garrysmod\lua\bin\gmcl_vrmod_linux.dll" 2>nul
del "%gmod_dir%\garrysmod\lua\bin\gmcl_vrmod_linux64.dll" 2>nul
del "%gmod_dir%\garrysmod\lua\bin\update_vrmod.bat" 2>nul

:: Bin cleanup
for %%F in (
    "openvr_api.dll"
    "openvr_license"
    "libopenvr_api.so"
) do del "%gmod_dir%\bin\%%F" 2>nul

:: Cleanup by arch
for %%F in (
    "linux32\libopenvr_api.so"
    "linux64\libopenvr_api.so"
    "win64\openvr_api.dll"
    "win64\HTC_License"
    "win64\libHTC_License.dll"
    "win64\nanomsg.dll"
    "win64\SRanipal.dll"
    "win64\SRWorks_Log.dll"
    "win64\ViveSR_Client.dll"
) do del "%gmod_dir%\bin\%%F" 2>nul

echo.
echo ✅ Uninstallation complete.
pause
exit
