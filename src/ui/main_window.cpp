#include "main_window.h"
#include "preview_dialog.h"
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <algorithm>

namespace SoftwareCenter208 {

enum ControlIds {
    IDC_LIST_ORDERS = 101,
    IDC_BTN_ADD,
    IDC_BTN_REMOVE,
    IDC_BTN_CLEAR,

    // Per-preset quantities (- [ 0 ] +)
    IDC_BTN_MINUS_2X3,
    IDC_EDIT_QTY_2X3,
    IDC_BTN_PLUS_2X3,

    IDC_BTN_MINUS_3X4,
    IDC_EDIT_QTY_3X4,
    IDC_BTN_PLUS_3X4,

    IDC_BTN_MINUS_4X6,
    IDC_EDIT_QTY_4X6,
    IDC_BTN_PLUS_4X6,

    // Packing mode
    IDC_RADIO_SMARTSTRIP,
    IDC_RADIO_EASYCUT,
    IDC_RADIO_MAXDENSITY,
    IDC_CHK_CUTLINES,

    // Output & stats
    IDC_BTN_PREVIEW,
    IDC_BTN_PRINT,
    IDC_BTN_EXPORT,
    IDC_STATIC_INFO,
    IDC_STATIC_SCRAP,

    // Page navigation
    IDC_BTN_PREV_PAGE,
    IDC_BTN_NEXT_PAGE,
    IDC_STATIC_PAGE_NUM
};

MainWindow::MainWindow() {
}

MainWindow::~MainWindow() {
    if (m_hAccel) DestroyAcceleratorTable(m_hAccel);
    if (m_hFontUI) DeleteObject(m_hFontUI);
    if (m_hFontHeader) DeleteObject(m_hFontHeader);
    if (m_hFontBold) DeleteObject(m_hFontBold);
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    // Inisialisasi Common Controls (XP through 11 support)
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_STANDARD_CLASSES | ICC_UPDOWN_CLASS };
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"SoftwareCenter208_MainWindowClass";

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.style = CS_HREDRAW | CS_VREDRAW;

    RegisterClassExW(&wc);

    m_hwnd = CreateWindowExW(
        WS_EX_ACCEPTFILES, // Support Drag and Drop
        CLASS_NAME,
        L"208 Software Center - Pas Foto A4 Layout & Print",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1060, 730,
        nullptr, nullptr, hInstance, this
    );

    if (!m_hwnd) return false;

    // Keyboard Accelerators (Ctrl+P -> Print Preview)
    ACCEL accels[1] = {
        { FCONTROL | FVIRTKEY, 'P', IDC_BTN_PREVIEW }
    };
    m_hAccel = CreateAcceleratorTableW(accels, 1);

    InitControls();
    UpdateLayoutCalculation();

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    return true;
}

