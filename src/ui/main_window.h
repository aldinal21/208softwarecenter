#pragma once

#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include "../core/types.h"
#include "../core/packing_engine.h"
#include "../graphics/image_processor.h"

namespace SoftwareCenter208 {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE hInstance, int nCmdShow);
    int RunMessageLoop();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitControls();
    void ResizeLayout(int clientWidth, int clientHeight);
    void UpdateLayoutCalculation();
    void RefreshOrderListUI();
    void DrawPreviewCanvas(HDC hdc, const RECT& previewRect);

    // Actions
    void OnAddPhotoFiles();
    void OnAddSingleFilePath(const std::wstring& path);
    void OnRemoveSelectedPhoto();
    void OnClearAllPhotos();
    void OnChangeQuantity(PhotoSizePreset preset, int newQty);
    void OnChangePackingMode(PackingMode mode);
    void OnPrintPreview();
    void OnPrintDirect();
    void OnExportImage();

    // Data members
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    HACCEL m_hAccel = nullptr;
    HFONT m_hFontUI = nullptr;
    HFONT m_hFontHeader = nullptr;
    HFONT m_hFontBold = nullptr;

    // Controls
    HWND m_hListOrders = nullptr;
    HWND m_hBtnAdd = nullptr;
    HWND m_hBtnRemove = nullptr;
    HWND m_hBtnClear = nullptr;

    // Multi-preset quantity controls per photo (- [ 0 ] +)
    HWND m_hBtnMinus2x3 = nullptr;
    HWND m_hEditQty2x3 = nullptr;
    HWND m_hBtnPlus2x3 = nullptr;

    HWND m_hBtnMinus3x4 = nullptr;
    HWND m_hEditQty3x4 = nullptr;
    HWND m_hBtnPlus3x4 = nullptr;

    HWND m_hBtnMinus4x6 = nullptr;
    HWND m_hEditQty4x6 = nullptr;
    HWND m_hBtnPlus4x6 = nullptr;

    // Packing mode controls
    HWND m_hRadioSmartStrip = nullptr;
    HWND m_hRadioEasyCut = nullptr;
    HWND m_hRadioMaxDensity = nullptr;
    HWND m_hChkCutLines = nullptr;

    // Output controls
    HWND m_hBtnPreview = nullptr;
    HWND m_hBtnPrint = nullptr;
    HWND m_hBtnExport = nullptr;
    HWND m_hStaticInfo = nullptr;
    HWND m_hStaticScrap = nullptr;

    // Page navigation controls
    HWND m_hBtnPrevPage = nullptr;
    HWND m_hStaticPageNum = nullptr;
    HWND m_hBtnNextPage = nullptr;
    int m_currentPageIndex = 0;

    // Domain models & services
    std::vector<PhotoOrderItem> m_orderItems;
    PaperConfig m_paperConfig;
    LayoutResult m_currentLayout;
    ImageProcessor m_imageProcessor;

    uint32_t m_nextItemId = 1;
    bool m_isUpdatingUI = false;
};

} // namespace SoftwareCenter208
