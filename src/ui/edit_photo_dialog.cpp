#include "edit_photo_dialog.h"
#include <windowsx.h>
#include <algorithm>
#include <cmath>

namespace SoftwareCenter208 {

enum EditControlIds {
    IDC_EDIT_BTN_ROTATE_CCW = 3001,
    IDC_EDIT_BTN_ROTATE_CW,
    IDC_EDIT_STATIC_ROTATE,
    IDC_EDIT_SLIDER_ZOOM,
    IDC_EDIT_STATIC_ZOOM_VAL,
    IDC_EDIT_BTN_ZOOM_RESET,
    IDC_EDIT_BTN_ASPECT_2X3,
    IDC_EDIT_BTN_ASPECT_3X4,
    IDC_EDIT_BTN_ASPECT_4X6,
    IDC_EDIT_CHK_BW,
    IDC_EDIT_BTN_RESET_POS,
    IDC_EDIT_BTN_BG_NONE,
    IDC_EDIT_BTN_BG_RED,
    IDC_EDIT_BTN_BG_BLUE,
    IDC_EDIT_BTN_BG_WHITE,
    IDC_EDIT_BTN_BG_GREY,
    IDC_EDIT_BTN_BG_YELLOW,
    IDC_EDIT_SLIDER_BG_TOL,
    IDC_EDIT_STATIC_BG_TOL,
    IDC_EDIT_BTN_SAVE,
    IDC_EDIT_BTN_CANCEL
};

EditPhotoDialog::EditPhotoDialog(
    HWND hParent,
    HINSTANCE hInstance,
    PhotoOrderItem& item,
    ImageProcessor& imageProcessor
) : m_hParent(hParent),
    m_hInstance(hInstance),
    m_item(item),
    m_imageProcessor(imageProcessor),
    m_cropCenterX(item.cropCenterX),
    m_cropCenterY(item.cropCenterY),
    m_zoomLevel(item.zoomLevel),
    m_rotationAngle(item.rotationAngle),
    m_isBlackAndWhite(item.isBlackAndWhite),
    m_bgPreset(item.bgPreset),
    m_customBgColor(item.customBgColor),
    m_bgTolerance(item.bgTolerance) {
    if (m_zoomLevel < 1.0) m_zoomLevel = 1.0;
    if (m_zoomLevel > 3.0) m_zoomLevel = 3.0;
    m_rotationAngle = (m_rotationAngle % 360 + 360) % 360;
    if (m_bgTolerance < 10) m_bgTolerance = 30;

    // Tentukan preset preview awal berdasarkan kuantitas yang dipesan
    if (!item.printSizes.empty()) {
        m_previewPreset = item.printSizes[0].preset;
    } else {
        m_previewPreset = PhotoSizePreset::Size3x4;
    }
}

EditPhotoDialog::~EditPhotoDialog() {
    if (m_hFontUI) DeleteObject(m_hFontUI);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontSmall) DeleteObject(m_hFontSmall);
    if (m_hFontTitle) DeleteObject(m_hFontTitle);
}

bool EditPhotoDialog::ShowModal(
    HWND hParent,
    HINSTANCE hInstance,
    PhotoOrderItem& item,
    ImageProcessor& imageProcessor
) {
    EditPhotoDialog dlg(hParent, hInstance, item, imageProcessor);
    return dlg.CreateAndRun();
}

