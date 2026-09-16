#pragma once

#include <windows.h>
#include <string>
#include "../core/types.h"

namespace SoftwareCenter208 {

class CustomSizeDialog {
public:
    static bool ShowModal(
        HWND hParent,
        HINSTANCE hInstance,
        PhotoPrintSize& outPrintSize
    );

private:
    CustomSizeDialog(
        HWND hParent,
        HINSTANCE hInstance,
        PhotoPrintSize& outPrintSize
    );
    ~CustomSizeDialog();

    bool CreateAndRun();

    static LRESULT CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitControls();
    void OnConfirm();

    HWND m_hwnd = nullptr;
    HWND m_hParent = nullptr;
    HINSTANCE m_hInstance = nullptr;

    HFONT m_hFontUI = nullptr;
    HFONT m_hFontBold = nullptr;
    HFONT m_hFontTitle = nullptr;

    HWND m_hEditWidth = nullptr;
    HWND m_hEditHeight = nullptr;
    HWND m_hBtnMinusQty = nullptr;
    HWND m_hEditQty = nullptr;
    HWND m_hBtnPlusQty = nullptr;
    HWND m_hEditLabel = nullptr;
    HWND m_hBtnOk = nullptr;
    HWND m_hBtnCancel = nullptr;

    PhotoPrintSize& m_outPrintSize;
    int m_qty = 1;
    bool m_confirmed = false;
};

} // namespace SoftwareCenter208
