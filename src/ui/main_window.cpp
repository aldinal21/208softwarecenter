#include "main_window.h"
#include "preview_dialog.h"
#include "edit_photo_dialog.h"
#include "custom_size_dialog.h"
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
    IDC_BTN_DUPLICATE,
    IDC_BTN_EDIT_PHOTO,

    // Dynamic print sizes per photo
    IDC_COMBO_SIZE_PRESETS,
    IDC_BTN_ADD_SIZE,
    IDC_LIST_SIZES,
    IDC_BTN_MINUS_QTY,
    IDC_EDIT_QTY,
    IDC_BTN_PLUS_QTY,
    IDC_BTN_TOGGLE_ORIENTATION,
    IDC_BTN_REMOVE_SIZE,
    IDC_CHK_BW,

    // Paper size & Packing mode
    IDC_COMBO_PAPER,
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

static const PhotoSizePreset kAllPresets[] = {
    PhotoSizePreset::Size3x4,
    PhotoSizePreset::Size2x3,
    PhotoSizePreset::Size4x6,
    PhotoSizePreset::SizeVisaUS,
    PhotoSizePreset::SizeVisaSchengen,
    PhotoSizePreset::SizeVisaChina,
    PhotoSizePreset::SizeWallet,
    PhotoSizePreset::Size2R,
    PhotoSizePreset::Size3R,
    PhotoSizePreset::Size4R,
    PhotoSizePreset::Size5R,
    PhotoSizePreset::Size6R,
    PhotoSizePreset::Size8R,
    PhotoSizePreset::Custom
};

