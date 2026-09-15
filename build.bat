@echo off
setlocal enabledelayedexpansion

echo =======================================================
echo  208 Software Center - Build Script (WinXP to Win11)
echo =======================================================

:: Check and add Scoop MinGW to PATH if needed
where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    if exist "%USERPROFILE%\scoop\apps\mingw-winlibs\current\bin" (
        set "PATH=%USERPROFILE%\scoop\apps\mingw-winlibs\current\bin;!PATH!"
    )
)

if not exist bin mkdir bin
if not exist build mkdir build

echo [1/3] Compiling Resources and Manifest...
windres -i src\resource.rc -O coff -o build\resource.o
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Gagal meng-compile resource.rc!
    exit /b %ERRORLEVEL%
)

echo [2/3] Compiling C++ Win32 GDI+ Sources...
g++ -std=c++17 -O2 -mwindows ^
    -static -static-libgcc -static-libstdc++ ^
    -specs=build/no-default-manifest.specs ^
    -Wl,--subsystem,windows:5.1 ^
    -Isrc ^
    src\main.cpp ^
    src\core\packing_engine.cpp ^
    src\graphics\image_processor.cpp ^
    src\ui\main_window.cpp ^
    src\ui\preview_dialog.cpp ^
    build\resource.o ^
    -o bin\208softwarecenter.exe ^
    -lgdiplus -lgdi32 -lcomctl32 -lcomdlg32 -lole32 -lshell32 -lshlwapi -luxtheme -lmsimg32 -lwinspool

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Gagal melakukan linking binary!
    exit /b %ERRORLEVEL%
)

echo [3/3] Build Berhasil: bin\208softwarecenter.exe
echo =======================================================