bool EditPhotoDialog::CreateAndRun() {
    const wchar_t CLASS_NAME[] = L"SoftwareCenter208_EditPhotoDialogClass";

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    if (!GetClassInfoExW(m_hInstance, CLASS_NAME, &wc)) {
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = EditPhotoDialog::DialogProc;
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
        rcParent.right = 1020;
        rcParent.bottom = 800;
    }

    int parentW = rcParent.right - rcParent.left;
    int parentH = rcParent.bottom - rcParent.top;
    int dlgW = 920;
    int dlgH = 700;
    int dlgX = rcParent.left + (parentW - dlgW) / 2;
    int dlgY = rcParent.top + (parentH - dlgH) / 2;

    if (dlgX < 10) dlgX = 10;
    if (dlgY < 10) dlgY = 10;

    m_canvasRect = { 15, 50, 565, 645 };

    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        CLASS_NAME,
        L"Atur Posisi, Orientasi & Background Foto - 208 Software Center",
        WS_POPUPWINDOW | WS_CAPTION,
        dlgX, dlgY, dlgW, dlgH,
        m_hParent, nullptr, m_hInstance, this
    );

    if (!m_hwnd) return false;

    if (m_hParent && IsWindow(m_hParent)) {
        EnableWindow(m_hParent, FALSE);
    }

    InitControls();
    UpdateControlsState();

    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsWindow(m_hwnd)) {
            break;
        }

        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_ESCAPE) {
                DestroyWindow(m_hwnd);
                break;
            } else if (msg.wParam == VK_RETURN) {
                m_item.cropCenterX = m_cropCenterX;
                m_item.cropCenterY = m_cropCenterY;
                m_item.zoomLevel = m_zoomLevel;
                m_item.rotationAngle = m_rotationAngle;
                m_item.isBlackAndWhite = m_isBlackAndWhite;
                m_item.bgPreset = m_bgPreset;
                m_item.customBgColor = m_customBgColor;
                m_item.bgTolerance = m_bgTolerance;
                m_saved = true;
                DestroyWindow(m_hwnd);
                break;
            }
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (m_hParent && IsWindow(m_hParent)) {
        EnableWindow(m_hParent, TRUE);
        SetForegroundWindow(m_hParent);
    }

    return m_saved;
}