int MainWindow::RunMessageLoop() {
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (!m_hAccel || !TranslateAcceleratorW(m_hwnd, m_hAccel, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCTW* pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(pCreate->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
        pThis->m_hwnd = hwnd;
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->HandleMessage(uMsg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void MainWindow::InitControls() {
    // Setup keyboard accelerators (Ctrl+P, Ctrl+S, Ctrl+O, Delete)
    ACCEL accels[] = {
        { FCONTROL | FVIRTKEY, 'P', IDC_BTN_PRINT },
        { FCONTROL | FVIRTKEY, 'S', IDC_BTN_EXPORT },
        { FCONTROL | FVIRTKEY, 'O', IDC_BTN_ADD },
        { FVIRTKEY, VK_DELETE, IDC_BTN_REMOVE }
    };
    m_hAccel = CreateAcceleratorTableW(accels, sizeof(accels) / sizeof(ACCEL));

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
    m_hFontHeader = CreateFontW(
        -14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    // Header Label Antrian
    HWND hLblQueue = CreateWindowExW(
        0, L"STATIC", L"📂 Daftar Foto Antrian (Drag & Drop ke sini)",
        WS_CHILD | WS_VISIBLE,
        22, 16, 326, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblQueue, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    // ListBox Antrian
    m_hListOrders = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        22, 40, 326, 175,
        m_hwnd, (HMENU)IDC_LIST_ORDERS, m_hInstance, nullptr
    );
    SendMessageW(m_hListOrders, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Tombol Antrian
    m_hBtnAdd = CreateWindowExW(
        0, L"BUTTON", L"+ Tambah",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 222, 104, 28,
        m_hwnd, (HMENU)IDC_BTN_ADD, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnAdd, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnRemove = CreateWindowExW(
        0, L"BUTTON", L"- Hapus",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        133, 222, 104, 28,
        m_hwnd, (HMENU)IDC_BTN_REMOVE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnRemove, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnClear = CreateWindowExW(
        0, L"BUTTON", L"🗑️ Kosongkan",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        244, 222, 104, 28,
        m_hwnd, (HMENU)IDC_BTN_CLEAR, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnClear, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Group Box 1: Jumlah Pas Foto per Ukuran
    HWND hGrpSize = CreateWindowExW(
        0, L"BUTTON", L"Jumlah Pas Foto per Ukuran (Foto Terpilih)",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        22, 258, 326, 120,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpSize, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // 2x3 Row
    HWND hLbl2x3 = CreateWindowExW(
        0, L"STATIC", L"Ukuran 2x3:",
        WS_CHILD | WS_VISIBLE,
        32, 280, 135, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLbl2x3, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnMinus2x3 = CreateWindowExW(
        0, L"BUTTON", L"-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        172, 278, 26, 24,
        m_hwnd, (HMENU)IDC_BTN_MINUS_2X3, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnMinus2x3, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hEditQty2x3 = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER,
        202, 278, 38, 24,
        m_hwnd, (HMENU)IDC_EDIT_QTY_2X3, m_hInstance, nullptr
    );
    SendMessageW(m_hEditQty2x3, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnPlus2x3 = CreateWindowExW(
        0, L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        244, 278, 26, 24,
        m_hwnd, (HMENU)IDC_BTN_PLUS_2X3, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPlus2x3, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND hLblPcs2x3 = CreateWindowExW(
        0, L"STATIC", L"lembar",
        WS_CHILD | WS_VISIBLE,
        276, 280, 55, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblPcs2x3, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // 3x4 Row
    HWND hLbl3x4 = CreateWindowExW(
        0, L"STATIC", L"Ukuran 3x4:",
        WS_CHILD | WS_VISIBLE,
        32, 308, 135, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLbl3x4, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnMinus3x4 = CreateWindowExW(
        0, L"BUTTON", L"-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        172, 306, 26, 24,
        m_hwnd, (HMENU)IDC_BTN_MINUS_3X4, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnMinus3x4, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hEditQty3x4 = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER,
        202, 306, 38, 24,
        m_hwnd, (HMENU)IDC_EDIT_QTY_3X4, m_hInstance, nullptr
    );
    SendMessageW(m_hEditQty3x4, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnPlus3x4 = CreateWindowExW(
        0, L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        244, 306, 26, 24,
        m_hwnd, (HMENU)IDC_BTN_PLUS_3X4, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPlus3x4, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND hLblPcs3x4 = CreateWindowExW(
        0, L"STATIC", L"lembar",
        WS_CHILD | WS_VISIBLE,
        276, 308, 55, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblPcs3x4, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // 4x6 Row
    HWND hLbl4x6 = CreateWindowExW(
        0, L"STATIC", L"Ukuran 4x6:",
        WS_CHILD | WS_VISIBLE,
        32, 336, 135, 22,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLbl4x6, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnMinus4x6 = CreateWindowExW(
        0, L"BUTTON", L"-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        172, 334, 26, 24,
        m_hwnd, (HMENU)IDC_BTN_MINUS_4X6, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnMinus4x6, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hEditQty4x6 = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER,
        202, 334, 38, 24,
        m_hwnd, (HMENU)IDC_EDIT_QTY_4X6, m_hInstance, nullptr
    );
    SendMessageW(m_hEditQty4x6, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnPlus4x6 = CreateWindowExW(
        0, L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        244, 334, 26, 24,
        m_hwnd, (HMENU)IDC_BTN_PLUS_4X6, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPlus4x6, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND hLblPcs4x6 = CreateWindowExW(
        0, L"STATIC", L"lembar",
        WS_CHILD | WS_VISIBLE,
        276, 336, 55, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblPcs4x6, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Group Box 2: Mode Penataan & Pemotongan
    HWND hGrpMode = CreateWindowExW(
        0, L"BUTTON", L"Mode Penataan Kertas A4",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        22, 386, 326, 120,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpMode, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Radio SmartStrip (Default)
    m_hRadioSmartStrip = CreateWindowExW(
        0, L"BUTTON", L"⚡ Smart Strip (Gunting Mudah & Hemat)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
        34, 406, 300, 20,
        m_hwnd, (HMENU)IDC_RADIO_SMARTSTRIP, m_hInstance, nullptr
    );
    SendMessageW(m_hRadioSmartStrip, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);
    SendMessageW(m_hRadioSmartStrip, BM_SETCHECK, BST_CHECKED, 0);

    // Radio EasyCut
    m_hRadioEasyCut = CreateWindowExW(
        0, L"BUTTON", L"✂️ Baris Murni (1 Baris 1 Ukuran)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        34, 428, 300, 20,
        m_hwnd, (HMENU)IDC_RADIO_EASYCUT, m_hInstance, nullptr
    );
    SendMessageW(m_hRadioEasyCut, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Radio MaxDensity
    m_hRadioMaxDensity = CreateWindowExW(
        0, L"BUTTON", L"📐 Hemat Maksimal (Isi Celah Kosong)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        34, 450, 300, 20,
        m_hwnd, (HMENU)IDC_RADIO_MAXDENSITY, m_hInstance, nullptr
    );
    SendMessageW(m_hRadioMaxDensity, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Checkbox Cut Lines
    m_hChkCutLines = CreateWindowExW(
        0, L"BUTTON", L"Garis Batas Potong (Cut Guide Lines)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        34, 474, 300, 20,
        m_hwnd, (HMENU)IDC_CHK_CUTLINES, m_hInstance, nullptr
    );
    SendMessageW(m_hChkCutLines, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);
    SendMessageW(m_hChkCutLines, BM_SETCHECK, BST_CHECKED, 0);

    // Status Efisiensi Kertas Sisa
    m_hStaticInfo = CreateWindowExW(
        0, L"STATIC", L"Total Foto: 0 lembar",
        WS_CHILD | WS_VISIBLE,
        22, 506, 326, 20,
        m_hwnd, (HMENU)IDC_STATIC_INFO, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticInfo, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hStaticScrap = CreateWindowExW(
        0, L"STATIC", L"Sisa Kertas A4: 29.7 cm (Utuh)",
        WS_CHILD | WS_VISIBLE,
        22, 528, 326, 24,
        m_hwnd, (HMENU)IDC_STATIC_SCRAP, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticScrap, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Tombol Pratinjau, Cetak & Export (Bottom Actions)
    m_hBtnPreview = CreateWindowExW(
        0, L"BUTTON", L"🔍 Pratinjau Cetak (Print Preview)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 558, 326, 36,
        m_hwnd, (HMENU)IDC_BTN_PREVIEW, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPreview, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    m_hBtnPrint = CreateWindowExW(
        0, L"BUTTON", L"🖨️ Cetak Langsung (Ctrl+P)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 598, 326, 36,
        m_hwnd, (HMENU)IDC_BTN_PRINT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPrint, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnExport = CreateWindowExW(
        0, L"BUTTON", L"💾 Export (PDF / PNG / JPG)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 638, 326, 32,
        m_hwnd, (HMENU)IDC_BTN_EXPORT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnExport, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Page navigation bar (di atas canvas preview)
    m_hBtnPrevPage = CreateWindowExW(
        0, L"BUTTON", L"◀ Hal Sebelumnya",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        380, 10, 140, 28,
        m_hwnd, (HMENU)IDC_BTN_PREV_PAGE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPrevPage, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hStaticPageNum = CreateWindowExW(
        0, L"STATIC", L"Halaman 1 / 1",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        530, 15, 140, 20,
        m_hwnd, (HMENU)IDC_STATIC_PAGE_NUM, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticPageNum, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    m_hBtnNextPage = CreateWindowExW(
        0, L"BUTTON", L"Hal Berikutnya ▶",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        680, 10, 140, 28,
        m_hwnd, (HMENU)IDC_BTN_NEXT_PAGE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnNextPage, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
}

void MainWindow::ResizeLayout(int clientWidth, int clientHeight) {
    if (m_hwnd == nullptr) return;

    int previewAreaX = 375;
    int previewAreaW = clientWidth - previewAreaX - 10;
    if (previewAreaW > 0 && m_hBtnPrevPage && m_hStaticPageNum && m_hBtnNextPage) {
        int navBtnW = 140;
        int navLabelW = 140;
        int navSpacing = 10;
        int navTotalW = navBtnW + navSpacing + navLabelW + navSpacing + navBtnW; // 440
        int startX = previewAreaX + (previewAreaW - navTotalW) / 2;
        if (startX < previewAreaX) startX = previewAreaX;

        SetWindowPos(m_hBtnPrevPage, nullptr, startX, 10, navBtnW, 28, SWP_NOZORDER | SWP_NOCOPYBITS);
        SetWindowPos(m_hStaticPageNum, nullptr, startX + navBtnW + navSpacing, 15, navLabelW, 20, SWP_NOZORDER | SWP_NOCOPYBITS);
        SetWindowPos(m_hBtnNextPage, nullptr, startX + navBtnW + navSpacing + navLabelW + navSpacing, 10, navBtnW, 28, SWP_NOZORDER | SWP_NOCOPYBITS);
    }

    InvalidateRect(m_hwnd, nullptr, TRUE);
}

void MainWindow::UpdateLayoutCalculation() {
    m_currentLayout = PackingEngine::CalculateLayout(m_orderItems, m_paperConfig);

    if (m_currentPageIndex >= m_currentLayout.totalPages) {
        m_currentPageIndex = (m_currentLayout.totalPages > 0) ? (m_currentLayout.totalPages - 1) : 0;
    }
    if (m_currentPageIndex < 0) {
        m_currentPageIndex = 0;
    }

    wchar_t infoText[256];
    if (m_currentLayout.totalPages <= 1) {
        swprintf_s(infoText, L"Total Foto: %d lembar (Muat 1 Lembar A4)",
            m_currentLayout.totalPhotosPlaced);
    } else {
        swprintf_s(infoText, L"Total Foto: %d lembar (%d Lembar A4)",
            m_currentLayout.totalPhotosPlaced, m_currentLayout.totalPages);
    }
    SetWindowTextW(m_hStaticInfo, infoText);

    double remMm = (m_currentPageIndex < (int)m_currentLayout.pages.size())
        ? m_currentLayout.pages[m_currentPageIndex].remainingHeightMm
        : m_paperConfig.heightMm;

    wchar_t scrapText[256];
    if (remMm >= 2.0) {
        swprintf_s(scrapText, L"Sisa Kertas Hal %d: %.1f cm\n(Bisa dipotong untuk kertas sisa)",
            m_currentPageIndex + 1, remMm / 10.0);
    } else {
        swprintf_s(scrapText, L"Sisa Kertas Hal %d: Terpakai Penuh", m_currentPageIndex + 1);
    }
    SetWindowTextW(m_hStaticScrap, scrapText);

    if (m_hStaticPageNum) {
        wchar_t pageText[64];
        swprintf_s(pageText, L"Halaman %d / %d",
            m_currentPageIndex + 1,
            (m_currentLayout.totalPages > 0 ? m_currentLayout.totalPages : 1));
        SetWindowTextW(m_hStaticPageNum, pageText);
    }

    if (m_hBtnPrevPage) {
        EnableWindow(m_hBtnPrevPage, m_currentPageIndex > 0);
    }
    if (m_hBtnNextPage) {
        EnableWindow(m_hBtnNextPage, m_currentPageIndex < m_currentLayout.totalPages - 1);
    }

    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::RefreshOrderListUI() {
    SendMessageW(m_hListOrders, LB_RESETCONTENT, 0, 0);

    for (size_t i = 0; i < m_orderItems.size(); ++i) {
        const auto& item = m_orderItems[i];
        std::wstring fileName = item.sourceFilePath;
        size_t lastSlash = fileName.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            fileName = fileName.substr(lastSlash + 1);
        }

        std::wstring qtySummary;
        if (item.qty2x3 > 0) qtySummary += L"2x3:" + std::to_wstring(item.qty2x3) + L" ";
        if (item.qty3x4 > 0) qtySummary += L"3x4:" + std::to_wstring(item.qty3x4) + L" ";
        if (item.qty4x6 > 0) qtySummary += L"4x6:" + std::to_wstring(item.qty4x6) + L" ";
        if (qtySummary.empty()) qtySummary = L"0 pcs";
        else qtySummary += L"(" + std::to_wstring(item.GetTotalQuantity()) + L" pcs)";

        wchar_t itemText[256];
        swprintf_s(itemText, L"[%d] %s - %s",
            static_cast<int>(i + 1),
            fileName.c_str(),
            qtySummary.c_str()
        );

        SendMessageW(m_hListOrders, LB_ADDSTRING, 0, (LPARAM)itemText);
    }

    UpdateLayoutCalculation();
}

void MainWindow::DrawPreviewCanvas(HDC hdc, const RECT& previewRect) {
    int availW = previewRect.right - previewRect.left;
    int availH = previewRect.bottom - previewRect.top;

    if (availW <= 50 || availH <= 50) return;

    // Hitung bounding box kertas A4 dengan mempertahankan aspect ratio 210 x 297
    double paperAspect = m_paperConfig.widthMm / m_paperConfig.heightMm; // ~0.707
    double availAspect = static_cast<double>(availW) / static_cast<double>(availH);

    int canvasW, canvasH;
    if (availAspect > paperAspect) {
        // Window lebih lebar: fit height
        canvasH = availH - 20;
        canvasW = static_cast<int>(canvasH * paperAspect);
    } else {
        // Window lebih sempit: fit width
        canvasW = availW - 20;
        canvasH = static_cast<int>(canvasW / paperAspect);
    }

    int canvasX = previewRect.left + (availW - canvasW) / 2;
    int canvasY = previewRect.top + (availH - canvasH) / 2;

    // Double buffer menggunakan GDI+ Bitmap
    Gdiplus::Bitmap memBmp(availW, availH, PixelFormat32bppARGB);
    {
        Gdiplus::Graphics g(&memBmp);
        g.Clear(Gdiplus::Color(230, 232, 238)); // Background canvas workspace

        // Bayangan lembut (Drop shadow) kertas A4
        Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(180, 185, 195));
        g.FillRectangle(&shadowBrush, canvasX - previewRect.left + 5, canvasY - previewRect.top + 5, canvasW, canvasH);

        // Render Lembar Kertas A4
        double previewDpi = 96.0;
        double targetPaperPxW = MmToPixels(m_paperConfig.widthMm, previewDpi);
        double scale = static_cast<double>(canvasW) / targetPaperPxW;

        Gdiplus::GraphicsState gState = g.Save();
        g.TranslateTransform(static_cast<Gdiplus::REAL>(canvasX - previewRect.left), static_cast<Gdiplus::REAL>(canvasY - previewRect.top));

        // Render sheet ke dalam viewport preview
        m_imageProcessor.RenderSheet(g, m_currentLayout, m_paperConfig, m_currentPageIndex, previewDpi, true, scale);

        // Border luar kertas A4
        Gdiplus::Pen borderPen(Gdiplus::Color(170, 175, 185), 1.0f);
        g.DrawRectangle(&borderPen, 0, 0, canvasW, canvasH);

        g.Restore(gState);
    }

    // Blit hasil render memBmp ke screen HDC
    Gdiplus::Graphics screenGraphics(hdc);
    screenGraphics.DrawImage(&memBmp, static_cast<INT>(previewRect.left), static_cast<INT>(previewRect.top));
}

void MainWindow::OnAddPhotoFiles() {
    WCHAR szFiles[8192] = { 0 };

    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"File Gambar (JPG, PNG, BMP, TIFF)\0*.jpg;*.jpeg;*.png;*.bmp;*.tif;*.tiff\0Semua File (*.*)\0*.*\0";
    ofn.lpstrFile = szFiles;
    ofn.nMaxFile = sizeof(szFiles) / sizeof(WCHAR);
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY;

    if (GetOpenFileNameW(&ofn)) {
        WCHAR* p = szFiles;
        std::wstring dir = p;
        p += dir.length() + 1;

        if (*p == 0) {
            // Single file selection
            OnAddSingleFilePath(dir);
        } else {
            // Multi file selection
            while (*p) {
                std::wstring fullPath = dir + L"\\" + p;
                OnAddSingleFilePath(fullPath);
                p += wcslen(p) + 1;
            }
        }
        RefreshOrderListUI();
        if (!m_orderItems.empty()) {
            int newSel = (int)m_orderItems.size() - 1;
            SendMessageW(m_hListOrders, LB_SETCURSEL, newSel, 0);
            const auto& item = m_orderItems[newSel];
            m_isUpdatingUI = true;
            SetWindowTextW(m_hEditQty2x3, std::to_wstring(item.qty2x3).c_str());
            SetWindowTextW(m_hEditQty3x4, std::to_wstring(item.qty3x4).c_str());
            SetWindowTextW(m_hEditQty4x6, std::to_wstring(item.qty4x6).c_str());
            m_isUpdatingUI = false;
        }
    }
}

void MainWindow::OnAddSingleFilePath(const std::wstring& path) {
    std::wstring ext = PathFindExtensionW(path.c_str());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    if (ext != L".jpg" && ext != L".jpeg" && ext != L".png" && ext != L".bmp" && ext != L".tif" && ext != L".tiff") {
        return;
    }

    PhotoOrderItem item;
    item.id = m_nextItemId++;
    item.sourceFilePath = path;
    item.qty2x3 = 0;
    item.qty3x4 = 0; // Default 0 lembar
    item.qty4x6 = 0;

    m_orderItems.push_back(item);
}

void MainWindow::OnRemoveSelectedPhoto() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel >= 0 && sel < (int)m_orderItems.size()) {
        m_orderItems.erase(m_orderItems.begin() + sel);
        RefreshOrderListUI();
        if (!m_orderItems.empty()) {
            int newSel = (sel < (int)m_orderItems.size()) ? sel : (int)m_orderItems.size() - 1;
            SendMessageW(m_hListOrders, LB_SETCURSEL, newSel, 0);
            const auto& item = m_orderItems[newSel];
            m_isUpdatingUI = true;
            SetWindowTextW(m_hEditQty2x3, std::to_wstring(item.qty2x3).c_str());
            SetWindowTextW(m_hEditQty3x4, std::to_wstring(item.qty3x4).c_str());
            SetWindowTextW(m_hEditQty4x6, std::to_wstring(item.qty4x6).c_str());
            m_isUpdatingUI = false;
        } else {
            m_isUpdatingUI = true;
            SetWindowTextW(m_hEditQty2x3, L"0");
            SetWindowTextW(m_hEditQty3x4, L"0");
            SetWindowTextW(m_hEditQty4x6, L"0");
            m_isUpdatingUI = false;
        }
    }
}

void MainWindow::OnClearAllPhotos() {
    if (m_orderItems.empty()) return;

    if (MessageBoxW(m_hwnd, L"Hapus semua foto dari antrian?", L"Konfirmasi", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        m_orderItems.clear();
        m_imageProcessor.ClearCache();
        m_isUpdatingUI = true;
        SetWindowTextW(m_hEditQty2x3, L"0");
        SetWindowTextW(m_hEditQty3x4, L"0");
        SetWindowTextW(m_hEditQty4x6, L"0");
        m_isUpdatingUI = false;
        RefreshOrderListUI();
    }
}

void MainWindow::OnChangeQuantity(PhotoSizePreset preset, int newQty) {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) return;

    if (newQty < 0) newQty = 0;
    if (newQty > 99) newQty = 99;

    if (preset == PhotoSizePreset::Size2x3) {
        m_orderItems[sel].qty2x3 = newQty;
        m_isUpdatingUI = true;
        SetWindowTextW(m_hEditQty2x3, std::to_wstring(newQty).c_str());
        m_isUpdatingUI = false;
    } else if (preset == PhotoSizePreset::Size3x4) {
        m_orderItems[sel].qty3x4 = newQty;
        m_isUpdatingUI = true;
        SetWindowTextW(m_hEditQty3x4, std::to_wstring(newQty).c_str());
        m_isUpdatingUI = false;
    } else if (preset == PhotoSizePreset::Size4x6) {
        m_orderItems[sel].qty4x6 = newQty;
        m_isUpdatingUI = true;
        SetWindowTextW(m_hEditQty4x6, std::to_wstring(newQty).c_str());
        m_isUpdatingUI = false;
    }

    RefreshOrderListUI();
    SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
}

void MainWindow::OnChangePackingMode(PackingMode mode) {
    m_paperConfig.packingMode = mode;
    UpdateLayoutCalculation();
}

static UINT_PTR CALLBACK PrintHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam) {
    // Return 0 agar dialog print standar memproses semua event secara default
    // Flag PD_ENABLEPRINTHOOK memastikan Windows 11 menampilkan Classic Win32 Print Dialog
    // yang ringan, cepat, dan memiliki tombol 'Preferences/Properties' lengkap tanpa kotak hitam kosong.
    return 0;
}

void MainWindow::OnPrintPreview() {
    if (m_orderItems.empty()) {
        MessageBoxW(m_hwnd, L"Tambahkan foto terlebih dahulu sebelum membuka pratinjau cetak!", L"Perhatian", MB_OK | MB_ICONWARNING);
        return;
    }

    bool shouldPrint = PrintPreviewDialog::ShowModal(
        m_hwnd,
        m_hInstance,
        m_currentLayout,
        m_paperConfig,
        m_imageProcessor,
        m_currentPageIndex
    );

    if (shouldPrint) {
        OnPrintDirect();
    }
}

void MainWindow::OnPrintDirect() {
    if (m_orderItems.empty()) {
        MessageBoxW(m_hwnd, L"Belum ada foto yang ditambahkan!", L"Informasi Cetak", MB_OK | MB_ICONINFORMATION);
        return;
    }

    PRINTDLGW pd = { sizeof(PRINTDLGW) };
    pd.hwndOwner = m_hwnd;
    pd.Flags = PD_RETURNDC | PD_USEDEVMODECOPIESANDCOLLATE | PD_NOSELECTION | PD_ENABLEPRINTHOOK;
    pd.lpfnPrintHook = PrintHookProc;
    pd.nFromPage = 1;
    pd.nToPage = (WORD)(m_currentLayout.totalPages > 0 ? m_currentLayout.totalPages : 1);
    pd.nMinPage = 1;
    pd.nMaxPage = (WORD)(m_currentLayout.totalPages > 0 ? m_currentLayout.totalPages : 1);
    pd.nCopies = 1;

    if (PrintDlgW(&pd)) {
        HDC hdcPrinter = pd.hDC;
        if (hdcPrinter) {
            DOCINFOW docInfo = { sizeof(DOCINFOW) };
            docInfo.lpszDocName = L"208 Software Center - Cetak Pas Foto A4";

            if (StartDocW(hdcPrinter, &docInfo) > 0) {
                int dpiX = GetDeviceCaps(hdcPrinter, LOGPIXELSX);
                int dpiY = GetDeviceCaps(hdcPrinter, LOGPIXELSY);
                double printDpi = (dpiX > 0) ? static_cast<double>(dpiX) : 300.0;

                int startPage = 0;
                int endPage = (int)m_currentLayout.pages.size() - 1;
                if ((pd.Flags & PD_PAGENUMS) && pd.nFromPage >= 1 && pd.nToPage >= pd.nFromPage) {
                    startPage = pd.nFromPage - 1;
                    endPage = pd.nToPage - 1;
                    if (startPage < 0) startPage = 0;
                    if (endPage >= (int)m_currentLayout.pages.size()) {
                        endPage = (int)m_currentLayout.pages.size() - 1;
                    }
                }

                int printedPages = 0;
                for (int p = startPage; p <= endPage; ++p) {
                    if (StartPage(hdcPrinter) > 0) {
                        {
                            Gdiplus::Graphics g(hdcPrinter);
                            m_imageProcessor.RenderSheet(g, m_currentLayout.pages[p], m_paperConfig, printDpi, false, 1.0);
                        }
                        EndPage(hdcPrinter);
                        printedPages++;
                    }
                }

                EndDoc(hdcPrinter);

                wchar_t succMsg[128];
                swprintf_s(succMsg, L"%d lembar halaman berhasil dikirim ke printer!", printedPages);
                MessageBoxW(m_hwnd, succMsg, L"Cetak Berhasil", MB_OK | MB_ICONINFORMATION);
            }
            DeleteDC(hdcPrinter);
        }
    }
}

void MainWindow::OnExportImage() {
    if (m_orderItems.empty()) {
        MessageBoxW(m_hwnd, L"Belum ada foto yang ditambahkan!", L"Informasi Export", MB_OK | MB_ICONINFORMATION);
        return;
    }

    WCHAR szPath[MAX_PATH] = L"PasFoto_A4_Layout.pdf";
    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Dokumen PDF Multi-Halaman (*.pdf)\0*.pdf\0Gambar PNG (*.png)\0*.png\0Gambar JPEG (*.jpg)\0*.jpg\0Semua Format yang Didukung (*.pdf;*.png;*.jpg)\0*.pdf;*.png;*.jpg\0";
    ofn.lpstrFile = szPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"pdf";

    if (GetSaveFileNameW(&ofn)) {
        std::wstring outPath = szPath;
        std::wstring lowerPath = outPath;
        for (auto& c : lowerPath) c = towlower(c);

        bool isPdf = (ofn.nFilterIndex == 1) || (lowerPath.length() >= 4 && lowerPath.substr(lowerPath.length() - 4) == L".pdf");
        bool isJpg = (ofn.nFilterIndex == 3) || (lowerPath.length() >= 4 && lowerPath.substr(lowerPath.length() - 4) == L".jpg") || (lowerPath.length() >= 5 && lowerPath.substr(lowerPath.length() - 5) == L".jpeg");

        if (isPdf) {
            if (m_imageProcessor.ExportToPdf(outPath, m_currentLayout, m_paperConfig)) {
                wchar_t succMsg[512];
                swprintf_s(succMsg, L"Dokumen PDF %d halaman berhasil disimpan:\n%s", (int)m_currentLayout.pages.size(), outPath.c_str());
                MessageBoxW(m_hwnd, succMsg, L"Export PDF Berhasil", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(m_hwnd, L"Gagal menyimpan dokumen PDF!", L"Export Gagal", MB_OK | MB_ICONERROR);
            }
        } else {
            std::wstring mime = isJpg ? L"image/jpeg" : L"image/png";
            std::vector<std::wstring> files = m_imageProcessor.ExportAllPagesToFile(outPath, m_currentLayout, m_paperConfig, mime);
            if (!files.empty()) {
                wchar_t succMsg[512];
                if (files.size() == 1) {
                    swprintf_s(succMsg, L"File layout 300 DPI berhasil disimpan:\n%s", files[0].c_str());
                } else {
                    swprintf_s(succMsg, L"%d file layout halaman (300 DPI) berhasil disimpan!", (int)files.size());
                }
                MessageBoxW(m_hwnd, succMsg, L"Export Berhasil", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(m_hwnd, L"Gagal menyimpan file gambar!", L"Export Gagal", MB_OK | MB_ICONERROR);
            }
        }
    }
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DROPFILES: {
            HDROP hDrop = (HDROP)wParam;
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            for (UINT i = 0; i < count; ++i) {
                WCHAR filePath[MAX_PATH];
                if (DragQueryFileW(hDrop, i, filePath, MAX_PATH) > 0) {
                    OnAddSingleFilePath(filePath);
                }
            }
            DragFinish(hDrop);
            RefreshOrderListUI();
            if (!m_orderItems.empty()) {
                int newSel = (int)m_orderItems.size() - 1;
                SendMessageW(m_hListOrders, LB_SETCURSEL, newSel, 0);
                const auto& item = m_orderItems[newSel];
                m_isUpdatingUI = true;
                SetWindowTextW(m_hEditQty2x3, std::to_wstring(item.qty2x3).c_str());
                SetWindowTextW(m_hEditQty3x4, std::to_wstring(item.qty3x4).c_str());
                SetWindowTextW(m_hEditQty4x6, std::to_wstring(item.qty4x6).c_str());
                m_isUpdatingUI = false;
            }
            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == IDC_BTN_ADD) {
                OnAddPhotoFiles();
            } else if (wmId == IDC_BTN_REMOVE) {
                OnRemoveSelectedPhoto();
            } else if (wmId == IDC_BTN_CLEAR) {
                OnClearAllPhotos();
            } else if (wmId == IDC_BTN_MINUS_2X3) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size()) {
                    OnChangeQuantity(PhotoSizePreset::Size2x3, m_orderItems[sel].qty2x3 - 1);
                }
            } else if (wmId == IDC_BTN_PLUS_2X3) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size()) {
                    OnChangeQuantity(PhotoSizePreset::Size2x3, m_orderItems[sel].qty2x3 + 1);
                }
            } else if (wmId == IDC_BTN_MINUS_3X4) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size()) {
                    OnChangeQuantity(PhotoSizePreset::Size3x4, m_orderItems[sel].qty3x4 - 1);
                }
            } else if (wmId == IDC_BTN_PLUS_3X4) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size()) {
                    OnChangeQuantity(PhotoSizePreset::Size3x4, m_orderItems[sel].qty3x4 + 1);
                }
            } else if (wmId == IDC_BTN_MINUS_4X6) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size()) {
                    OnChangeQuantity(PhotoSizePreset::Size4x6, m_orderItems[sel].qty4x6 - 1);
                }
            } else if (wmId == IDC_BTN_PLUS_4X6) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size()) {
                    OnChangeQuantity(PhotoSizePreset::Size4x6, m_orderItems[sel].qty4x6 + 1);
                }
            } else if (wmId == IDC_EDIT_QTY_2X3 && wmEvent == EN_KILLFOCUS && !m_isUpdatingUI) {
                WCHAR buf[16] = { 0 };
                GetWindowTextW(m_hEditQty2x3, buf, 15);
                int qty = _wtoi(buf);
                OnChangeQuantity(PhotoSizePreset::Size2x3, qty);
            } else if (wmId == IDC_EDIT_QTY_3X4 && wmEvent == EN_KILLFOCUS && !m_isUpdatingUI) {
                WCHAR buf[16] = { 0 };
                GetWindowTextW(m_hEditQty3x4, buf, 15);
                int qty = _wtoi(buf);
                OnChangeQuantity(PhotoSizePreset::Size3x4, qty);
            } else if (wmId == IDC_EDIT_QTY_4X6 && wmEvent == EN_KILLFOCUS && !m_isUpdatingUI) {
                WCHAR buf[16] = { 0 };
                GetWindowTextW(m_hEditQty4x6, buf, 15);
                int qty = _wtoi(buf);
                OnChangeQuantity(PhotoSizePreset::Size4x6, qty);
            } else if (wmId == IDC_RADIO_SMARTSTRIP) {
                OnChangePackingMode(PackingMode::SmartStrip);
            } else if (wmId == IDC_RADIO_EASYCUT) {
                OnChangePackingMode(PackingMode::EasyCut);
            } else if (wmId == IDC_RADIO_MAXDENSITY) {
                OnChangePackingMode(PackingMode::MaxDensity);
            } else if (wmId == IDC_CHK_CUTLINES) {
                LRESULT chk = SendMessageW(m_hChkCutLines, BM_GETCHECK, 0, 0);
                m_paperConfig.drawCutLines = (chk == BST_CHECKED);
                UpdateLayoutCalculation();
            } else if (wmId == IDC_BTN_PREVIEW) {
                OnPrintPreview();
            } else if (wmId == IDC_BTN_PRINT) {
                OnPrintDirect();
            } else if (wmId == IDC_BTN_EXPORT) {
                OnExportImage();
            } else if (wmId == IDC_BTN_PREV_PAGE) {
                if (m_currentPageIndex > 0) {
                    m_currentPageIndex--;
                    UpdateLayoutCalculation();
                }
            } else if (wmId == IDC_BTN_NEXT_PAGE) {
                if (m_currentPageIndex < m_currentLayout.totalPages - 1) {
                    m_currentPageIndex++;
                    UpdateLayoutCalculation();
                }
            } else if (wmId == IDC_LIST_ORDERS && wmEvent == LBN_SELCHANGE) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size()) {
                    const auto& item = m_orderItems[sel];
                    m_isUpdatingUI = true;
                    SetWindowTextW(m_hEditQty2x3, std::to_wstring(item.qty2x3).c_str());
                    SetWindowTextW(m_hEditQty3x4, std::to_wstring(item.qty3x4).c_str());
                    SetWindowTextW(m_hEditQty4x6, std::to_wstring(item.qty4x6).c_str());
                    m_isUpdatingUI = false;
                }
            }
            return 0;
        }

        case WM_ACTIVATE: {
            if (LOWORD(wParam) != WA_INACTIVE) {
                RedrawWindow(m_hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
            }
            return 0;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, OPAQUE);
            SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
            SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }

        case WM_CTLCOLORBTN: {
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }

        case WM_SIZE: {
            int cx = LOWORD(lParam);
            int cy = HIWORD(lParam);
            ResizeLayout(cx, cy);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);

            RECT rcClient;
            GetClientRect(m_hwnd, &rcClient);

            // Bersihkan seluruh background window dengan warna standar (COLOR_BTNFACE)
            HBRUSH hbrFace = GetSysColorBrush(COLOR_BTNFACE);
            FillRect(hdc, &rcClient, hbrFace);

            RECT rcPreview = { 375, 48, rcClient.right - 10, rcClient.bottom - 10 };
            if (rcPreview.right > rcPreview.left && rcPreview.bottom > rcPreview.top) {
                DrawPreviewCanvas(hdc, rcPreview);
            }

            EndPaint(m_hwnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1; // Prevent flicker on resize/redraw

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(m_hwnd, uMsg, wParam, lParam);
}

} // namespace SoftwareCenter208
