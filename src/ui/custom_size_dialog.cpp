#include "custom_size_dialog.h"
#include <windowsx.h>
#include <cwchar>
#include <algorithm>

namespace SoftwareCenter208 {

enum CustomSizeControlIds {
    IDC_CUSTOM_EDIT_WIDTH = 4001,
    IDC_CUSTOM_EDIT_HEIGHT,
    IDC_CUSTOM_BTN_MINUS_QTY,
    IDC_CUSTOM_EDIT_QTY,
    IDC_CUSTOM_BTN_PLUS_QTY,
    IDC_CUSTOM_EDIT_LABEL,
    IDC_CUSTOM_BTN_OK,
    IDC_CUSTOM_BTN_CANCEL
};

CustomSizeDialog::CustomSizeDialog(
    HWND hParent,
    HINSTANCE hInstance,
    PhotoPrintSize& outPrintSize
) : m_hParent(hParent),
    m_hInstance(hInstance),
    m_outPrintSize(outPrintSize),
    m_qty(outPrintSize.quantity > 0 ? outPrintSize.quantity : 1) {
}

CustomSizeDialog::~CustomSizeDialog() {
    if (m_hFontUI) DeleteObject(m_hFontUI);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
}

bool CustomSizeDialog::ShowModal(
    HWND hParent,
    HINSTANCE hInstance,
    PhotoPrintSize& outPrintSize
) {
    CustomSizeDialog dlg(hParent, hInstance, outPrintSize);
    return dlg.CreateAndRun();
}

bool CustomSizeDialog::CreateAndRun() {
    const wchar_t CLASS_NAME[] = L"SoftwareCenter208_CustomSizeDialogClass";

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    if (!GetClassInfoExW(m_hInstance, CLASS_NAME, &wc)) {
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = CustomSizeDialog::DialogProc;
        wc.hInstance = m_hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.style = CS_HREDRAW | CS_VREDRAW;

        RegisterClassExW(&wc);
    }

    RECT rcParent = {};
    if (m_hParent && IsWindow(m_hParent)) {
        GetWindowRect(m_hParent, &rcParent);
    } else {
        rcParent.left = 150;
        rcParent.top = 150;
        rcParent.right = 800;
        rcParent.bottom = 600;
    }

    int dlgW = 400;
    int dlgH = 340;
    int x = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (x < 10) x = 10;
    if (y < 10) y = 10;

    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        CLASS_NAME,
        L"Tambah Ukuran Kustom (cm)",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH,
        m_hParent, nullptr, m_hInstance, this
    );

    if (!m_hwnd) return false;

    if (m_hParent && IsWindow(m_hParent)) {
        EnableWindow(m_hParent, FALSE);
    }

    InitControls();

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE) {
                DestroyWindow(m_hwnd);
                break;
            } else if (msg.wParam == VK_RETURN) {
                OnConfirm();
                break;
            }
        }

        if (!IsDialogMessageW(m_hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        if (!IsWindow(m_hwnd)) break;
    }

    if (m_hParent && IsWindow(m_hParent)) {
        EnableWindow(m_hParent, TRUE);
        SetForegroundWindow(m_hParent);
    }

    return m_confirmed;
}