static const PaperSizePreset kAllPaperPresets[] = {
    PaperSizePreset::A4,
    PaperSizePreset::F4_Folio,
    PaperSizePreset::A3,
    PaperSizePreset::A3Plus,
    PaperSizePreset::Letter,
    PaperSizePreset::Photo4R
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

    // Keyboard Accelerators (Ctrl+P -> Print, Ctrl+S -> Export, Ctrl+O -> Add, Delete -> Remove)
    ACCEL accels[] = {
        { FCONTROL | FVIRTKEY, 'P', IDC_BTN_PRINT },
        { FCONTROL | FVIRTKEY, 'S', IDC_BTN_EXPORT },
        { FCONTROL | FVIRTKEY, 'O', IDC_BTN_ADD },
        { FVIRTKEY, VK_DELETE, IDC_BTN_REMOVE }
    };
    m_hAccel = CreateAcceleratorTableW(accels, sizeof(accels) / sizeof(ACCEL));

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
        22, 38, 326, 120,
        m_hwnd, (HMENU)IDC_LIST_ORDERS, m_hInstance, nullptr
    );
    SendMessageW(m_hListOrders, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Tombol Antrian (Add, Duplicate, Remove, Clear)
    m_hBtnAdd = CreateWindowExW(
        0, L"BUTTON", L"+ Tambah",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 162, 76, 26,
        m_hwnd, (HMENU)IDC_BTN_ADD, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnAdd, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnDuplicate = CreateWindowExW(
        0, L"BUTTON", L"📋 Duplikat",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        104, 162, 76, 26,
        m_hwnd, (HMENU)IDC_BTN_DUPLICATE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnDuplicate, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnRemove = CreateWindowExW(
        0, L"BUTTON", L"- Hapus",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        186, 162, 76, 26,
        m_hwnd, (HMENU)IDC_BTN_REMOVE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnRemove, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnClear = CreateWindowExW(
        0, L"BUTTON", L"🗑️ Bersihkan",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        268, 162, 80, 26,
        m_hwnd, (HMENU)IDC_BTN_CLEAR, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnClear, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnEditPhoto = CreateWindowExW(
        0, L"BUTTON", L"📐 Atur Posisi / Putar / Latar (Edit Foto)...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 192, 326, 26,
        m_hwnd, (HMENU)IDC_BTN_EDIT_PHOTO, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnEditPhoto, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Group Box 2: Ukuran Cetak per Foto
    HWND hGrpSize = CreateWindowExW(
        0, L"BUTTON", L"Ukuran Cetak Foto Terpilih",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        22, 224, 326, 154,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpSize, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Preset ComboBox & Tambah Button
    m_hComboSizePresets = CreateWindowExW(
        0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        32, 246, 218, 200,
        m_hwnd, (HMENU)IDC_COMBO_SIZE_PRESETS, m_hInstance, nullptr
    );
    SendMessageW(m_hComboSizePresets, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    for (PhotoSizePreset p : kAllPresets) {
        int idx = (int)SendMessageW(m_hComboSizePresets, CB_ADDSTRING, 0, (LPARAM)GetPresetName(p));
        SendMessageW(m_hComboSizePresets, CB_SETITEMDATA, idx, (LPARAM)p);
    }
    SendMessageW(m_hComboSizePresets, CB_SETCURSEL, 0, 0); // Default: 3x4

    m_hBtnAddSize = CreateWindowExW(
        0, L"BUTTON", L"+ Tambah",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        254, 245, 86, 26,
        m_hwnd, (HMENU)IDC_BTN_ADD_SIZE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnAddSize, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // ListBox Ukuran Cetak
    m_hListSizes = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS | WS_TABSTOP,
        32, 276, 308, 62,
        m_hwnd, (HMENU)IDC_LIST_SIZES, m_hInstance, nullptr
    );
    SendMessageW(m_hListSizes, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Stepper, Toggle Orientation, Remove Size & B&W
    HWND hLblQtyTitle = CreateWindowExW(
        0, L"STATIC", L"Qty:",
        WS_CHILD | WS_VISIBLE,
        32, 348, 26, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblQtyTitle, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnMinusQty = CreateWindowExW(
        0, L"BUTTON", L"-",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        58, 345, 24, 24,
        m_hwnd, (HMENU)IDC_BTN_MINUS_QTY, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnMinusQty, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hEditQty = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"1",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER | WS_TABSTOP,
        84, 345, 32, 24,
        m_hwnd, (HMENU)IDC_EDIT_QTY, m_hInstance, nullptr
    );
    SendMessageW(m_hEditQty, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnPlusQty = CreateWindowExW(
        0, L"BUTTON", L"+",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        118, 345, 24, 24,
        m_hwnd, (HMENU)IDC_BTN_PLUS_QTY, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPlusQty, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnToggleOrientation = CreateWindowExW(
        0, L"BUTTON", L"⇄ Putar",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        146, 345, 62, 24,
        m_hwnd, (HMENU)IDC_BTN_TOGGLE_ORIENTATION, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnToggleOrientation, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hBtnRemoveSize = CreateWindowExW(
        0, L"BUTTON", L"🗑️ Hapus",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        212, 345, 60, 24,
        m_hwnd, (HMENU)IDC_BTN_REMOVE_SIZE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnRemoveSize, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hChkBw = CreateWindowExW(
        0, L"BUTTON", L"⚫⚪ B&W",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
        276, 347, 66, 20,
        m_hwnd, (HMENU)IDC_CHK_BW, m_hInstance, nullptr
    );
    SendMessageW(m_hChkBw, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Group Box 3: Ukuran Kertas & Mode Tata Letak (Packing)
    HWND hGrpMode = CreateWindowExW(
        0, L"BUTTON", L"Ukuran Kertas & Tata Letak",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        22, 386, 326, 148,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hGrpMode, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    HWND hLblPaper = CreateWindowExW(
        0, L"STATIC", L"Kertas:",
        WS_CHILD | WS_VISIBLE,
        34, 408, 48, 20,
        m_hwnd, nullptr, m_hInstance, nullptr
    );
    SendMessageW(hLblPaper, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hComboPaper = CreateWindowExW(
        0, L"COMBOBOX", nullptr,
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
        84, 405, 256, 200,
        m_hwnd, (HMENU)IDC_COMBO_PAPER, m_hInstance, nullptr
    );
    SendMessageW(m_hComboPaper, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    for (PaperSizePreset p : kAllPaperPresets) {
        int idx = (int)SendMessageW(m_hComboPaper, CB_ADDSTRING, 0, (LPARAM)GetPaperPresetName(p));
        SendMessageW(m_hComboPaper, CB_SETITEMDATA, idx, (LPARAM)p);
    }
    SendMessageW(m_hComboPaper, CB_SETCURSEL, 0, 0); // Default: A4

    m_hRadioSmartStrip = CreateWindowExW(
        0, L"BUTTON", L"⚡ Smart Strip (Gunting Mudah & Hemat)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        34, 434, 300, 18,
        m_hwnd, (HMENU)IDC_RADIO_SMARTSTRIP, m_hInstance, nullptr
    );
    SendMessageW(m_hRadioSmartStrip, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);
    SendMessageW(m_hRadioSmartStrip, BM_SETCHECK, BST_CHECKED, 0);

    m_hRadioEasyCut = CreateWindowExW(
        0, L"BUTTON", L"✂️ Baris Murni (1 Baris 1 Ukuran Sama)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        34, 455, 300, 18,
        m_hwnd, (HMENU)IDC_RADIO_EASYCUT, m_hInstance, nullptr
    );
    SendMessageW(m_hRadioEasyCut, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hRadioMaxDensity = CreateWindowExW(
        0, L"BUTTON", L"📐 Hemat Maksimal (Isi Celah Kosong)",
        WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
        34, 476, 300, 18,
        m_hwnd, (HMENU)IDC_RADIO_MAXDENSITY, m_hInstance, nullptr
    );
    SendMessageW(m_hRadioMaxDensity, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hChkCutLines = CreateWindowExW(
        0, L"BUTTON", L"Garis Batas Potong (Cut Guide Lines)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        34, 500, 300, 18,
        m_hwnd, (HMENU)IDC_CHK_CUTLINES, m_hInstance, nullptr
    );
    SendMessageW(m_hChkCutLines, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);
    SendMessageW(m_hChkCutLines, BM_SETCHECK, BST_CHECKED, 0);

    // Status Efisiensi Kertas Sisa
    m_hStaticInfo = CreateWindowExW(
        0, L"STATIC", L"Total Foto: 0 lembar",
        WS_CHILD | WS_VISIBLE,
        22, 542, 326, 18,
        m_hwnd, (HMENU)IDC_STATIC_INFO, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticInfo, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hStaticScrap = CreateWindowExW(
        0, L"STATIC", L"Sisa Kertas A4: 29.7 cm (Utuh)",
        WS_CHILD | WS_VISIBLE,
        22, 562, 326, 20,
        m_hwnd, (HMENU)IDC_STATIC_SCRAP, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticScrap, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Tombol Pratinjau, Cetak & Export (Bottom Actions)
    m_hBtnPreview = CreateWindowExW(
        0, L"BUTTON", L"🔍 Pratinjau Cetak (Print Preview)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 588, 326, 30,
        m_hwnd, (HMENU)IDC_BTN_PREVIEW, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPreview, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    m_hBtnPrint = CreateWindowExW(
        0, L"BUTTON", L"🖨️ Cetak Langsung (Ctrl+P)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 622, 326, 30,
        m_hwnd, (HMENU)IDC_BTN_PRINT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPrint, WM_SETFONT, (WPARAM)m_hFontHeader, TRUE);

    m_hBtnExport = CreateWindowExW(
        0, L"BUTTON", L"💾 Export Gambar / PDF (Ctrl+S)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        22, 656, 326, 28,
        m_hwnd, (HMENU)IDC_BTN_EXPORT, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnExport, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    // Navigasi Halaman Preview
    m_hBtnPrevPage = CreateWindowExW(
        0, L"BUTTON", L"◀ Hal. Sblm",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        375, 12, 90, 26,
        m_hwnd, (HMENU)IDC_BTN_PREV_PAGE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnPrevPage, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    m_hStaticPageNum = CreateWindowExW(
        0, L"STATIC", L"Halaman 1 dari 1",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        475, 15, 180, 24,
        m_hwnd, (HMENU)IDC_STATIC_PAGE_NUM, m_hInstance, nullptr
    );
    SendMessageW(m_hStaticPageNum, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hBtnNextPage = CreateWindowExW(
        0, L"BUTTON", L"Hal. Brkt ▶",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        665, 12, 90, 26,
        m_hwnd, (HMENU)IDC_BTN_NEXT_PAGE, m_hInstance, nullptr
    );
    SendMessageW(m_hBtnNextPage, WM_SETFONT, (WPARAM)m_hFontUI, TRUE);

    RefreshSizeListUI();
}

void MainWindow::ResizeLayout(int clientWidth, int clientHeight) {
    RECT rcPreview = { 375, 48, clientWidth - 10, clientHeight - 10 };
    InvalidateRect(m_hwnd, &rcPreview, TRUE);
}

void MainWindow::UpdateLayoutCalculation() {
    m_currentLayout = PackingEngine::CalculateLayout(m_orderItems, m_paperConfig);

    if (m_currentPageIndex >= m_currentLayout.totalPages) {
        m_currentPageIndex = (m_currentLayout.totalPages > 0) ? m_currentLayout.totalPages - 1 : 0;
    }
    if (m_currentPageIndex < 0) {
        m_currentPageIndex = 0;
    }

    // Ambil nama kertas ringkas (tanpa dimensi panjang)
    std::wstring paperName = GetPaperPresetName(m_paperConfig.paperPreset);
    size_t parenPos = paperName.find(L" (");
    if (parenPos != std::wstring::npos) {
        paperName = paperName.substr(0, parenPos);
    }

    // Update status bar text
    int totalPhotos = m_currentLayout.totalPhotosPlaced;
    int totalPages = m_currentLayout.totalPages;
    int curPage = m_currentPageIndex + 1;

    wchar_t infoText[256];
    if (totalPages <= 1) {
        swprintf_s(infoText, L"Total Foto: %d lembar (1 Halaman %s)", totalPhotos, paperName.c_str());
    } else {
        swprintf_s(infoText, L"Total Foto: %d lembar (%d Lembar %s)", totalPhotos, totalPages, paperName.c_str());
    }
    SetWindowTextW(m_hStaticInfo, infoText);

    // Update status scrap sisa kertas
    wchar_t scrapText[256];
    if (m_orderItems.empty() || totalPhotos == 0) {
        swprintf_s(scrapText, L"Sisa Kertas %s: %.1f cm (Utuh)", paperName.c_str(), m_paperConfig.heightMm / 10.0);
    } else {
        double remMm = (m_currentPageIndex < (int)m_currentLayout.pages.size())
            ? m_currentLayout.pages[m_currentPageIndex].remainingHeightMm
            : m_currentLayout.remainingHeightMm;
        double remCm = remMm / 10.0;
        double usedMm = (m_currentPageIndex < (int)m_currentLayout.pages.size())
            ? m_currentLayout.pages[m_currentPageIndex].usedHeightMm
            : m_currentLayout.usedHeightMm;
        double usedCm = usedMm / 10.0;

        swprintf_s(scrapText, L"Terpakai: %.1f cm | Sisa Bersih: %.1f cm", usedCm, remCm);
    }
    SetWindowTextW(m_hStaticScrap, scrapText);

    // Update page navigation buttons & text
    wchar_t pageNavText[128];
    swprintf_s(pageNavText, L"Halaman %d dari %d", curPage, (totalPages > 0 ? totalPages : 1));
    SetWindowTextW(m_hStaticPageNum, pageNavText);

    EnableWindow(m_hBtnPrevPage, m_currentPageIndex > 0);
    EnableWindow(m_hBtnNextPage, m_currentPageIndex < totalPages - 1);

    // Trigger repaint of canvas
    InvalidateRect(m_hwnd, nullptr, FALSE);
}

void MainWindow::RefreshOrderListUI() {
    int curSel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    SendMessageW(m_hListOrders, LB_RESETCONTENT, 0, 0);

    for (size_t i = 0; i < m_orderItems.size(); ++i) {
        const auto& item = m_orderItems[i];
        std::wstring fileName = PathFindFileNameW(item.sourceFilePath.c_str());

        // Format summary ukuran
        std::wstring sizeSummary;
        for (size_t s = 0; s < item.printSizes.size(); ++s) {
            if (s > 0) sizeSummary += L", ";
            std::wstring shortLabel = item.printSizes[s].label;
            size_t parenPos = shortLabel.find(L" (");
            if (parenPos != std::wstring::npos) {
                shortLabel = shortLabel.substr(0, parenPos);
            }
            sizeSummary += shortLabel + L":" + std::to_wstring(item.printSizes[s].quantity);
        }
        if (sizeSummary.empty()) {
            sizeSummary = L"0 lembar";
        }

        wchar_t itemText[256];
        if (item.isBlackAndWhite) {
            swprintf_s(itemText, L"%zu. %s [%s] [B&W]", i + 1, fileName.c_str(), sizeSummary.c_str());
        } else {
            swprintf_s(itemText, L"%zu. %s [%s]", i + 1, fileName.c_str(), sizeSummary.c_str());
        }

        SendMessageW(m_hListOrders, LB_ADDSTRING, 0, (LPARAM)itemText);
    }

    if (!m_orderItems.empty()) {
        if (curSel < 0 || curSel >= (int)m_orderItems.size()) {
            curSel = 0;
        }
        SendMessageW(m_hListOrders, LB_SETCURSEL, curSel, 0);
    }

    RefreshSizeListUI();
    UpdateLayoutCalculation();
}

void MainWindow::RefreshSizeListUI() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    int sizeSel = (int)SendMessageW(m_hListSizes, LB_GETCURSEL, 0, 0);

    m_isUpdatingUI = true;
    SendMessageW(m_hListSizes, LB_RESETCONTENT, 0, 0);

    if (sel < 0 || sel >= (int)m_orderItems.size()) {
        EnableWindow(m_hComboSizePresets, FALSE);
        EnableWindow(m_hBtnAddSize, FALSE);
        EnableWindow(m_hListSizes, FALSE);
        EnableWindow(m_hBtnMinusQty, FALSE);
        EnableWindow(m_hEditQty, FALSE);
        EnableWindow(m_hBtnPlusQty, FALSE);
        EnableWindow(m_hBtnToggleOrientation, FALSE);
        EnableWindow(m_hBtnRemoveSize, FALSE);
        EnableWindow(m_hChkBw, FALSE);
        SetWindowTextW(m_hEditQty, L"0");
        SendMessageW(m_hChkBw, BM_SETCHECK, BST_UNCHECKED, 0);
        m_isUpdatingUI = false;
        return;
    }

    EnableWindow(m_hComboSizePresets, TRUE);
    EnableWindow(m_hBtnAddSize, TRUE);
    EnableWindow(m_hListSizes, TRUE);
    EnableWindow(m_hChkBw, TRUE);

    const auto& item = m_orderItems[sel];
    SendMessageW(m_hChkBw, BM_SETCHECK, item.isBlackAndWhite ? BST_CHECKED : BST_UNCHECKED, 0);

    for (size_t i = 0; i < item.printSizes.size(); ++i) {
        const auto& ps = item.printSizes[i];
        wchar_t szLine[256];
        swprintf_s(szLine, L"• %s  —  %d lbr", ps.label.c_str(), ps.quantity);
        SendMessageW(m_hListSizes, LB_ADDSTRING, 0, (LPARAM)szLine);
    }

    if (!item.printSizes.empty()) {
        if (sizeSel < 0 || sizeSel >= (int)item.printSizes.size()) {
            sizeSel = 0;
        }
        SendMessageW(m_hListSizes, LB_SETCURSEL, sizeSel, 0);
        SetWindowTextW(m_hEditQty, std::to_wstring(item.printSizes[sizeSel].quantity).c_str());

        EnableWindow(m_hBtnMinusQty, TRUE);
        EnableWindow(m_hEditQty, TRUE);
        EnableWindow(m_hBtnPlusQty, TRUE);
        EnableWindow(m_hBtnToggleOrientation, TRUE);
        EnableWindow(m_hBtnRemoveSize, TRUE);
    } else {
        SetWindowTextW(m_hEditQty, L"0");
        EnableWindow(m_hBtnMinusQty, FALSE);
        EnableWindow(m_hEditQty, FALSE);
        EnableWindow(m_hBtnPlusQty, FALSE);
        EnableWindow(m_hBtnToggleOrientation, FALSE);
        EnableWindow(m_hBtnRemoveSize, FALSE);
    }

    m_isUpdatingUI = false;
}

void MainWindow::DrawPreviewCanvas(HDC hdc, const RECT& previewRect) {
    int availW = previewRect.right - previewRect.left;
    int availH = previewRect.bottom - previewRect.top;
    if (availW <= 10 || availH <= 10) return;

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
    screenGraphics.DrawImage(&memBmp, (INT)previewRect.left, (INT)previewRect.top);
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
            RefreshSizeListUI();
        }
    }
}

void MainWindow::OnAddSingleFilePath(const std::wstring& path) {
    std::wstring ext = PathFindExtensionW(path.c_str());
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

    if (ext != L".jpg" && ext != L".jpeg" && ext != L".png" && ext != L".bmp" && ext != L".tif" && ext != L".tiff") {
        return; // Skip non-image files
    }

    PhotoOrderItem item;
    item.id = m_nextItemId++;
    item.sourceFilePath = path;
    // Foto baru mulai dengan daftar ukuran kosong sampai user menambahkan ukuran cetak

    m_orderItems.push_back(item);
}

void MainWindow::OnRemoveSelectedPhoto() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel >= 0 && sel < (int)m_orderItems.size()) {
        m_orderItems.erase(m_orderItems.begin() + sel);
        int newSel = sel;
        if (newSel >= (int)m_orderItems.size()) {
            newSel = (int)m_orderItems.size() - 1;
        }
        RefreshOrderListUI();
        if (newSel >= 0) {
            SendMessageW(m_hListOrders, LB_SETCURSEL, newSel, 0);
        }
        RefreshSizeListUI();
        UpdateLayoutCalculation();
    }
}

void MainWindow::OnClearAllPhotos() {
    if (m_orderItems.empty()) return;
    if (MessageBoxW(m_hwnd, L"Kosongkan semua antrian foto?", L"Konfirmasi", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        m_orderItems.clear();
        m_currentPageIndex = 0;
        RefreshOrderListUI();
        RefreshSizeListUI();
        UpdateLayoutCalculation();
    }
}

void MainWindow::OnDuplicateSelectedPhoto() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) {
        MessageBoxW(m_hwnd, L"Pilih foto yang ingin diduplikat dari daftar antrian!", L"Duplikat Foto", MB_OK | MB_ICONINFORMATION);
        return;
    }

    PhotoOrderItem itemCopy = m_orderItems[sel];
    itemCopy.id = m_nextItemId++;
    m_orderItems.insert(m_orderItems.begin() + sel + 1, itemCopy);

    RefreshOrderListUI();
    SendMessageW(m_hListOrders, LB_SETCURSEL, sel + 1, 0);
    RefreshSizeListUI();
    UpdateLayoutCalculation();
}

void MainWindow::OnToggleBlackAndWhite() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) return;

    m_orderItems[sel].isBlackAndWhite = (SendMessageW(m_hChkBw, BM_GETCHECK, 0, 0) == BST_CHECKED);
    RefreshOrderListUI();
    SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
    UpdateLayoutCalculation();
}

void MainWindow::OnEditPhotoCrop() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) {
        MessageBoxW(m_hwnd, L"Pilih foto yang ingin diatur posisinya dari daftar antrian!", L"Edit Foto", MB_OK | MB_ICONINFORMATION);
        return;
    }

    PhotoOrderItem& item = m_orderItems[sel];
    bool changed = EditPhotoDialog::ShowModal(m_hwnd, m_hInstance, item, m_imageProcessor);
    if (changed) {
        RefreshOrderListUI();
        SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
        RefreshSizeListUI();
        UpdateLayoutCalculation();
    }
}

void MainWindow::OnAddSizeToSelectedPhoto() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) return;

    int comboSel = (int)SendMessageW(m_hComboSizePresets, CB_GETCURSEL, 0, 0);
    if (comboSel < 0) return;

    PhotoSizePreset preset = (PhotoSizePreset)SendMessageW(m_hComboSizePresets, CB_GETITEMDATA, comboSel, 0);

    if (preset == PhotoSizePreset::Custom) {
        PhotoPrintSize customSize;
        customSize.preset = PhotoSizePreset::Custom;
        customSize.widthMm = 50.0;
        customSize.heightMm = 70.0;
        customSize.quantity = 1;

        if (CustomSizeDialog::ShowModal(m_hwnd, m_hInstance, customSize)) {
            m_orderItems[sel].printSizes.push_back(customSize);
            RefreshOrderListUI();
            SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
            SendMessageW(m_hListSizes, LB_SETCURSEL, (WPARAM)(m_orderItems[sel].printSizes.size() - 1), 0);
            RefreshSizeListUI();
            UpdateLayoutCalculation();
        }
        return;
    }

    // Auto-detect landscape orientation for studio photo sizes (2R to 8R)
    bool isStudioSize = (preset == PhotoSizePreset::Size2R ||
                         preset == PhotoSizePreset::Size3R ||
                         preset == PhotoSizePreset::Size4R ||
                         preset == PhotoSizePreset::Size5R ||
                         preset == PhotoSizePreset::Size6R ||
                         preset == PhotoSizePreset::Size8R);

    bool autoLandscape = false;
    if (isStudioSize && !m_orderItems[sel].sourceFilePath.empty()) {
        std::unique_ptr<Gdiplus::Bitmap> bmp = std::make_unique<Gdiplus::Bitmap>(m_orderItems[sel].sourceFilePath.c_str());
        if (bmp && bmp->GetWidth() > 0 && bmp->GetHeight() > 0) {
            if (bmp->GetWidth() > bmp->GetHeight()) {
                autoLandscape = true;
            }
        }
    }

    // Check if preset already exists with same orientation
    bool found = false;
    for (size_t i = 0; i < m_orderItems[sel].printSizes.size(); ++i) {
        if (m_orderItems[sel].printSizes[i].preset == preset &&
            m_orderItems[sel].printSizes[i].isLandscape == autoLandscape) {
            m_orderItems[sel].printSizes[i].quantity++;
            found = true;
            RefreshOrderListUI();
            SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
            SendMessageW(m_hListSizes, LB_SETCURSEL, (WPARAM)i, 0);
            RefreshSizeListUI();
            UpdateLayoutCalculation();
            break;
        }
    }

    if (!found) {
        PhotoPrintSize ps = CreateDefaultPrintSize(preset, autoLandscape);
        m_orderItems[sel].printSizes.push_back(ps);
        RefreshOrderListUI();
        SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
        SendMessageW(m_hListSizes, LB_SETCURSEL, (WPARAM)(m_orderItems[sel].printSizes.size() - 1), 0);
        RefreshSizeListUI();
        UpdateLayoutCalculation();
    }
}

void MainWindow::OnRemoveSelectedSize() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) return;

    int sizeSel = (int)SendMessageW(m_hListSizes, LB_GETCURSEL, 0, 0);
    if (sizeSel < 0 || sizeSel >= (int)m_orderItems[sel].printSizes.size()) return;

    m_orderItems[sel].printSizes.erase(m_orderItems[sel].printSizes.begin() + sizeSel);

    int newSizeSel = sizeSel;
    if (newSizeSel >= (int)m_orderItems[sel].printSizes.size()) {
        newSizeSel = (int)m_orderItems[sel].printSizes.size() - 1;
    }

    RefreshOrderListUI();
    SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
    if (newSizeSel >= 0) {
        SendMessageW(m_hListSizes, LB_SETCURSEL, newSizeSel, 0);
    }
    RefreshSizeListUI();
    UpdateLayoutCalculation();
}

void MainWindow::OnToggleSizeOrientation() {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) return;

    int sizeSel = (int)SendMessageW(m_hListSizes, LB_GETCURSEL, 0, 0);
    if (sizeSel < 0 || sizeSel >= (int)m_orderItems[sel].printSizes.size()) return;

    PhotoPrintSize& ps = m_orderItems[sel].printSizes[sizeSel];
    ps.isLandscape = !ps.isLandscape;
    std::swap(ps.widthMm, ps.heightMm);

    // Recompute label
    std::wstring baseLabel = GetPresetName(ps.preset);
    if (ps.preset == PhotoSizePreset::Custom) {
        wchar_t buf[128];
        swprintf_s(buf, L"Custom (%.1f x %.1f cm)", ps.widthMm / 10.0, ps.heightMm / 10.0);
        ps.label = buf;
    } else {
        if (ps.isLandscape) {
            wchar_t buf[128];
            swprintf_s(buf, L" (%.1f x %.1f cm) [Mendatar]", ps.widthMm / 10.0, ps.heightMm / 10.0);
            size_t parenPos = baseLabel.find(L" (");
            if (parenPos != std::wstring::npos) {
                ps.label = baseLabel.substr(0, parenPos) + buf;
            } else {
                ps.label = baseLabel + buf;
            }
        } else {
            ps.label = baseLabel;
        }
    }

    RefreshOrderListUI();
    SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
    SendMessageW(m_hListSizes, LB_SETCURSEL, sizeSel, 0);
    RefreshSizeListUI();
    UpdateLayoutCalculation();
}

void MainWindow::OnChangeSizeQuantity(int newQty) {
    int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)m_orderItems.size()) return;

    int sizeSel = (int)SendMessageW(m_hListSizes, LB_GETCURSEL, 0, 0);
    if (sizeSel < 0 || sizeSel >= (int)m_orderItems[sel].printSizes.size()) return;

    if (newQty < 1) newQty = 1;
    if (newQty > 999) newQty = 999;

    m_orderItems[sel].printSizes[sizeSel].quantity = newQty;

    m_isUpdatingUI = true;
    SetWindowTextW(m_hEditQty, std::to_wstring(newQty).c_str());
    m_isUpdatingUI = false;

    RefreshOrderListUI();
    SendMessageW(m_hListOrders, LB_SETCURSEL, sel, 0);
    SendMessageW(m_hListSizes, LB_SETCURSEL, sizeSel, 0);
    RefreshSizeListUI();
    UpdateLayoutCalculation();
}

void MainWindow::OnChangePaperSize() {
    int comboSel = (int)SendMessageW(m_hComboPaper, CB_GETCURSEL, 0, 0);
    if (comboSel < 0) return;

    PaperSizePreset preset = (PaperSizePreset)SendMessageW(m_hComboPaper, CB_GETITEMDATA, comboSel, 0);
    m_paperConfig.paperPreset = preset;
    MillimeterSize dim = GetPaperPresetDimensionMm(preset);
    m_paperConfig.widthMm = dim.widthMm;
    m_paperConfig.heightMm = dim.heightMm;

    m_currentPageIndex = 0;
    UpdateLayoutCalculation();
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
                double printDpiX = (dpiX > 0) ? static_cast<double>(dpiX) : 300.0;
                double printDpiY = (dpiY > 0) ? static_cast<double>(dpiY) : printDpiX;

                int physOffsetX = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETX);
                int physOffsetY = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETY);

                int totalPagesToPrint = m_currentLayout.totalPages;
                for (int pageIdx = 0; pageIdx < totalPagesToPrint; ++pageIdx) {
                    StartPage(hdcPrinter);
                    {
                        Gdiplus::Graphics g(hdcPrinter);
                        g.SetPageUnit(Gdiplus::UnitPixel);
                        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
                        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);

                        g.TranslateTransform(
                            static_cast<Gdiplus::REAL>(-physOffsetX),
                            static_cast<Gdiplus::REAL>(-physOffsetY)
                        );

                        m_imageProcessor.RenderSheet(
                            g,
                            m_currentLayout,
                            m_paperConfig,
                            pageIdx,
                            printDpiX,
                            false,
                            1.0
                        );
                    }
                    EndPage(hdcPrinter);
                }
                EndDoc(hdcPrinter);
                MessageBoxW(m_hwnd, L"Pencetakan berhasil dikirim ke antrian printer!", L"Pencetakan Selesai", MB_OK | MB_ICONINFORMATION);
            }
            DeleteDC(hdcPrinter);
        }
        if (pd.hDevMode) GlobalFree(pd.hDevMode);
        if (pd.hDevNames) GlobalFree(pd.hDevNames);
    }
}

void MainWindow::OnExportImage() {
    if (m_orderItems.empty()) {
        MessageBoxW(m_hwnd, L"Belum ada foto yang ditambahkan untuk di-export!", L"Export Gambar / PDF", MB_OK | MB_ICONINFORMATION);
        return;
    }

    WCHAR szPath[MAX_PATH] = L"PasFoto_A4_SiapCetak.pdf";

    OPENFILENAMEW ofn = { sizeof(OPENFILENAMEW) };
    ofn.hwndOwner = m_hwnd;
    ofn.lpstrFilter = L"Dokumen PDF (*.pdf)\0*.pdf\0Gambar PNG (*.png)\0*.png\0Gambar JPEG (*.jpg;*.jpeg)\0*.jpg;*.jpeg\0";
    ofn.lpstrFile = szPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"pdf";

    if (GetSaveFileNameW(&ofn)) {
        std::wstring outPath = szPath;
        std::wstring ext = PathFindExtensionW(szPath);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

        if (ext == L".pdf") {
            bool ok = m_imageProcessor.ExportToPdf(outPath, m_currentLayout, m_paperConfig);
            if (ok) {
                MessageBoxW(m_hwnd, L"Dokumen PDF siap cetak 300 DPI berhasil disimpan!", L"Export Sukses", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(m_hwnd, L"Gagal menyimpan dokumen PDF!", L"Export Gagal", MB_OK | MB_ICONERROR);
            }
        } else {
            std::wstring mimeType = L"image/png";
            if (ext == L".jpg" || ext == L".jpeg") {
                mimeType = L"image/jpeg";
            }

            bool ok = false;
            if (m_currentPageIndex >= 0 && m_currentPageIndex < (int)m_currentLayout.pages.size()) {
                ok = m_imageProcessor.ExportPageToFile(outPath, m_currentLayout.pages[m_currentPageIndex], m_paperConfig, mimeType);
            } else {
                ok = m_imageProcessor.ExportToFile(outPath, m_currentLayout, m_paperConfig, mimeType);
            }

            if (ok) {
                MessageBoxW(m_hwnd, L"Gambar tata letak A4 siap cetak 300 DPI berhasil disimpan!", L"Export Sukses", MB_OK | MB_ICONINFORMATION);
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
                WCHAR szFile[MAX_PATH];
                if (DragQueryFileW(hDrop, i, szFile, MAX_PATH)) {
                    OnAddSingleFilePath(szFile);
                }
            }
            DragFinish(hDrop);
            RefreshOrderListUI();
            if (!m_orderItems.empty()) {
                int newSel = (int)m_orderItems.size() - 1;
                SendMessageW(m_hListOrders, LB_SETCURSEL, newSel, 0);
                RefreshSizeListUI();
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
            } else if (wmId == IDC_BTN_DUPLICATE) {
                OnDuplicateSelectedPhoto();
            } else if (wmId == IDC_BTN_EDIT_PHOTO) {
                OnEditPhotoCrop();
            } else if (wmId == IDC_BTN_ADD_SIZE) {
                OnAddSizeToSelectedPhoto();
            } else if (wmId == IDC_BTN_REMOVE_SIZE) {
                OnRemoveSelectedSize();
            } else if (wmId == IDC_BTN_MINUS_QTY) {
                WCHAR szQty[32] = { 0 };
                GetWindowTextW(m_hEditQty, szQty, 31);
                int q = _wtoi(szQty);
                if (q > 1) {
                    OnChangeSizeQuantity(q - 1);
                }
            } else if (wmId == IDC_BTN_PLUS_QTY) {
                WCHAR szQty[32] = { 0 };
                GetWindowTextW(m_hEditQty, szQty, 31);
                int q = _wtoi(szQty);
                if (q < 999) {
                    OnChangeSizeQuantity(q + 1);
                }
            } else if (wmId == IDC_EDIT_QTY && wmEvent == EN_KILLFOCUS) {
                if (!m_isUpdatingUI) {
                    WCHAR szQty[32] = { 0 };
                    GetWindowTextW(m_hEditQty, szQty, 31);
                    int q = _wtoi(szQty);
                    OnChangeSizeQuantity(q);
                }
            } else if (wmId == IDC_BTN_TOGGLE_ORIENTATION) {
                OnToggleSizeOrientation();
            } else if (wmId == IDC_COMBO_PAPER && wmEvent == CBN_SELCHANGE) {
                OnChangePaperSize();
            } else if (wmId == IDC_CHK_BW) {
                OnToggleBlackAndWhite();
            } else if (wmId == IDC_RADIO_SMARTSTRIP) {
                OnChangePackingMode(PackingMode::SmartStrip);
            } else if (wmId == IDC_RADIO_EASYCUT) {
                OnChangePackingMode(PackingMode::EasyCut);
            } else if (wmId == IDC_RADIO_MAXDENSITY) {
                OnChangePackingMode(PackingMode::MaxDensity);
            } else if (wmId == IDC_CHK_CUTLINES) {
                m_paperConfig.drawCutLines = (SendMessageW(m_hChkCutLines, BM_GETCHECK, 0, 0) == BST_CHECKED);
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
            } else if (wmId == IDC_LIST_ORDERS && wmEvent == LBN_DBLCLK) {
                OnEditPhotoCrop();
            } else if (wmId == IDC_LIST_ORDERS && wmEvent == LBN_SELCHANGE) {
                RefreshSizeListUI();
            } else if (wmId == IDC_LIST_SIZES && wmEvent == LBN_SELCHANGE) {
                int sel = (int)SendMessageW(m_hListOrders, LB_GETCURSEL, 0, 0);
                int sizeSel = (int)SendMessageW(m_hListSizes, LB_GETCURSEL, 0, 0);
                if (sel >= 0 && sel < (int)m_orderItems.size() &&
                    sizeSel >= 0 && sizeSel < (int)m_orderItems[sel].printSizes.size()) {
                    m_isUpdatingUI = true;
                    SetWindowTextW(m_hEditQty, std::to_wstring(m_orderItems[sel].printSizes[sizeSel].quantity).c_str());
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
