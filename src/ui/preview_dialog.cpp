#include "preview_dialog.h"
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>

namespace SoftwareCenter208 {

enum PreviewControlIds {
    IDC_PREV_BTN_PRINT = 2001,
    IDC_PREV_BTN_PREV,
    IDC_PREV_STATIC_PAGE,
    IDC_PREV_BTN_NEXT,
    IDC_PREV_BTN_ZOOM_IN,
    IDC_PREV_BTN_ZOOM_OUT,
    IDC_PREV_BTN_ZOOM_FIT,
    IDC_PREV_BTN_CLOSE
};

PrintPreviewDialog::PrintPreviewDialog(
    HWND hParent,
    HINSTANCE hInstance,
    const LayoutResult& layout,
    const PaperConfig& paper,
    ImageProcessor& imageProcessor,
    int initialPage
) : m_hParent(hParent),
    m_hInstance(hInstance),
    m_layout(layout),
    m_paper(paper),
    m_imageProcessor(imageProcessor),
    m_currentPage(initialPage) {
    if (m_currentPage < 0) m_currentPage = 0;
    if (!m_layout.pages.empty() && m_currentPage >= static_cast<int>(m_layout.pages.size())) {
        m_currentPage = static_cast<int>(m_layout.pages.size()) - 1;
    }
}

PrintPreviewDialog::~PrintPreviewDialog() {
    if (m_hFontUI) DeleteObject(m_hFontUI);
    if (m_hFontBold) DeleteObject(m_hFontBold);
}

bool PrintPreviewDialog::ShowModal(
    HWND hParent,
    HINSTANCE hInstance,
    const LayoutResult& layout,
    const PaperConfig& paper,
    ImageProcessor& imageProcessor,
    int initialPage
) {
    PrintPreviewDialog dlg(hParent, hInstance, layout, paper, imageProcessor, initialPage);
    return dlg.CreateAndRun();
}

bool PrintPreviewDialog::CreateAndRun() {
    const wchar_t CLASS_NAME[] = L"SoftwareCenter208_PrintPreviewDialogClass";

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    if (!GetClassInfoExW(m_hInstance, CLASS_NAME, &wc)) {
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = PrintPreviewDialog::DialogProc;
        wc.hInstance = m_hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.style = CS_HREDRAW | CS_VREDRAW;

        RegisterClassExW(&wc);
    }

    // Hitung posisi center relatif terhadap parent
    RECT rcParent = {};
    if (m_hParent && IsWindow(m_hParent)) {
        GetWindowRect(m_hParent, &rcParent);
    } else {
        rcParent.left = 100;
        rcParent.top = 100;
        rcParent.right = 1100;
        rcParent.bottom = 850;
    }

    int parentW = rcParent.right - rcParent.left;
    int parentH = rcParent.bottom - rcParent.top;
    int dlgW = std::min(1020, parentW > 400 ? parentW - 40 : 960);
    int dlgH = std::min(780, parentH > 300 ? parentH - 30 : 720);
    int dlgX = rcParent.left + (parentW - dlgW) / 2;
    int dlgY = rcParent.top + (parentH - dlgH) / 2;

    if (dlgX < 10) dlgX = 10;
    if (dlgY < 10) dlgY = 10;

    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        CLASS_NAME,
        L"Pratinjau Cetak (Print Preview) - Pas Foto A4",
        WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        dlgX, dlgY, dlgW, dlgH,
        m_hParent, nullptr, m_hInstance, this
    );

    if (!m_hwnd) return false;

    // Nonaktifkan parent window untuk modalitas
    if (m_hParent && IsWindow(m_hParent)) {
        EnableWindow(m_hParent, FALSE);
    }