LRESULT CALLBACK CustomSizeDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CustomSizeDialog* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<CustomSizeDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<CustomSizeDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void CustomSizeDialog::InitControls() {
    m_hFontUI = CreateFontW(
        -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    m_hFontBold = CreateFontW(
        -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    m_hFontTitle = CreateFontW(
        -14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    // Title label
    HWND hTitle = CreateWindowExW(
        0, L"STATIC", L"📐 Masukkan Dimensi Foto Kustom (cm)",
        WS_CHILD | WS_VISIBLE,
        20, 16, 350, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hTitle, WM_SETFONT, (WPARAM)m_hFontTitle, TRUE);

    // Lebar
    HWND hLblW = CreateWindowExW(
        0, L"STATIC", L"Lebar (cm):",
        WS_CHILD | WS_VISIBLE,
        20, 52, 110, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblW, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    wchar_t initialW[32] = L"5.0";
    wchar_t initialH[32] = L"7.0";
    if (m_outPrintSize.widthMm > 0) {
        swprintf_s(initialW, L"%.1f", m_outPrintSize.widthMm / 10.0);
    }
    if (m_outPrintSize.heightMm > 0) {
        swprintf_s(initialH, L"%.1f", m_outPrintSize.heightMm / 10.0);
    }

    m_hEditWidth = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", initialW,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        140, 50, 120, 24,
        m_hwnd, (HMENU)IDC_CUSTOM_EDIT_WIDTH, m_hInstance, nullptr
    );
    SendMessageW(m_hEditWidth, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    HWND hHintW = CreateWindowExW(
        0, L"STATIC", L"cm",
        WS_CHILD | WS_VISIBLE,
        268, 52, 50, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hHintW, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Tinggi
    HWND hLblH = CreateWindowExW(
        0, L"STATIC", L"Tinggi (cm):",
        WS_CHILD | WS_VISIBLE,
        20, 92, 110, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblH, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hEditHeight = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", initialH,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        140, 90, 120, 24,
        m_hwnd, (HMENU)IDC_CUSTOM_EDIT_HEIGHT, m_hInstance, nullptr
    );
    SendMessageW(m_hEditHeight, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    HWND hHintH = CreateWindowExW(
        0, L"STATIC", L"cm",
        WS_CHILD | WS_VISIBLE,
        268, 92, 50, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hHintH, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Jumlah Cetak
    HWND hLblQty = CreateWindowExW(
        0, L"STATIC", L"Jumlah Cetak:",
        WS_CHILD | WS_VISIBLE,
        20, 132, 110, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblQty, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnMinusQty = CreateWindowExW(
        0, L"BUTTON", L"-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        140, 130, 28, 24,
        m_hwnd, (HMENU)IDC_CUSTOM_BTN_MINUS_QTY, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnMinusQty, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hEditQty = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", std::to_wstring(m_qty).c_str(),
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER | WS_TABSTOP,
        172, 130, 56, 24,
        m_hwnd, (HMENU)IDC_CUSTOM_EDIT_QTY, m_hInstance, nullptr
    );
    SendMessageW(m_hEditQty, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnPlusQty = CreateWindowExW(
        0, L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        232, 130, 28, 24,
        m_hwnd, (HMENU)IDC_CUSTOM_BTN_PLUS_QTY, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPlusQty, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND hHintQty = CreateWindowExW(
        0, L"STATIC", L"lembar",
        WS_CHILD | WS_VISIBLE,
        268, 132, 80, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hHintQty, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Label / Keterangan Kustom (Opsional)
    HWND hLblDesc = CreateWindowExW(
        0, L"STATIC", L"Label / Nama:",
        WS_CHILD | WS_VISIBLE,
        20, 172, 110, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblDesc, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hEditLabel = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", m_outPrintSize.label.c_str(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        140, 170, 220, 24,
        m_hwnd, (HMENU)IDC_CUSTOM_EDIT_LABEL, m_hInstance, nullptr
    );
    SendMessageW(m_hEditLabel, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Action buttons
    m_hBtnOk = CreateWindowExW(
        0, L"BUTTON", L"✔ Tambahkan",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
        140, 230, 110, 32,
        m_hwnd, (HMENU)IDC_CUSTOM_BTN_OK, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnOk, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnCancel = CreateWindowExW(
        0, L"BUTTON", L"✖ Batal",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        260, 230, 100, 32,
        m_hwnd, (HMENU)IDC_CUSTOM_BTN_CANCEL, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnCancel, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    SetFocus(m_hEditWidth);
    SendMessageW(m_hEditWidth, EM_SETSEL, 0, -1);
}

void CustomSizeDialog::OnConfirm() {
    WCHAR szW[64] = { 0 };
    WCHAR szH[64] = { 0 };
    WCHAR szQty[64] = { 0 };
    WCHAR szLabel[128] = { 0 };

    GetWindowTextW(m_hEditWidth, szW, 63);
    GetWindowTextW(m_hEditHeight, szH, 63);
    GetWindowTextW(m_hEditQty, szQty, 63);
    GetWindowTextW(m_hEditLabel, szLabel, 127);

    // Replace koma (,) dengan titik (.) jika operator mengetik format lokal Indonesia
    for (int i = 0; szW[i]; ++i) if (szW[i] == L',') szW[i] = L'.';
    for (int i = 0; szH[i]; ++i) if (szH[i] == L',') szH[i] = L'.';

    double widthCm = _wtof(szW);
    double heightCm = _wtof(szH);
    int qty = _wtoi(szQty);

    if (widthCm <= 0.1 || widthCm > 21.0) {
        MessageBoxW(m_hwnd, L"Lebar foto harus antara 0.1 cm dan 21.0 cm (batas kertas A4)!", L"Input Tidak Valid", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEditWidth);
        return;
    }

    if (heightCm <= 0.1 || heightCm > 29.7) {
        MessageBoxW(m_hwnd, L"Tinggi foto harus antara 0.1 cm dan 29.7 cm (batas kertas A4)!", L"Input Tidak Valid", MB_OK | MB_ICONWARNING);
        SetFocus(m_hEditHeight);
        return;
    }

    if (qty <= 0) qty = 1;
    if (qty > 999) qty = 999;

    m_outPrintSize.preset = PhotoSizePreset::Custom;
    m_outPrintSize.widthMm = widthCm * 10.0;
    m_outPrintSize.heightMm = heightCm * 10.0;
    m_outPrintSize.quantity = qty;

    std::wstring labelStr = szLabel;
    // Trim
    while (!labelStr.empty() && (labelStr.front() == L' ' || labelStr.front() == L'\t')) labelStr.erase(labelStr.begin());
    while (!labelStr.empty() && (labelStr.back() == L' ' || labelStr.back() == L'\t')) labelStr.pop_back();

    if (labelStr.empty()) {
        wchar_t autoLabel[128];
        swprintf_s(autoLabel, L"Custom (%.1f x %.1f cm)", widthCm, heightCm);
        m_outPrintSize.label = autoLabel;
    } else {
        m_outPrintSize.label = labelStr;
    }

    m_confirmed = true;
    DestroyWindow(m_hwnd);
}

LRESULT CustomSizeDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            if (wmId == IDC_CUSTOM_BTN_OK) {
                OnConfirm();
                return 0;
            } else if (wmId == IDC_CUSTOM_BTN_CANCEL) {
                DestroyWindow(m_hwnd);
                return 0;
            } else if (wmId == IDC_CUSTOM_BTN_MINUS_QTY) {
                WCHAR szQty[32] = { 0 };
                GetWindowTextW(m_hEditQty, szQty, 31);
                int q = _wtoi(szQty);
                if (q > 1) q--;
                SetWindowTextW(m_hEditQty, std::to_wstring(q).c_str());
                return 0;
            } else if (wmId == IDC_CUSTOM_BTN_PLUS_QTY) {
                WCHAR szQty[32] = { 0 };
                GetWindowTextW(m_hEditQty, szQty, 31);
                int q = _wtoi(szQty);
                if (q < 999) q++;
                SetWindowTextW(m_hEditQty, std::to_wstring(q).c_str());
                return 0;
            }
            break;
        }

        case WM_CLOSE: {
            DestroyWindow(m_hwnd);
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
            SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }

        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

} // namespace SoftwareCenter208
