@echo off
setlocal

set "REPO_ROOT=%~dp0..\.."
set "RUNNER=%REPO_ROOT%\build\Debug\NES_Nestest.exe"
set "ROM=%~dp0artifacts\nestest.nes"
set "LOG=%~dp0artifacts\nestest.log"
set "OPCODE_MODE="

if /I "%~1"=="--all" set "OPCODE_MODE=--all"

if not exist "%RUNNER%" (
    echo NES_Nestest executable not found. Build the NES_Nestest CMake target first.
    exit /b 2
)

if not exist "%ROM%" (
    echo Missing ROM: %ROM%
    exit /b 2
)

if not exist "%LOG%" (
    echo Missing reference log: %LOG%
    exit /b 2
)

"%RUNNER%" %OPCODE_MODE% "%ROM%" "%LOG%"
exit /b %ERRORLEVEL%
