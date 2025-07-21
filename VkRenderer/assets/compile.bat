@echo off
setlocal enabledelayedexpansion

set GLSLC=C:\VulkanSDK\1.4.313.0\Bin\glslc.exe
set OUTPUT_DIR=spv

if not exist "%OUTPUT_DIR%" (
    mkdir "%OUTPUT_DIR%"
    echo Created folder: %OUTPUT_DIR%
)

echo Starting shader compilation...
echo -------------------------------

for %%f in (*.vert *.frag) do (
    set "name=%%~nf"
    set "ext=%%~xf"
    
    if "%%~xf"==".vert" (
        set "stage=vert"
        set "suffix=Vert.spv"
    ) else if "%%~xf"==".frag" (
        set "stage=frag"
        set "suffix=Frag.spv"
    )
    
    echo Compiling %%f to %OUTPUT_DIR%\!name!!suffix!
    "%GLSLC%" -fshader-stage=!stage! -I . "%%f" -o "%OUTPUT_DIR%\!name!!suffix!"
)

echo -------------------------------
echo Compilation complete
pause