    InitControls();
    UpdatePageControls();

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    // Modal Message Loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsWindow(m_hwnd)) {
            break;
        }

        // Handle keyboard navigation di dialog preview
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE) {
                DestroyWindow(m_hwnd);
                break;
            } else if (msg.wParam == 'P' && (GetKeyState(VK_CONTROL) & 0x8000)) {
                m_shouldPrint = true;
                DestroyWindow(m_hwnd);
                break;
            } else if (msg.wParam == VK_LEFT || msg.wParam == VK_PRIOR) {
                if (m_currentPage > 0) {
                    m_currentPage--;
                    UpdatePageControls();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
                continue;
            } else if (msg.wParam == VK_RIGHT || msg.wParam == VK_NEXT) {
                if (!m_layout.pages.empty() && m_currentPage < static_cast<int>(m_layout.pages.size()) - 1) {
                    m_currentPage++;
                    UpdatePageControls();
                    InvalidateRect(m_hwnd, nullptr, FALSE);
                }
                continue;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Aktifkan kembali parent window
    if (m_hParent && IsWindow(m_hParent)) {
        EnableWindow(m_hParent, TRUE);
        SetForegroundWindow(m_hParent);
    }

    return m_shouldPrint;
}

LRESULT CALLBACK PrintPreviewDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    PrintPreviewDialog* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<PrintPreviewDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<PrintPreviewDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void PrintPreviewDialog::InitControls() {
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

    // 1. Tombol Cetak Utama
    m_hBtnPrint = CreateWindowExW(
        0, L"BUTTON", L"🖨️ Cetak ke Printer (Ctrl+P)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 8, 190, 32,
        m_hwnd, (HMENU)IDC_PREV_BTN_PRINT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPrint, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // 2. Tombol Navigasi Halaman
    m_hBtnPrev = CreateWindowExW(
        0, L"BUTTON", L"◀ Sebelumnya",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        210, 8, 110, 32,
        m_hwnd, (HMENU)IDC_PREV_BTN_PREV, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPrev, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hStaticPage = CreateWindowExW(
        0, L"STATIC", L"Halaman 1 dari 1",
        WS_CHILD | WS_VISIBLE | SS_CENTER | SS_CENTERIMAGE,
        325, 8, 140, 32,
        m_hwnd, (HMENU)IDC_PREV_STATIC_PAGE, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticPage, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnNext = CreateWindowExW(
        0, L"BUTTON", L"Berikutnya ▶",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        470, 8, 110, 32,
        m_hwnd, (HMENU)IDC_PREV_BTN_NEXT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnNext, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // 3. Zoom Controls
    m_hBtnZoomIn = CreateWindowExW(
        0, L"BUTTON", L"Zoom +",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        590, 8, 70, 32,
        m_hwnd, (HMENU)IDC_PREV_BTN_ZOOM_IN, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnZoomIn, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnZoomOut = CreateWindowExW(
        0, L"BUTTON", L"Zoom -",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        665, 8, 70, 32,
        m_hwnd, (HMENU)IDC_PREV_BTN_ZOOM_OUT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnZoomOut, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnZoomFit = CreateWindowExW(
        0, L"BUTTON", L"Pas Layar (100%)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        740, 8, 120, 32,
        m_hwnd, (HMENU)IDC_PREV_BTN_ZOOM_FIT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnZoomFit, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // 4. Tombol Tutup
    m_hBtnClose = CreateWindowExW(
        0, L"BUTTON", L"✖ Tutup (Esc)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        870, 8, 100, 32,
        m_hwnd, (HMENU)IDC_PREV_BTN_CLOSE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnClose, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);
}

void PrintPreviewDialog::UpdatePageControls() {
    int totalPages = m_layout.totalPages > 0 ? m_layout.totalPages : 1;
    if (m_currentPage < 0) m_currentPage = 0;
    if (m_currentPage >= totalPages) m_currentPage = totalPages - 1;

    wchar_t pageText[64];
    swprintf_s(pageText, L"Halaman %d dari %d", m_currentPage + 1, totalPages);
    SetWindowTextW(m_hStaticPage, pageText);

    EnableWindow(m_hBtnPrev, m_currentPage > 0 ? TRUE : FALSE);
    EnableWindow(m_hBtnNext, m_currentPage < totalPages - 1 ? TRUE : FALSE);
}

void PrintPreviewDialog::ResizeLayout(int clientWidth, int clientHeight) {
    if (m_hBtnClose) {
        int closeX = clientWidth - 115;
        if (closeX > 800) {
            SetWindowPos(m_hBtnClose, nullptr, closeX, 8, 105, 32, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

void PrintPreviewDialog::DrawPreviewCanvas(HDC hdc, const RECT& canvasRect) {
    int viewW = canvasRect.right - canvasRect.left;
    int viewH = canvasRect.bottom - canvasRect.top;
    if (viewW <= 0 || viewH <= 0) return;

    // Double buffer
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, viewW, viewH);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    // 1. Background studio gelap (Professional Dark Slate)
    HBRUSH bgBrush = CreateSolidBrush(RGB(50, 52, 56));
    RECT localRect = { 0, 0, viewW, viewH };
    FillRect(memDC, &localRect, bgBrush);
    DeleteObject(bgBrush);

    // 2. Hitung dimensi dan posisi kertas A4
    double aspect = m_paper.heightMm / m_paper.widthMm; // ~ 1.414 (A4)
    int margin = 20;

    int maxSheetW = viewW - (margin * 2);
    int maxSheetH = viewH - (margin * 2);

    int sheetW = maxSheetW;
    int sheetH = static_cast<int>(sheetW * aspect);

    if (sheetH > maxSheetH) {
        sheetH = maxSheetH;
        sheetW = static_cast<int>(sheetH / aspect);
    }

    // Terapkan Zoom Factor
    sheetW = static_cast<int>(sheetW * m_zoomFactor);
    sheetH = static_cast<int>(sheetH * m_zoomFactor);

    int sheetX = (viewW - sheetW) / 2;
    int sheetY = (viewH - sheetH) / 2;

    {
        Gdiplus::Graphics g(memDC);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        // 3. Realistic Drop Shadow di sekeliling kertas
        Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(60, 0, 0, 0));
        g.FillRectangle(&shadowBrush, sheetX + 6, sheetY + 6, sheetW, sheetH);

        Gdiplus::SolidBrush shadowSoftBrush(Gdiplus::Color(30, 0, 0, 0));
        g.FillRectangle(&shadowSoftBrush, sheetX + 3, sheetY + 3, sheetW, sheetH);

        // 4. Kertas Putih Bersih
        Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255));
        g.FillRectangle(&whiteBrush, sheetX, sheetY, sheetW, sheetH);

        // 5. Render Sheet Layout dengan DPI dan Skala
        double renderScale = static_cast<double>(sheetW) / MmToPixels(m_paper.widthMm, 300.0);

        Gdiplus::GraphicsState gState = g.Save();
        g.TranslateTransform(static_cast<Gdiplus::REAL>(sheetX), static_cast<Gdiplus::REAL>(sheetY));

        if (!m_layout.pages.empty() && m_currentPage >= 0 && m_currentPage < static_cast<int>(m_layout.pages.size())) {
            m_imageProcessor.RenderSheet(g, m_layout.pages[m_currentPage], m_paper, 300.0, true, renderScale);
        } else {
            PageLayout emptyPage;
            emptyPage.pageIndex = 0;
            emptyPage.usedHeightMm = 0.0;
            emptyPage.remainingHeightMm = m_paper.heightMm;
            m_imageProcessor.RenderSheet(g, emptyPage, m_paper, 300.0, true, renderScale);
        }

        g.Restore(gState);

        // 6. Border Tipis Kertas A4
        Gdiplus::Pen borderPen(Gdiplus::Color(160, 160, 160), 1.0f);
        g.DrawRectangle(&borderPen, sheetX, sheetY, sheetW, sheetH);
    }

    BitBlt(hdc, canvasRect.left, canvasRect.top, viewW, viewH, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

LRESULT PrintPreviewDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_PREV_BTN_PRINT:
            m_shouldPrint = true;
            DestroyWindow(m_hwnd);
            return 0;

        case IDC_PREV_BTN_PREV:
            if (m_currentPage > 0) {
                m_currentPage--;
                UpdatePageControls();
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;

        case IDC_PREV_BTN_NEXT:
            if (!m_layout.pages.empty() && m_currentPage < static_cast<int>(m_layout.pages.size()) - 1) {
                m_currentPage++;
                UpdatePageControls();
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;

        case IDC_PREV_BTN_ZOOM_IN:
            if (m_zoomFactor < 2.5) {
                m_zoomFactor += 0.2;
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;

        case IDC_PREV_BTN_ZOOM_OUT:
            if (m_zoomFactor > 0.5) {
                m_zoomFactor -= 0.2;
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
            return 0;

        case IDC_PREV_BTN_ZOOM_FIT:
            m_zoomFactor = 1.0;
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;

        case IDC_PREV_BTN_CLOSE:
            m_shouldPrint = false;
            DestroyWindow(m_hwnd);
            return 0;
        }
        break;
    }

    case WM_MOUSEWHEEL: {
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        if (GetKeyState(VK_CONTROL) & 0x8000) {
            // Ctrl + Wheel = Zoom
            if (delta > 0 && m_zoomFactor < 2.5) {
                m_zoomFactor += 0.15;
            } else if (delta < 0 && m_zoomFactor > 0.5) {
                m_zoomFactor -= 0.15;
            }
        } else {
            // Wheel = Navigasi Halaman
            if (delta > 0 && m_currentPage > 0) {
                m_currentPage--;
                UpdatePageControls();
            } else if (delta < 0 && !m_layout.pages.empty() && m_currentPage < static_cast<int>(m_layout.pages.size()) - 1) {
                m_currentPage++;
                UpdatePageControls();
            }
        }
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_SIZE: {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        ResizeLayout(width, height);
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // Handled in WM_PAINT

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
        SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);

        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);

        // 1. Toolbar area di atas
        RECT rcToolbar = { 0, 0, rcClient.right, 48 };
        HBRUSH hbrToolbar = GetSysColorBrush(COLOR_BTNFACE);
        FillRect(hdc, &rcToolbar, hbrToolbar);

        // Garis pemisah toolbar
        HPEN hPenSep = CreatePen(PS_SOLID, 1, RGB(190, 190, 190));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPenSep);
        MoveToEx(hdc, 0, 48, nullptr);
        LineTo(hdc, rcClient.right, 48);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPenSep);

        // 2. Canvas preview area di bawah toolbar
        RECT rcCanvas = { 0, 49, rcClient.right, rcClient.bottom };
        if (rcCanvas.right > rcCanvas.left && rcCanvas.bottom > rcCanvas.top) {
            DrawPreviewCanvas(hdc, rcCanvas);
        }

        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        m_shouldPrint = false;
        DestroyWindow(m_hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

} // namespace SoftwareCenter208
