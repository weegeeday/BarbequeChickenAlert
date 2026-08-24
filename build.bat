@echo off
if "%1"=="clean" (
    echo Cleaning build artifacts...
    if exist build rmdir /s /q build
    if exist BarbequeChickenAlert.elf del /f /q BarbequeChickenAlert.elf
    if exist BarbequeChickenAlert.nds del /f /q BarbequeChickenAlert.nds
    echo Clean completed.
    exit /b 0
)

echo Building Barbeque Chicken Alert for Nintendo DS...
set DEVKITARM=C:/devkitPro/devkitARM
set DEVKITPRO=C:/devkitPro
set PATH=C:\devkitPro\devkitARM\bin;C:\devkitPro\tools\bin;C:\devkitPro\msys2\usr\bin;%PATH%

python convert_assets.py
make %*
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Build failed with error code %ERRORLEVEL%!
    exit /b %ERRORLEVEL%
)

echo.
echo Build Succeeded! BarbequeChickenAlert.nds is ready.
