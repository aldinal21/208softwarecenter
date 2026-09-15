#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501 // Target Windows XP SP3+ minimum

#include <windows.h>
#include <objbase.h>
#include <propidl.h>
#include <commctrl.h>
#include <gdiplus.h>
#include "ui/main_window.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "winspool.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;

    // 1. Initialize Common Controls v6 (Manifest embedded)
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX) };
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);

    // 2. Initialize OLE & COM
    OleInitialize(nullptr);

    // 3. Initialize GDI+
    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
        MessageBoxW(nullptr, L"Gagal menginisialisasi GDI+ engine!", L"Error 208 Software Center", MB_OK | MB_ICONERROR);
        return 1;
    }

    int exitCode = 0;
    {
        SoftwareCenter208::MainWindow mainWindow;
        if (mainWindow.Create(hInstance, nCmdShow)) {
            exitCode = mainWindow.RunMessageLoop();
        } else {
            MessageBoxW(nullptr, L"Gagal membuat antarmuka utama (Main Window)!", L"Error 208 Software Center", MB_OK | MB_ICONERROR);
            exitCode = 1;
        }
    }

    // 4. Shutdown GDI+ & OLE
    Gdiplus::GdiplusShutdown(gdiplusToken);
    OleUninitialize();

    return exitCode;
}
