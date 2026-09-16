#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <map>
#include <memory>
#include "../core/types.h"

namespace SoftwareCenter208 {

class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    // Cache image loader agar tidak re-read disk berulang kali saat render preview
    Gdiplus::Bitmap* GetOrLoadBitmap(const std::wstring& filePath);
    Gdiplus::Bitmap* GetOrProcessBgBitmap(
        const std::wstring& filePath,
        BgColorPreset bgPreset,
        uint32_t customBgColor,
        int tolerance
    );
    void ClearCache();

    // Render A4 canvas ke GDI+ Graphics untuk 1 halaman (PageLayout)
    void RenderSheet(
        Gdiplus::Graphics& graphics,
        const PageLayout& page,
        const PaperConfig& paper,
        double targetDpi = 300.0,
        bool isPreviewMode = false,
        double previewScale = 1.0
    );

    void RenderSheet(
        Gdiplus::Graphics& graphics,
        const PageLayout& page,
        const PaperConfig& paper,
        double targetDpiX,
        double targetDpiY,
        bool isPreviewMode = false,
        double previewScale = 1.0
    );

    // Render A4 canvas ke GDI+ Graphics untuk pageIndex tertentu dari LayoutResult
    void RenderSheet(
        Gdiplus::Graphics& graphics,
        const LayoutResult& layout,
        const PaperConfig& paper,
        int pageIndex = 0,
        double targetDpi = 300.0,
        bool isPreviewMode = false,
        double previewScale = 1.0
    );

    // Export 1 halaman ke file gambar 300 DPI (PNG / JPG / BMP)
    bool ExportPageToFile(
        const std::wstring& outPath,
        const PageLayout& page,
        const PaperConfig& paper,
        const std::wstring& mimeType = L"image/png"
    );

    // Export A4 sheet (halaman 1 atau file tunggal) ke file gambar 300 DPI
    bool ExportToFile(
        const std::wstring& outPath,
        const LayoutResult& layout,
        const PaperConfig& paper,
        const std::wstring& mimeType = L"image/png"
    );

    // Export seluruh halaman ke file-file gambar 300 DPI (e.g. out_Hal1.png, out_Hal2.png)
    std::vector<std::wstring> ExportAllPagesToFile(
        const std::wstring& baseOutPath,
        const LayoutResult& layout,
        const PaperConfig& paper,
        const std::wstring& mimeType = L"image/png"
    );

    // Export seluruh halaman layout ke file Dokumen PDF Multi-Halaman (Zero-dependency native PDF 1.4)
    bool ExportToPdf(
        const std::wstring& outPath,
        const LayoutResult& layout,
        const PaperConfig& paper
    );

    // Helper untuk mencari CLSID encoder GDI+ (PNG, JPEG, BMP)
    static int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

    // Static helper untuk memproses penggantian warna background pada bitmap
    static std::unique_ptr<Gdiplus::Bitmap> ProcessStudioBackground(
        Gdiplus::Bitmap* srcBmp,
        BgColorPreset bgPreset,
        uint32_t customBgColor,
        int tolerance
    );

private:
    std::map<std::wstring, std::unique_ptr<Gdiplus::Bitmap>> m_imageCache;
    std::map<std::wstring, std::unique_ptr<Gdiplus::Bitmap>> m_bgProcessedCache;

    void DrawPhotoSlot(
        Gdiplus::Graphics& graphics,
        Gdiplus::Bitmap* bmp,
        const PlacedPhotoSlot& slot,
        const PaperConfig& paper,
        double dpiX,
        double dpiY,
        double scale
    );
};

} // namespace SoftwareCenter208
