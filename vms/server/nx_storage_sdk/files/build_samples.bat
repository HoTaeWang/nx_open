:: Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

@echo off
setlocal %= Reset errorlevel and prohibit changing env vars of the parent shell. =%

if [%1] == [/?] goto :show_usage
if [%1] == [-h] goto :show_usage
if [%1] == [--help] goto :show_usage
goto :skip_show_usage
:show_usage
    echo Usage: %~n0%~x0 [--debug] [^<cmake-generation-args^>...]
    echo  --debug Compile using Debug configuration (without optimizations) instead of Release.
    goto :exit
:skip_show_usage

:: Make the build dir at the same level as the parent dir of this script, suffixed with "-build".
set BASE_DIR_WITH_BACKSLASH=%~dp0
set BASE_DIR=%BASE_DIR_WITH_BACKSLASH:~0,-1%
set BUILD_DIR=%BASE_DIR%-build

echo on
call "%BASE_DIR%/call_vcvars64.bat" || goto :exit
@echo off

if [%1] == [--debug] (
    shift
    set BUILD_TYPE=Debug
) else (
    set BUILD_TYPE=Release
)

:: Use Ninja generator to avoid using CMake compiler search heuristics for MSBuild. Also, specify
:: the compiler explicitly to avoid potential clashes with gcc if run from a Cygwin/MinGW/GitBash
:: shell.
set GENERATOR_OPTIONS=-GNinja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_C_COMPILER=cl.exe -DCMAKE_CXX_COMPILER=cl.exe

echo on
    rmdir /S /Q "%BUILD_DIR%" 2>NUL
@echo off

for /d %%S in (%BASE_DIR%\samples\*) do (
    call :build_sample %%S %1 %2 %3 %4 %5 %6 %7 %8 %9 || goto :exit
)

echo:
echo Samples built successfully, see the binaries in %BUILD_DIR%
goto :exit

:build_sample
    set SOURCE_DIR=%1
    set SAMPLE=%~n1
    shift
    set SAMPLE_BUILD_DIR=%BUILD_DIR%\%SAMPLE%
    @echo on
        mkdir "%SAMPLE_BUILD_DIR%" || @exit /b
        cd "%SAMPLE_BUILD_DIR%" || @exit /b

        cmake "%SOURCE_DIR%\src" %GENERATOR_OPTIONS% %1 %2 %3 %4 %5 %6 %7 %8 %9 || @exit /b
        cmake --build . || @exit /b
    @echo off
    set ARTIFACT=%SAMPLE_BUILD_DIR%\%SAMPLE%.dll
    if not exist "%ARTIFACT%" (
        echo ERROR: Failed to build plugin %SAMPLE%.
        exit /b 70
    )
    echo:
    echo Plugin built: %ARTIFACT%
exit /b

:exit
    exit /b %ERRORLEVEL% %= Needed for a proper cmd.exe exit status. =%
