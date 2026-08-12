@echo off
REM ===========================================================================
REM Tremulous 1.3 - Universal Build Script for Windows
REM
REM Supports: Visual Studio 2019, 2022, 2026+ and cmake-gui workflow.
REM All dependencies (SDL3, OpenAL Soft, curl, etc.) are vendored in external/
REM and built from source. No downloads required.
REM
REM Usage:
REM   build.bat                 - Interactive menu (pick VS version or cmake-gui)
REM   build.bat vs2022          - Generate + build with VS2022 x64
REM   build.bat vs2019          - Generate + build with VS2019 x64
REM   build.bat gui             - Open cmake-gui for manual configuration
REM   build.bat clean           - Remove all build directories
REM   build.bat deploy [path]   - Deploy binaries to game install
REM ===========================================================================

setlocal enabledelayedexpansion

REM --- Configuration ---
set BUILD_ROOT=build
set BUILD_CONFIG=Release
set CMAKE_BIN=cmake

REM --- Check CMake ---
echo Looking for CMake in PATH...
where %CMAKE_BIN% >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo [NOT FOUND] CMake is not in PATH.
    echo Install CMake from https://cmake.org/download/ and add to PATH,
    echo or set CMAKE_BIN to a full path, e.g.:  set CMAKE_BIN="C:\Program Files\CMake\bin\cmake.exe"
    exit /b 1
)
echo [OK] Found CMake.
echo.

REM --- Parse action ---
set ACTION=%1
if "%ACTION%"=="" goto :menu

if /i "%ACTION%"=="vs2022" (
    set VS_GEN="Visual Studio 17 2022"
    set BUILD_DIR=%BUILD_ROOT%\build-vs2022-x64
    goto :all
)
if /i "%ACTION%"=="vs2019" (
    set VS_GEN="Visual Studio 16 2019"
    set BUILD_DIR=%BUILD_ROOT%\build-vs2019-x64
    goto :all
)
if /i "%ACTION%"=="gui" goto :gui
if /i "%ACTION%"=="clean" goto :clean
if /i "%ACTION%"=="deploy" (
    set BUILD_DIR=%BUILD_ROOT%\build-vs2022-x64
    goto :deploy
)

echo [ERROR] Unknown action: %ACTION%
echo Usage: build.bat [vs2022^|vs2019^|gui^|clean^|deploy]
exit /b 1

:menu
echo.
echo  ================================================
echo   Tremulous 1.3 - Build Configuration
echo  ================================================
echo.
echo   1. Visual Studio 2022 (x64)
echo   2. Visual Studio 2019 (x64)
echo   3. Open cmake-gui (manual setup)
echo   4. Clean all build directories
echo   5. Deploy binaries to game install
echo   Q. Quit
echo.
set /p CHOICE="Select option: "

if /i "%CHOICE%"=="1" (
    set VS_GEN="Visual Studio 17 2022"
    set BUILD_DIR=%BUILD_ROOT%\build-vs2022-x64
    goto :all
)
if /i "%CHOICE%"=="2" (
    set VS_GEN="Visual Studio 16 2019"
    set BUILD_DIR=%BUILD_ROOT%\build-vs2019-x64
    goto :all
)
if /i "%CHOICE%"=="3" goto :gui
if /i "%CHOICE%"=="4" goto :clean
if /i "%CHOICE%"=="5" (
    set BUILD_DIR=%BUILD_ROOT%\build-vs2022-x64
    goto :deploy
)
if /i "%CHOICE%"=="Q" exit /b 0
echo Invalid choice.
goto :menu

:all
echo.
echo  ================================================
echo   Step 1: Generating project files
echo  ================================================
echo  Generator: %VS_GEN%
echo  Build dir: %BUILD_DIR%
echo.

REM --- Optional config.yaml override ----------------------------------------
REM If config.yaml is present at the repo root, convert it to a CMake initial
REM cache and pass it with -C so users can set build options (USE_OPENAL,
REM product name, version, etc.) without editing CMakeLists.txt.
set "CMAKE_CONFIG_ARG="
if exist config.yaml (
    echo  Applying config.yaml ...
    call cmake\parse_yaml.cmd config.yaml "%BUILD_DIR%\config_cache.cmake" "%CMAKE_BIN%"
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Failed to parse config.yaml.
        exit /b 1
    )
    set "CMAKE_CONFIG_ARG=-C %BUILD_DIR%\config_cache.cmake"
    echo.
)

REM Let CMake create the build directory; do not pre-create build/ here.
%CMAKE_BIN% -S . -B %BUILD_DIR% -G %VS_GEN% -A x64 !CMAKE_CONFIG_ARG!
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake project generation failed.
    exit /b 1
)

echo.
echo  ================================================
echo   Step 2: Building %BUILD_CONFIG% configuration
echo  ================================================
echo.

%CMAKE_BIN% --build %BUILD_DIR% --config %BUILD_CONFIG% --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed. See output above.
    exit /b 1
)

