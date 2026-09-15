#pragma once

#include <windows.h>
#include <gdiplus.h>
#include "../core/types.h"
#include "../graphics/image_processor.h"

namespace SoftwareCenter208 {

class PrintPreviewDialog {
public:
    static bool ShowModal(
        HWND hParent,
        HINSTANCE hInstance,
        const LayoutResult& layout,
        const PaperConfig& paper,
        ImageProcessor& imageProcessor,
        int initialPage = 0
    );

private:
    PrintPreviewDialog(
        HWND hParent,
        HINSTANCE hInstance,
        const LayoutResult& layout,
        const PaperConfig& paper,
        ImageProcessor& imageProcessor,
        int initialPage
    );
    ~PrintPreviewDialog();

    bool CreateAndRun();

    static LRESULT CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitControls();
    void ResizeLayout(int clientWidth, int clientHeight);
    void UpdatePageControls();
    void DrawPreviewCanvas(HDC hdc, const RECT& canvasRect);

    HWND m_hwnd = nullptr;
    HWND m_hParent = nullptr;
    HINSTANCE m_hInstance = nullptr;

    HFONT m_hFontUI = nullptr;
    HFONT m_hFontBold = nullptr;

    // Controls
    HWND m_hBtnPrint = nullptr;
    HWND m_hBtnPrev = nullptr;
    HWND m_hStaticPage = nullptr;
    HWND m_hBtnNext = nullptr;
    HWND m_hBtnZoomIn = nullptr;
    HWND m_hBtnZoomOut = nullptr;
    HWND m_hBtnZoomFit = nullptr;
    HWND m_hBtnClose = nullptr;

    const LayoutResult& m_layout;
    const PaperConfig& m_paper;
    ImageProcessor& m_imageProcessor;

    int m_currentPage = 0;
    double m_zoomFactor = 1.0; // 1.0 = Fit window
    bool m_shouldPrint = false;
};

} // namespace SoftwareCenter208
