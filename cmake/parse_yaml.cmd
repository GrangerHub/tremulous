@echo off
REM ===========================================================================
REM parse_yaml.cmd  --  thin wrapper around cmake/parse_yaml.cmake
REM
REM Converts config.yaml into a CMake initial-cache script. build.bat passes
REM that script to CMake with -C so users can configure the build (feature
REM toggles, product name, version) without editing CMakeLists.txt.
REM
REM Usage:
REM     call cmake\parse_yaml.cmd <input.yaml> [output_cache.cmake] [cmake.exe]
REM
REM The conversion itself is done by cmake -P (cmake has real string
REM handling), which avoids all batch quoting pitfalls for values that
REM contain spaces or special characters.
REM ===========================================================================

setlocal

set "IN=%~1"
if "%IN%"=="" (
    >&2 echo [ERROR] parse_yaml.cmd: missing YAML input path.
    exit /b 1
)
set "OUT=%~2"
if "%OUT%"=="" set "OUT=build\config_cache.cmake"
set "CMAKE=%~3"
if "%CMAKE%"=="" set "CMAKE=cmake"

REM Ensure parent dir of OUT exists
for %%I in ("%OUT%") do if not exist "%%~dpI" mkdir "%%~dpI" 2>nul

"%CMAKE%" -DIN="%IN%" -DOUT="%OUT%" -P "%~dp0parse_yaml.cmake"
set "RC=%errorlevel%"

endlocal & set "PARSE_YAML_OUT=%OUT%" & exit /b %RC%
