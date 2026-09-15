@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo  208 Software Center - Local Root CA Certificate Installer
echo ========================================================
echo.

:: Check for administrative privileges
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] Membutuhkan hak akses Administrator...
    echo [INFO] Menjalankan User Account Control (UAC) Elevation...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

set "CER_PATH=%~dp0build\208softwarecenter.cer"
if not exist "!CER_PATH!" (
    set "CER_PATH=%~dp0208softwarecenter.cer"
)

if not exist "!CER_PATH!" (
    echo [ERROR] File sertifikat 208softwarecenter.cer tidak ditemukan!
    pause
    exit /b 1
)

echo Menginstall sertifikat 'Toko Fotokopi 208' ke Trusted Root CA...
certutil -addstore -f "Root" "!CER_PATH!"

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================================
    echo  [BERHASIL] Sertifikat resmi lokal terinstall!
    echo  Aplikasi 208 Software Center sekarang berstatus:
    echo  "Verified Publisher: Toko Fotokopi 208"
    echo ========================================================
) else (
    echo.
    echo [GAGAL] Gagal menginstall sertifikat. Error code: %ERRORLEVEL%
)

echo.
pause
