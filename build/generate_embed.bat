@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
cd /d "%SCRIPT_DIR%"

echo Generating embed.hpp from TroveBoat.dll...

if not exist "lua/luajit.exe" (
    echo Error: luajit.exe not found!
    exit /b 1
)

"lua/luajit.exe" "lua/dll_to_embed.lua"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Failed to generate embed.hpp.
    exit /b 1
)

echo.
echo embed.hpp has been generated successfully!
echo.
echo Updating project source and build status...

cd /d "%SCRIPT_DIR%.."
powershell -Command "(Get-Content project\src\main.cpp) -replace '// #define USE_EMBEDDED_DLL', '#define USE_EMBEDDED_DLL' | Set-Content project\src\main.cpp"

if /I "%~1"=="nobuild" (
    echo Skipping injector build because nobuild mode was specified.
    endlocal
    exit /b 0
)

REM may need to change your path
set "MSBUILD=C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe"
goto :build

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe 2^>nul`) do (
    set "MSBUILD=%%i"
    goto :build
)

where msbuild.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    set "MSBUILD=msbuild.exe"
    goto :build
)

echo Error: MSBuild not found! Please build the project manually in Visual Studio.
exit /b 1

:build
cd /d "%SCRIPT_DIR%.."
"%MSBUILD%" "injector.sln" /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo

if %ERRORLEVEL% EQU 0 (
    echo ========================================
    echo Build completed successfully!
    echo ========================================
) else (
    echo.
    echo Build failed! Check the errors above.
    exit /b 1
)

endlocal
