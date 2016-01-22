@REM Copyright (c) Microsoft. All rights reserved.
@REM Licensed under the MIT license. See LICENSE file in the project root for full license information.

@setlocal EnableExtensions EnableDelayedExpansion
@echo off

set current-path=%~dp0
rem // remove trailing slash
set current-path=%current-path:~0,-1%

echo Current Path: %current-path%

set build-root=%current-path%\..\..
rem // resolve to fully qualified path
for %%i in ("%build-root%") do set build-root=%%~fi

set repo_root=%build-root%\..\..
rem // resolve to fully qualified path
for %%i in ("%repo_root%") do set repo_root=%%~fi

echo Build Root: %build-root%
echo Repo Root: %repo_root%

rem -----------------------------------------------------------------------------
rem -- check prerequisites
rem -----------------------------------------------------------------------------


rem -----------------------------------------------------------------------------
rem -- parse script arguments
rem -----------------------------------------------------------------------------

rem // default build options
set build-clean=0
set build-config=Debug
set build-platform=Win32
set CMAKE_skip_unittests=OFF

:args-loop
if "%1" equ "" goto args-done
if "%1" equ "-c" goto arg-build-clean
if "%1" equ "--clean" goto arg-build-clean
if "%1" equ "--config" goto arg-build-config
if "%1" equ "--platform" goto arg-build-platform
if "%1" equ "--skip-unittests" goto arg-skip-unittests
call :usage && exit /b 1

:arg-build-clean
set build-clean=1
goto args-continue

:arg-build-config
shift
if "%1" equ "" call :usage && exit /b 1
set build-config=%1
goto args-continue

:arg-build-platform
shift
if "%1" equ "" call :usage && exit /b 1
set build-platform=%1
goto args-continue

:arg-skip-unittests
set CMAKE_skip_unittests=ON
goto args-continue

:args-continue
shift
goto args-loop

:args-done

rem -----------------------------------------------------------------------------
rem -- clean solutions
rem -----------------------------------------------------------------------------

if %build-clean%==1 (
    rem -- call nuget restore "%build-root%\samples\mqtt_client_sample\windows\mqtt_client_sample.sln"
    rem -- call :clean-a-solution "%build-root%\samples\mqtt_client_sample\windows\mqtt_client_sample.sln"
    rem -- if not %errorlevel%==0 exit /b %errorlevel%
)

rem -----------------------------------------------------------------------------
rem -- build with CMAKE and run tests
rem -----------------------------------------------------------------------------

rmdir /s/q %USERPROFILE%\azure_mqtt
rem no error checking

mkdir %USERPROFILE%\azure_mqtt
rem no error checking

pushd %USERPROFILE%\azure_mqtt
cmake %build-root% -Dskip_unittests:BOOL=%CMAKE_skip_unittests%
if not %errorlevel%==0 exit /b %errorlevel%

rem msbuild /m umqtt.sln
call :_run-msbuild "Build" umqtt.sln %2 %3
if not %errorlevel%==0 exit /b %errorlevel%

echo Build Config: %build-config%

if "%build-config%" == "Debug" (
    ctest -C "debug" -V
    if not %errorlevel%==0 exit /b %errorlevel%
)

popd
goto :eof

rem -----------------------------------------------------------------------------
rem -- subroutines
rem -----------------------------------------------------------------------------

:clean-a-solution
call :_run-msbuild "Clean" %1 %2 %3
goto :eof

:build-a-solution
call :_run-msbuild "Build" %1 %2 %3
goto :eof

:run-unit-tests
call :_run-tests %1 "UnitTests"
goto :eof

:usage
echo build.cmd [options]
echo options:
echo  -c, --clean             delete artifacts from previous build before building
echo  --config ^<value^>      [Debug] build configuration (e.g. Debug, Release)
echo  --platform ^<value^>    [Win32] build platform (e.g. Win32, x64, ...)
echo  --skip-unittests        skip the unit tests
goto :eof

rem -----------------------------------------------------------------------------
rem -- helper subroutines
rem -----------------------------------------------------------------------------

echo build config: %build-config%
echo platform:     %build-platform%

:_run-msbuild
rem // optionally override configuration|platform
setlocal EnableExtensions
set build-target=
if "%~1" neq "Build" set "build-target=/t:%~1"
if "%~3" neq "" set build-config=%~3
if "%~4" neq "" set build-platform=%~4

msbuild /m %build-target% "/p:Configuration=%build-config%;Platform=%build-platform%" %2
if not %errorlevel%==0 exit /b %errorlevel%
goto :eof

:_run-tests
rem // discover tests
set test-dlls-list=
set test-dlls-path=%build-root%\%~1\build\windows\%build-platform%\%build-config%
for /f %%i in ('dir /b %test-dlls-path%\*%~2*.dll') do set test-dlls-list="%test-dlls-path%\%%i" !test-dlls-list!

if "%test-dlls-list%" equ "" (
    echo No unit tests found in %test-dlls-path%
    exit /b 1
)

rem // run tests
echo Test DLLs: %test-dlls-list%
echo.
vstest.console.exe %test-dlls-list%

if not %errorlevel%==0 exit /b %errorlevel%
goto :eof

echo done