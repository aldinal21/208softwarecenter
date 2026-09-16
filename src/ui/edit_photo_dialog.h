#pragma once

#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include "../core/types.h"
#include "../graphics/image_processor.h"

namespace SoftwareCenter208 {

class EditPhotoDialog {
public:
    static bool ShowModal(
        HWND hParent,
        HINSTANCE hInstance,
        PhotoOrderItem& item,
        ImageProcessor& imageProcessor
    );

private:
    EditPhotoDialog(
        HWND hParent,
        HINSTANCE hInstance,
        PhotoOrderItem& item,
        ImageProcessor& imageProcessor
    );
    ~EditPhotoDialog();

    bool CreateAndRun();

    static LRESULT CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    void InitControls();
    void UpdateControlsState();
    void DrawCanvas(HDC hdc);

    HWND m_hwnd = nullptr;
    HWND m_hParent = nullptr;
    HINSTANCE m_hInstance = nullptr;

    HFONT m_hFontUI = nullptr;
    HFONT m_hFontBold = nullptr;
    HFONT m_hFontSmall = nullptr;
    HFONT m_hFontTitle = nullptr;

    // Controls
    HWND m_hBtnRotateCCW = nullptr;
    HWND m_hBtnRotateCW = nullptr;
    HWND m_hStaticRotate = nullptr;

    HWND m_hSliderZoom = nullptr;
    HWND m_hStaticZoomVal = nullptr;
    HWND m_hBtnZoomReset = nullptr;

    HWND m_hBtnAspect2x3 = nullptr;
    HWND m_hBtnAspect3x4 = nullptr;
    HWND m_hBtnAspect4x6 = nullptr;
    HWND m_hBtnResetPos = nullptr;
    HWND m_hChkBw = nullptr;

    // Background color controls
    HWND m_hBtnBgNone = nullptr;
    HWND m_hBtnBgRed = nullptr;
    HWND m_hBtnBgBlue = nullptr;
    HWND m_hBtnBgWhite = nullptr;
    HWND m_hBtnBgGrey = nullptr;
    HWND m_hBtnBgYellow = nullptr;
    HWND m_hSliderBgTol = nullptr;
    HWND m_hStaticBgTol = nullptr;

    HWND m_hBtnSave = nullptr;
    HWND m_hBtnCancel = nullptr;

    PhotoOrderItem& m_item;
    ImageProcessor& m_imageProcessor;

    // Working copy of parameters
    double m_cropCenterX = 0.5;
    double m_cropCenterY = 0.45;
    double m_zoomLevel = 1.0;
    int m_rotationAngle = 0;
    bool m_isBlackAndWhite = false;
    BgColorPreset m_bgPreset = BgColorPreset::None;
    uint32_t m_customBgColor = 0x00FFFFFF;
    int m_bgTolerance = 30;
    PhotoSizePreset m_previewPreset = PhotoSizePreset::Size3x4;

    // Canvas viewport geometry
    RECT m_canvasRect = { 15, 50, 535, 615 };

    // Mouse drag state
    bool m_isDragging = false;
    POINT m_lastMousePos = { 0, 0 };
    double m_fitScale = 1.0;
    double m_imgDispX = 0.0;
    double m_imgDispY = 0.0;
    double m_effImgW = 0.0;
    double m_effImgH = 0.0;

    bool m_saved = false;
};

} // namespace SoftwareCenter208
