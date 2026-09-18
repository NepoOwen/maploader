@echo off
setlocal enabledelayedexpansion

REM Set the name of the process to inject into
set "process_name=Notepad.exe"

REM Get the directory where this batch file is located
set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo Looking for %process_name% process...

REM Get PID of %process_name%
for /f "tokens=2" %%a in ('tasklist /FI "IMAGENAME eq %process_name%" /NH 2^>nul ^| findstr /i /c:"%process_name%"') do (
    set "TROVE_PID=%%a"
    goto :found
)

echo Error: %process_name% is not running!
echo Please start %process_name% first.
exit /b 1

:found
echo Found %process_name% with PID: !TROVE_PID!
echo.

if not exist "Injector-x64.exe" (
    echo Error: Injector-x64.exe not found!
    echo Please build the project first.
    pause
    exit /b 1
)

echo Injecting into %process_name%...
"Injector-x64.exe" !TROVE_PID! "field1" "field2" "field3"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Injection completed!
) else (
    echo.
    echo Injection failed! Error code: %ERRORLEVEL%
)

endlocal