LRESULT CALLBACK EditPhotoDialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    EditPhotoDialog* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<EditPhotoDialog*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<EditPhotoDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void EditPhotoDialog::InitControls() {
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
    m_hFontSmall = CreateFontW(
        -11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    m_hFontTitle = CreateFontW(
        -14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    // Header Instruction
    HWND hHeader = CreateWindowExW(
        0, L"STATIC",
        L"💡 Geser mouse pada foto untuk memindahkan posisi wajah. Gunakan slider / scroll mouse untuk zoom.",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        15, 12, 880, 30,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hHeader, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // --- PANEL KANAN: KONTROL PENGATURAN ---
    const int panelX = 585;
    const int panelW = 305;

    // 1. GroupBox Putar Foto (Orientasi)
    HWND hGrpRotate = CreateWindowExW(
        0, L"BUTTON", L" Putar Orientasi Foto ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        panelX, 48, panelW, 88,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpRotate, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnRotateCCW = CreateWindowExW(
        0, L"BUTTON", L"⟲ Kiri 90°",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 15, 70, 130, 30,
        m_hwnd, (HMENU)IDC_EDIT_BTN_ROTATE_CCW, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnRotateCCW, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnRotateCW = CreateWindowExW(
        0, L"BUTTON", L"⟳ Kanan 90°",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 155, 70, 130, 30,
        m_hwnd, (HMENU)IDC_EDIT_BTN_ROTATE_CW, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnRotateCW, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hStaticRotate = CreateWindowExW(
        0, L"STATIC", L"Sudut: 0° (Tegak / Normal)",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        panelX + 15, 106, 270, 18,
        m_hwnd, (HMENU)IDC_EDIT_STATIC_ROTATE, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticRotate, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    // 2. GroupBox Perbesaran (Zoom)
    HWND hGrpZoom = CreateWindowExW(
        0, L"BUTTON", L" Perbesaran (Zoom) ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        panelX, 142, panelW, 95,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpZoom, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hSliderZoom = CreateWindowExW(
        0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        panelX + 15, 164, 205, 30,
        m_hwnd, (HMENU)IDC_EDIT_SLIDER_ZOOM, m_hInstance, nullptr
    );
    SendMessageW(m_hSliderZoom, TBM_SETRANGE, TRUE, MAKELPARAM(100, 300));
    SendMessageW(m_hSliderZoom, TBM_SETTICFREQ, 50, 0);

    m_hStaticZoomVal = CreateWindowExW(
        0, L"STATIC", L"1.0x",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        panelX + 225, 168, 60, 24,
        m_hwnd, (HMENU)IDC_EDIT_STATIC_ZOOM_VAL, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticZoomVal, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnZoomReset = CreateWindowExW(
        0, L"BUTTON", L"🔍 1.0x (Reset Zoom)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 15, 200, 150, 26,
        m_hwnd, (HMENU)IDC_EDIT_BTN_ZOOM_RESET, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnZoomReset, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    HWND hZoomHint = CreateWindowExW(
        0, L"STATIC", L"(Bisa scroll mouse)",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        panelX + 175, 204, 115, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hZoomHint, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    // 3. GroupBox Ganti Background Pas Foto
    HWND hGrpBg = CreateWindowExW(
        0, L"BUTTON", L" Ganti Background Pas Foto ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        panelX, 243, panelW, 155,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpBg, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Baris 1 Tombol Preset BG (Asli, Merah, Biru)
    m_hBtnBgNone = CreateWindowExW(
        0, L"BUTTON", L"Latar Asli",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 15, 267, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_BG_NONE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnBgNone, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnBgRed = CreateWindowExW(
        0, L"BUTTON", L"🔴 Merah",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 107, 267, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_BG_RED, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnBgRed, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnBgBlue = CreateWindowExW(
        0, L"BUTTON", L"🔵 Biru",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 199, 267, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_BG_BLUE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnBgBlue, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Baris 2 Tombol Preset BG (Putih, Abu-abu, Kuning)
    m_hBtnBgWhite = CreateWindowExW(
        0, L"BUTTON", L"⚪ Putih",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 15, 300, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_BG_WHITE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnBgWhite, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnBgGrey = CreateWindowExW(
        0, L"BUTTON", L"🔘 Abu-abu",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 107, 300, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_BG_GREY, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnBgGrey, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnBgYellow = CreateWindowExW(
        0, L"BUTTON", L"🟡 Kuning",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 199, 300, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_BG_YELLOW, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnBgYellow, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Slider Toleransi Warna Background
    m_hSliderBgTol = CreateWindowExW(
        0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,
        panelX + 15, 338, 180, 30,
        m_hwnd, (HMENU)IDC_EDIT_SLIDER_BG_TOL, m_hInstance, nullptr
    );
    SendMessageW(m_hSliderBgTol, TBM_SETRANGE, TRUE, MAKELPARAM(10, 80));
    SendMessageW(m_hSliderBgTol, TBM_SETTICFREQ, 10, 0);

    m_hStaticBgTol = CreateWindowExW(
        0, L"STATIC", L"Toleransi: 30",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        panelX + 202, 342, 90, 22,
        m_hwnd, (HMENU)IDC_EDIT_STATIC_BG_TOL, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticBgTol, WM_SETFONT, (WPARAM)m_hFontSmall, TRUE);

    // 4. GroupBox Pratinjau Bingkai Ukuran & Warna
    HWND hGrpAspect = CreateWindowExW(
        0, L"BUTTON", L" Pratinjau Bingkai Ukuran & Efek ",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        panelX, 405, panelW, 135,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpAspect, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnAspect2x3 = CreateWindowExW(
        0, L"BUTTON", L"2x3",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 15, 428, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_ASPECT_2X3, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnAspect2x3, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnAspect3x4 = CreateWindowExW(
        0, L"BUTTON", L"3x4",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 107, 428, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_ASPECT_3X4, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnAspect3x4, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnAspect4x6 = CreateWindowExW(
        0, L"BUTTON", L"4x6",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 199, 428, 85, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_ASPECT_4X6, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnAspect4x6, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hChkBw = CreateWindowExW(
        0, L"BUTTON", L"◼ Pratinjau Hitam Putih (B&W)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        panelX + 15, 462, 270, 24,
        m_hwnd, (HMENU)IDC_EDIT_CHK_BW, m_hInstance, nullptr
    );
    SendMessageW(m_hChkBw, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnResetPos = CreateWindowExW(
        0, L"BUTTON", L"🎯 Reset Posisi Framing Tengah",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 15, 492, 270, 28,
        m_hwnd, (HMENU)IDC_EDIT_BTN_RESET_POS, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnResetPos, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // 5. Tombol Aksi Bawah (Simpan & Batal)
    m_hBtnSave = CreateWindowExW(
        0, L"BUTTON", L"💾 Simpan (Enter)",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        panelX + 15, 552, 160, 38,
        m_hwnd, (HMENU)IDC_EDIT_BTN_SAVE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnSave, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnCancel = CreateWindowExW(
        0, L"BUTTON", L"✖ Batal (Esc)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        panelX + 185, 552, 105, 38,
        m_hwnd, (HMENU)IDC_EDIT_BTN_CANCEL, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnCancel, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);
}

void EditPhotoDialog::UpdateControlsState() {
    wchar_t rotText[64];
    if (m_rotationAngle == 0) {
        swprintf_s(rotText, L"Sudut: 0° (Tegak / Normal)");
    } else {
        swprintf_s(rotText, L"Sudut: %d° Putar", m_rotationAngle);
    }
    SetWindowTextW(m_hStaticRotate, rotText);

    wchar_t zoomText[32];
    swprintf_s(zoomText, L"%.1fx", m_zoomLevel);
    SetWindowTextW(m_hStaticZoomVal, zoomText);

    int trackPos = static_cast<int>(m_zoomLevel * 100.0 + 0.5);
    SendMessageW(m_hSliderZoom, TBM_SETPOS, TRUE, trackPos);

    if (m_hSliderBgTol) {
        SendMessageW(m_hSliderBgTol, TBM_SETPOS, TRUE, m_bgTolerance);
    }

    if (m_hStaticBgTol) {
        wchar_t tolText[64];
        swprintf_s(tolText, L"Toleransi: %d", m_bgTolerance);
        SetWindowTextW(m_hStaticBgTol, tolText);
    }

    if (m_hChkBw) {
        SendMessageW(m_hChkBw, BM_SETCHECK, m_isBlackAndWhite ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

void EditPhotoDialog::DrawCanvas(HDC hdc) {
    int w = m_canvasRect.right - m_canvasRect.left;
    int h = m_canvasRect.bottom - m_canvasRect.top;

    if (w <= 0 || h <= 0) return;

    // Double buffering
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

    {
        Gdiplus::Graphics graphics(memDC);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

        // 1. Background studio gelap
        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(38, 40, 44));
        graphics.FillRectangle(&bgBrush, 0, 0, w, h);

        Gdiplus::Bitmap* bmp = m_imageProcessor.GetOrProcessBgBitmap(
            m_item.sourceFilePath,
            m_bgPreset,
            m_customBgColor,
            m_bgTolerance
        );

        if (bmp != nullptr && bmp->GetWidth() > 0 && bmp->GetHeight() > 0) {
            double origW = bmp->GetWidth();
            double origH = bmp->GetHeight();

            int rot = (m_rotationAngle % 360 + 360) % 360;
            m_effImgW = (rot == 90 || rot == 270) ? origH : origW;
            m_effImgH = (rot == 90 || rot == 270) ? origW : origH;

            // Fit visual image ke dalam area canvas dengan padding
            const double pad = 24.0;
            double availW = w - (2.0 * pad);
            double availH = h - (2.0 * pad) - 24.0; // Sisakan ruang bawah untuk text status

            m_fitScale = std::min(availW / m_effImgW, availH / m_effImgH);
            double dispW = m_effImgW * m_fitScale;
            double dispH = m_effImgH * m_fitScale;
            m_imgDispX = (w - dispW) / 2.0;
            m_imgDispY = pad + (availH - dispH) / 2.0;

            // Gambar full image dengan rotasi
            Gdiplus::PointF destPts[3];
            if (rot == 0) {
                destPts[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX), static_cast<Gdiplus::REAL>(m_imgDispY));
                destPts[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX + dispW), static_cast<Gdiplus::REAL>(m_imgDispY));
                destPts[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX), static_cast<Gdiplus::REAL>(m_imgDispY + dispH));
            } else if (rot == 90) {
                destPts[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX + dispW), static_cast<Gdiplus::REAL>(m_imgDispY));
                destPts[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX + dispW), static_cast<Gdiplus::REAL>(m_imgDispY + dispH));
                destPts[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX), static_cast<Gdiplus::REAL>(m_imgDispY));
            } else if (rot == 180) {
                destPts[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX + dispW), static_cast<Gdiplus::REAL>(m_imgDispY + dispH));
                destPts[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX), static_cast<Gdiplus::REAL>(m_imgDispY + dispH));
                destPts[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX + dispW), static_cast<Gdiplus::REAL>(m_imgDispY));
            } else if (rot == 270) {
                destPts[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX), static_cast<Gdiplus::REAL>(m_imgDispY + dispH));
                destPts[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX), static_cast<Gdiplus::REAL>(m_imgDispY));
                destPts[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(m_imgDispX + dispW), static_cast<Gdiplus::REAL>(m_imgDispY + dispH));
            }

            Gdiplus::ImageAttributes imgAttr;
            if (m_isBlackAndWhite) {
                Gdiplus::ColorMatrix grayscaleMatrix = {
                    0.299f, 0.299f, 0.299f, 0.0f, 0.0f,
                    0.587f, 0.587f, 0.587f, 0.0f, 0.0f,
                    0.114f, 0.114f, 0.114f, 0.0f, 0.0f,
                    0.0f,   0.0f,   0.0f,   1.0f, 0.0f,
                    0.0f,   0.0f,   0.0f,   0.0f, 1.0f
                };
                imgAttr.SetColorMatrix(&grayscaleMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
            }

            graphics.DrawImage(
                bmp,
                destPts,
                3,
                0, 0,
                static_cast<Gdiplus::REAL>(origW),
                static_cast<Gdiplus::REAL>(origH),
                Gdiplus::UnitPixel,
                m_isBlackAndWhite ? &imgAttr : nullptr
            );

            // Hitung crop box visual berdasarkan preset aspect ratio yang aktif
            MillimeterSize presetMm = GetPresetDimensionMm(m_previewPreset);
            double targetAspect = presetMm.widthMm / presetMm.heightMm;
            double srcAspect = m_effImgW / m_effImgH;

            double cropW, cropH;
            if (srcAspect > targetAspect) {
                cropH = m_effImgH;
                cropW = cropH * targetAspect;
            } else {
                cropW = m_effImgW;
                cropH = cropW / targetAspect;
            }

            double zoom = (m_zoomLevel > 0.1) ? m_zoomLevel : 1.0;
            cropW /= zoom;
            cropH /= zoom;

            if (cropW > m_effImgW) cropW = m_effImgW;
            if (cropH > m_effImgH) cropH = m_effImgH;

            double centerX = m_effImgW * m_cropCenterX;
            double centerY = m_effImgH * m_cropCenterY;

            double effCropX = centerX - (cropW / 2.0);
            double effCropY = centerY - (cropH / 2.0);

            if (effCropX < 0) effCropX = 0;
            if (effCropY < 0) effCropY = 0;
            if (effCropX + cropW > m_effImgW) effCropX = m_effImgW - cropW;
            if (effCropY + cropH > m_effImgH) effCropY = m_effImgH - cropH;

            // Posisi crop box di layar canvas
            double cropBoxX = m_imgDispX + (effCropX * m_fitScale);
            double cropBoxY = m_imgDispY + (effCropY * m_fitScale);
            double cropBoxW = cropW * m_fitScale;
            double cropBoxH = cropH * m_fitScale;

            // 2. Dark Mask di luar crop box
            Gdiplus::Region maskRegion(Gdiplus::RectF(
                static_cast<Gdiplus::REAL>(m_imgDispX),
                static_cast<Gdiplus::REAL>(m_imgDispY),
                static_cast<Gdiplus::REAL>(dispW),
                static_cast<Gdiplus::REAL>(dispH)
            ));
            maskRegion.Exclude(Gdiplus::RectF(
                static_cast<Gdiplus::REAL>(cropBoxX),
                static_cast<Gdiplus::REAL>(cropBoxY),
                static_cast<Gdiplus::REAL>(cropBoxW),
                static_cast<Gdiplus::REAL>(cropBoxH)
            ));
            Gdiplus::SolidBrush dimBrush(Gdiplus::Color(170, 0, 0, 0));
            graphics.FillRegion(&dimBrush, &maskRegion);

            // 3. Border garis crop aktif (Warna Kuning Emas Cerah)
            Gdiplus::Pen cropPen(Gdiplus::Color(255, 225, 40), 2.0f);
            graphics.DrawRectangle(
                &cropPen,
                static_cast<Gdiplus::REAL>(cropBoxX),
                static_cast<Gdiplus::REAL>(cropBoxY),
                static_cast<Gdiplus::REAL>(cropBoxW),
                static_cast<Gdiplus::REAL>(cropBoxH)
            );

            // 4. Guideline Pas Foto (Oval Kepala & Garis Mata)
            Gdiplus::Pen guidePen(Gdiplus::Color(180, 255, 255, 255), 1.0f);
            guidePen.SetDashStyle(Gdiplus::DashStyleDash);

            // Oval kepala (bias atas)
            Gdiplus::REAL headX = static_cast<Gdiplus::REAL>(cropBoxX + cropBoxW * 0.22);
            Gdiplus::REAL headY = static_cast<Gdiplus::REAL>(cropBoxY + cropBoxH * 0.10);
            Gdiplus::REAL headW = static_cast<Gdiplus::REAL>(cropBoxW * 0.56);
            Gdiplus::REAL headH = static_cast<Gdiplus::REAL>(cropBoxH * 0.52);
            graphics.DrawEllipse(&guidePen, headX, headY, headW, headH);

            // Garis horizontal level mata (sekitar 38% dari atas foto)
            Gdiplus::REAL eyeY = static_cast<Gdiplus::REAL>(cropBoxY + cropBoxH * 0.38);
            graphics.DrawLine(
                &guidePen,
                static_cast<Gdiplus::REAL>(cropBoxX + cropBoxW * 0.20), eyeY,
                static_cast<Gdiplus::REAL>(cropBoxX + cropBoxW * 0.80), eyeY
            );

            // 5. Info Bar Bawah Canvas
            wchar_t statusBuf[256];
            swprintf_s(statusBuf, L"Bingkai: %s | Zoom: %.1fx | Sudut: %d° | BG: %s | Drag mouse untuk geser",
                GetPresetName(m_previewPreset), m_zoomLevel, m_rotationAngle, GetBgPresetName(m_bgPreset));

            Gdiplus::Font statusFont(L"Segoe UI", 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush statusBrush(Gdiplus::Color(210, 220, 235));
            Gdiplus::PointF statusPt(10.0f, static_cast<float>(h - 24));
            graphics.DrawString(statusBuf, -1, &statusFont, statusPt, &statusBrush);
        } else {
            // Gambar tidak ditemukan / error
            Gdiplus::Font errFont(L"Segoe UI", 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush errBrush(Gdiplus::Color(200, 100, 100));
            Gdiplus::PointF errPt(static_cast<float>(w / 2 - 80), static_cast<float>(h / 2 - 10));
            graphics.DrawString(L"Gagal memuat gambar", -1, &errFont, errPt, &errBrush);
        }

        // Canvas Outer Border
        Gdiplus::Pen borderPen(Gdiplus::Color(60, 64, 72), 1.0f);
        graphics.DrawRectangle(&borderPen, 0, 0, w - 1, h - 1);
    }

    // Blit ke screen DC
    BitBlt(hdc, m_canvasRect.left, m_canvasRect.top, w, h, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBmp);
    DeleteDC(memDC);
}

LRESULT EditPhotoDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            DrawCanvas(hdc);
            EndPaint(m_hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_SETCURSOR: {
            HWND hWndCursor = (HWND)wParam;
            if (hWndCursor == m_hwnd) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(m_hwnd, &pt);
                if (PtInRect(&m_canvasRect, pt)) {
                    SetCursor(LoadCursor(nullptr, m_isDragging ? IDC_SIZEALL : IDC_HAND));
                    return TRUE;
                }
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            int mouseX = GET_X_LPARAM(lParam);
            int mouseY = GET_Y_LPARAM(lParam);
            POINT pt = { mouseX, mouseY };

            if (PtInRect(&m_canvasRect, pt) && m_effImgW > 0 && m_effImgH > 0 && m_fitScale > 0) {
                m_isDragging = true;
                m_lastMousePos = pt;
                SetCapture(m_hwnd);
                SetCursor(LoadCursor(nullptr, IDC_SIZEALL));
                return 0;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            if (m_isDragging && (wParam & MK_LBUTTON)) {
                int mouseX = GET_X_LPARAM(lParam);
                int mouseY = GET_Y_LPARAM(lParam);

                int deltaX = mouseX - m_lastMousePos.x;
                int deltaY = mouseY - m_lastMousePos.y;

                m_lastMousePos.x = mouseX;
                m_lastMousePos.y = mouseY;

                // Geser crop center searah drag mouse
                double deltaNormX = (deltaX / m_fitScale) / m_effImgW;
                double deltaNormY = (deltaY / m_fitScale) / m_effImgH;

                m_cropCenterX += deltaNormX;
                m_cropCenterY += deltaNormY;

                if (m_cropCenterX < 0.0) m_cropCenterX = 0.0;
                if (m_cropCenterX > 1.0) m_cropCenterX = 1.0;
                if (m_cropCenterY < 0.0) m_cropCenterY = 0.0;
                if (m_cropCenterY > 1.0) m_cropCenterY = 1.0;

                InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                return 0;
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (m_isDragging) {
                m_isDragging = false;
                ReleaseCapture();
                InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                return 0;
            }
            break;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0) {
                m_zoomLevel += 0.1;
            } else if (delta < 0) {
                m_zoomLevel -= 0.1;
            }

            if (m_zoomLevel < 1.0) m_zoomLevel = 1.0;
            if (m_zoomLevel > 3.0) m_zoomLevel = 3.0;

            UpdateControlsState();
            InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
            return 0;
        }

        case WM_HSCROLL: {
            if ((HWND)lParam == m_hSliderZoom) {
                int pos = (int)SendMessageW(m_hSliderZoom, TBM_GETPOS, 0, 0);
                m_zoomLevel = pos / 100.0;
                if (m_zoomLevel < 1.0) m_zoomLevel = 1.0;
                if (m_zoomLevel > 3.0) m_zoomLevel = 3.0;

                wchar_t zoomText[32];
                swprintf_s(zoomText, L"%.1fx", m_zoomLevel);
                SetWindowTextW(m_hStaticZoomVal, zoomText);

                InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                return 0;
            } else if ((HWND)lParam == m_hSliderBgTol) {
                int tol = (int)SendMessageW(m_hSliderBgTol, TBM_GETPOS, 0, 0);
                m_bgTolerance = tol;

                wchar_t tolText[64];
                swprintf_s(tolText, L"Toleransi: %d", m_bgTolerance);
                SetWindowTextW(m_hStaticBgTol, tolText);

                InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                return 0;
            }
            break;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case IDC_EDIT_BTN_ROTATE_CCW:
                    m_rotationAngle = (m_rotationAngle - 90 + 360) % 360;
                    UpdateControlsState();
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_ROTATE_CW:
                    m_rotationAngle = (m_rotationAngle + 90) % 360;
                    UpdateControlsState();
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_ZOOM_RESET:
                    m_zoomLevel = 1.0;
                    UpdateControlsState();
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_BG_NONE:
                    m_bgPreset = BgColorPreset::None;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_BG_RED:
                    m_bgPreset = BgColorPreset::Red;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_BG_BLUE:
                    m_bgPreset = BgColorPreset::Blue;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_BG_WHITE:
                    m_bgPreset = BgColorPreset::White;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_BG_GREY:
                    m_bgPreset = BgColorPreset::Grey;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_BG_YELLOW:
                    m_bgPreset = BgColorPreset::Yellow;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_ASPECT_2X3:
                    m_previewPreset = PhotoSizePreset::Size2x3;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_ASPECT_3X4:
                    m_previewPreset = PhotoSizePreset::Size3x4;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_ASPECT_4X6:
                    m_previewPreset = PhotoSizePreset::Size4x6;
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_CHK_BW: {
                    LRESULT check = SendMessageW(m_hChkBw, BM_GETCHECK, 0, 0);
                    m_isBlackAndWhite = (check == BST_CHECKED);
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;
                }

                case IDC_EDIT_BTN_RESET_POS:
                    m_cropCenterX = 0.5;
                    m_cropCenterY = 0.45;
                    m_zoomLevel = 1.0;
                    UpdateControlsState();
                    InvalidateRect(m_hwnd, &m_canvasRect, FALSE);
                    return 0;

                case IDC_EDIT_BTN_SAVE:
                    m_item.cropCenterX = m_cropCenterX;
                    m_item.cropCenterY = m_cropCenterY;
                    m_item.zoomLevel = m_zoomLevel;
                    m_item.rotationAngle = m_rotationAngle;
                    m_item.isBlackAndWhite = m_isBlackAndWhite;
                    m_item.bgPreset = m_bgPreset;
                    m_item.customBgColor = m_customBgColor;
                    m_item.bgTolerance = m_bgTolerance;
                    m_saved = true;
                    DestroyWindow(m_hwnd);
                    return 0;

                case IDC_EDIT_BTN_CANCEL:
                    DestroyWindow(m_hwnd);
                    return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(m_hwnd);
            return 0;

        case WM_DESTROY:
            m_hwnd = nullptr;
            return 0;
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

} // namespace SoftwareCenter208