echo.
echo  ================================================
echo   BUILD SUCCEEDED
echo  ================================================
echo.
echo   Configuration: %BUILD_CONFIG%
echo   Output:         %BUILD_DIR%\release-windows-amd64\%BUILD_CONFIG%\
echo.
echo   Open in VS:     %BUILD_DIR%\Tremulous.sln
echo.

REM Check for SDL3 DLL
if not exist "%BUILD_DIR%\release-windows-amd64\%BUILD_CONFIG%\SDL3.dll" (
    echo [WARN] SDL3.dll not in output dir. Check build output.
)

goto :eof

:gui
echo.
echo  Opening cmake-gui for manual configuration...
echo  Source dir:  %CD%
echo  Build dir:   %BUILD_ROOT%\build-vs2022-x64
echo.

REM Resolve the cmake-gui executable. It normally lives next to cmake.exe;
REM derive its directory from the cmake binary on PATH.
set "CMAKE_GUI="
for /f "delims=" %%P in ('where "%CMAKE_BIN%" 2^>nul') do (
    for %%D in ("%%P") do (
        if exist "%%~dpDcmake-gui.exe" set "CMAKE_GUI=%%~dpDcmake-gui.exe"
    )
)
if "%CMAKE_GUI%"=="" set "CMAKE_GUI=cmake-gui"

REM Launch the GUI with source and build paths pre-filled. cmake-gui is a
REM GUI app, so this call returns immediately after the window opens.
start "" "%CMAKE_GUI%" -S "%CD%" -B "%CD%\%BUILD_ROOT%\build-vs2022-x64" 2>nul
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Could not launch cmake-gui ^(looked for: %CMAKE_GUI%^).
    echo Open cmake-gui manually and set:
    echo   Source: %CD%
    echo   Build:  %CD%\%BUILD_ROOT%\build-vs2022-x64
) else (
    echo  cmake-gui launched. Configure and Generate from the GUI.
)

goto :eof

:clean
echo.
echo  Cleaning all build directories in %BUILD_ROOT%\ ...
if not exist %BUILD_ROOT% (
    echo  Nothing to clean.
    goto :eof
)
rmdir /s /q %BUILD_ROOT%
if exist %BUILD_ROOT% (
    echo  [WARN] Could not fully remove %BUILD_ROOT% ^(files locked?^).
) else (
    echo  Done.
)
goto :eof

:deploy
echo.
echo  ================================================
echo   Deploying binaries to game installation
echo  ================================================
echo.

set INSTALL_DIR=%~2
if not "%INSTALL_DIR%"=="" goto :deploy_have_dir
echo.
set /p INSTALL_DIR="Enter install directory [F:\Program Files\Tremulous]: "
if "%INSTALL_DIR%"=="" set INSTALL_DIR=F:\Program Files\Tremulous
:deploy_have_dir

if not exist "%BUILD_DIR%\release-windows-amd64\%BUILD_CONFIG%\tremulous.exe" goto :deploy_no_build
if not exist "%INSTALL_DIR%" goto :deploy_no_install

echo  Deploying to: %INSTALL_DIR%
echo.

set OUT_DIR=%BUILD_DIR%\release-windows-amd64\%BUILD_CONFIG%

copy /Y "%OUT_DIR%\tremulous.exe" "%INSTALL_DIR%\" >nul 2>nul
copy /Y "%OUT_DIR%\tremded.exe" "%INSTALL_DIR%\" >nul 2>nul
copy /Y "%OUT_DIR%\renderergl1.dll" "%INSTALL_DIR%\" >nul 2>nul
copy /Y "%OUT_DIR%\renderergl2.dll" "%INSTALL_DIR%\" >nul 2>nul
copy /Y "%OUT_DIR%\SDL3.dll" "%INSTALL_DIR%\" >nul 2>nul

REM Copy game modules to gpp/ subdirectory
if not exist "%INSTALL_DIR%\gpp" mkdir "%INSTALL_DIR%\gpp"
copy /Y "%OUT_DIR%\cgame.dll" "%INSTALL_DIR%\gpp\" >nul 2>nul
copy /Y "%OUT_DIR%\game.dll" "%INSTALL_DIR%\gpp\" >nul 2>nul
copy /Y "%OUT_DIR%\ui.dll" "%INSTALL_DIR%\gpp\" >nul 2>nul

echo  [OK] Deployment complete.
echo  Binaries copied to %INSTALL_DIR%\
echo  Game modules copied to %INSTALL_DIR%\gpp\
echo.

goto :eof

:deploy_no_build
echo [ERROR] No build found at %BUILD_DIR%\release-windows-amd64\%BUILD_CONFIG%\
echo Run 'build.bat vs2022' first.
exit /b 1

:deploy_no_install
echo [ERROR] Install directory does not exist: %INSTALL_DIR%
echo Override with: build.bat deploy "C:\your\path"
exit /b 1
