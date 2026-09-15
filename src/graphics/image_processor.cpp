#include "image_processor.h"
#include <algorithm>
#include <cmath>

namespace SoftwareCenter208 {

ImageProcessor::ImageProcessor() {
}

ImageProcessor::~ImageProcessor() {
    ClearCache();
}

void ImageProcessor::ClearCache() {
    m_imageCache.clear();
}

Gdiplus::Bitmap* ImageProcessor::GetOrLoadBitmap(const std::wstring& filePath) {
    auto it = m_imageCache.find(filePath);
    if (it != m_imageCache.end()) {
        return it->second.get();
    }

    auto bmp = std::make_unique<Gdiplus::Bitmap>(filePath.c_str());
    if (bmp->GetLastStatus() != Gdiplus::Ok) {
        return nullptr;
    }

    Gdiplus::Bitmap* rawPtr = bmp.get();
    m_imageCache[filePath] = std::move(bmp);
    return rawPtr;
}

int ImageProcessor::GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    auto pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
    if (pImageCodecInfo == nullptr) return -1;

    Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j) {
        if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;
        }
    }

    free(pImageCodecInfo);
    return -1;
}

void ImageProcessor::RenderSheet(
    Gdiplus::Graphics& graphics,
    const PageLayout& page,
    const PaperConfig& paper,
    double targetDpi,
    bool isPreviewMode,
    double previewScale
) {
    RenderSheet(graphics, page, paper, targetDpi, targetDpi, isPreviewMode, previewScale);
}

void ImageProcessor::RenderSheet(
    Gdiplus::Graphics& graphics,
    const PageLayout& page,
    const PaperConfig& paper,
    double targetDpiX,
    double targetDpiY,
    bool isPreviewMode,
    double previewScale
) {
    if (targetDpiY <= 0.0) targetDpiY = targetDpiX;
    double scale = isPreviewMode ? previewScale : 1.0;

    if (isPreviewMode) {
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeBilinear);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighSpeed);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    } else {
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
    }

    // Hitung total dimensi kertas dalam unit pixel target
    int paperPxW = MmToPixels(paper.widthMm, targetDpiX);
    int paperPxH = MmToPixels(paper.heightMm, targetDpiY);

    if (isPreviewMode) {
        paperPxW = static_cast<int>(paperPxW * scale);
        paperPxH = static_cast<int>(paperPxH * scale);
    }

    // 1. Gambar latar belakang kertas A4 putih bersih
    Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255));
    graphics.FillRectangle(&whiteBrush, 0, 0, paperPxW, paperPxH);

    // 2. Render setiap foto pada posisinya di halaman ini
    for (const auto& slot : page.slots) {
        Gdiplus::Bitmap* bmp = GetOrLoadBitmap(slot.sourceFilePath);
        DrawPhotoSlot(graphics, bmp, slot, paper, targetDpiX, targetDpiY, scale);
    }

    // 3. Render Garis Batas Kertas Sisa (Scrap Line) jika ada sisa kertas
    if (page.usedHeightMm > 0 && page.usedHeightMm < paper.heightMm) {
        int scrapLineY = MmToPixels(page.usedHeightMm, targetDpiY);
        if (isPreviewMode) scrapLineY = static_cast<int>(scrapLineY * scale);

        // Garis batas potong sisa kertas (Warna Cyan / Biru Panduan)
        Gdiplus::Pen scrapPen(Gdiplus::Color(64, 128, 220), isPreviewMode ? 1.0f : 2.0f);
        scrapPen.SetDashStyle(Gdiplus::DashStyleDash);
        graphics.DrawLine(&scrapPen, 0, scrapLineY, paperPxW, scrapLineY);

        if (isPreviewMode) {
            // Label teks info sisa kertas
            Gdiplus::Font font(L"Segoe UI", 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush textBrush(Gdiplus::Color(100, 140, 200));

            wchar_t infoText[128];
            swprintf_s(infoText, L"-- SISA KERTAS BAWAH (%.1f mm / %.1f cm) BISA DISIMPAN --",
                page.remainingHeightMm, page.remainingHeightMm / 10.0);

            Gdiplus::PointF origin(10.0f, static_cast<float>(scrapLineY + 6));
            graphics.DrawString(infoText, -1, &font, origin, &textBrush);
        }
    }
}

void ImageProcessor::RenderSheet(
    Gdiplus::Graphics& graphics,
    const LayoutResult& layout,
    const PaperConfig& paper,
    int pageIndex,
    double targetDpi,
    bool isPreviewMode,
    double previewScale
) {
    if (layout.pages.empty() || pageIndex < 0 || pageIndex >= static_cast<int>(layout.pages.size())) {
        PageLayout emptyPage;
        emptyPage.pageIndex = 0;
        emptyPage.usedHeightMm = 0.0;
        emptyPage.remainingHeightMm = paper.heightMm;
        RenderSheet(graphics, emptyPage, paper, targetDpi, isPreviewMode, previewScale);
        return;
    }

    RenderSheet(graphics, layout.pages[pageIndex], paper, targetDpi, isPreviewMode, previewScale);
}

void ImageProcessor::DrawPhotoSlot(
    Gdiplus::Graphics& graphics,
    Gdiplus::Bitmap* bmp,
    const PlacedPhotoSlot& slot,
    const PaperConfig& paper,
    double dpiX,
    double dpiY,
    double scale
) {
    int destX = MmToPixels(slot.xMm, dpiX);
    int destY = MmToPixels(slot.yMm, dpiY);
    int destW = MmToPixels(slot.widthMm, dpiX);
    int destH = MmToPixels(slot.heightMm, dpiY);

    if (scale != 1.0) {
        destX = static_cast<int>(destX * scale);
        destY = static_cast<int>(destY * scale);
        destW = static_cast<int>(destW * scale);
        destH = static_cast<int>(destH * scale);
    }

    if (bmp != nullptr && bmp->GetLastStatus() == Gdiplus::Ok) {
        double imgW = bmp->GetWidth();
        double imgH = bmp->GetHeight();

        if (imgW > 0 && imgH > 0) {
            // Target Aspect Ratio (W / H)
            double targetAspect = slot.widthMm / slot.heightMm;
            double srcAspect = imgW / imgH;

            double cropW, cropH;
            if (srcAspect > targetAspect) {
                // Gambar sumber lebih lebar: fit height, crop width
                cropH = imgH;
                cropW = cropH * targetAspect;
            } else {
                // Gambar sumber lebih tinggi: fit width, crop height
                cropW = imgW;
                cropH = cropW / targetAspect;
            }

            // Terapkan Zoom Level
            double zoom = (slot.zoomLevel > 0.1) ? slot.zoomLevel : 1.0;
            cropW /= zoom;
            cropH /= zoom;

            // Pastikan crop tidak melebihi dimensi gambar asli
            if (cropW > imgW) cropW = imgW;
            if (cropH > imgH) cropH = imgH;

            // Hitung Crop Center
            double centerX = imgW * slot.cropCenterX;
            double centerY = imgH * slot.cropCenterY;

            double srcX = centerX - (cropW / 2.0);
            double srcY = centerY - (cropH / 2.0);

            // Clamp crop box agar tetap di dalam gambar
            if (srcX < 0) srcX = 0;
            if (srcY < 0) srcY = 0;
            if (srcX + cropW > imgW) srcX = imgW - cropW;
            if (srcY + cropH > imgH) srcY = imgH - cropH;

            Gdiplus::Rect destRect(destX, destY, destW, destH);
            graphics.DrawImage(
                bmp,
                destRect,
                static_cast<INT>(srcX),
                static_cast<INT>(srcY),
                static_cast<INT>(cropW),
                static_cast<INT>(cropH),
                Gdiplus::UnitPixel
            );
        }
    } else {
        // Placeholder jika gambar belum dimuat / error
        Gdiplus::SolidBrush grayBrush(Gdiplus::Color(240, 240, 245));
        graphics.FillRectangle(&grayBrush, destX, destY, destW, destH);

        Gdiplus::Pen borderPen(Gdiplus::Color(200, 200, 210), 1.0f);
        graphics.DrawRectangle(&borderPen, destX, destY, destW, destH);

        Gdiplus::Font font(L"Tahoma", 8.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPoint);
        Gdiplus::SolidBrush textBrush(Gdiplus::Color(150, 150, 160));
        Gdiplus::PointF pt(static_cast<float>(destX + 4), static_cast<float>(destY + 4));
        graphics.DrawString(L"No Image", -1, &font, pt, &textBrush);
    }

    // 4. Garis Panduan Potong (Guillotine Cut Border)
    if (paper.drawCutLines) {
        // Garis potong halus warna abu-abu muda 0.5px / 1px
        Gdiplus::Pen cutPen(Gdiplus::Color(200, 200, 200), 1.0f);
        cutPen.SetDashStyle(Gdiplus::DashStyleDot);
        graphics.DrawRectangle(&cutPen, destX, destY, destW, destH);
    }
}

bool ImageProcessor::ExportPageToFile(
    const std::wstring& outPath,
    const PageLayout& page,
    const PaperConfig& paper,
    const std::wstring& mimeType
) {
    const double printDpi = 300.0;
    int pixelWidth = MmToPixels(paper.widthMm, printDpi);
    int pixelHeight = MmToPixels(paper.heightMm, printDpi);

    CLSID encoderClsid;
    if (GetEncoderClsid(mimeType.c_str(), &encoderClsid) < 0) {
        return false;
    }

    Gdiplus::Bitmap exportBmp(pixelWidth, pixelHeight, PixelFormat24bppRGB);
    exportBmp.SetResolution(static_cast<Gdiplus::REAL>(printDpi), static_cast<Gdiplus::REAL>(printDpi));

    {
        Gdiplus::Graphics g(&exportBmp);
        RenderSheet(g, page, paper, printDpi, false, 1.0);
    }

    Gdiplus::Status status = exportBmp.Save(outPath.c_str(), &encoderClsid, nullptr);
    return (status == Gdiplus::Ok);
}

bool ImageProcessor::ExportToFile(
    const std::wstring& outPath,
    const LayoutResult& layout,
    const PaperConfig& paper,
    const std::wstring& mimeType
) {
    if (layout.pages.empty()) return false;
    return ExportPageToFile(outPath, layout.pages[0], paper, mimeType);
}

std::vector<std::wstring> ImageProcessor::ExportAllPagesToFile(
    const std::wstring& baseOutPath,
    const LayoutResult& layout,
    const PaperConfig& paper,
    const std::wstring& mimeType
) {
    std::vector<std::wstring> generatedFiles;
    if (layout.pages.empty()) return generatedFiles;

    if (layout.pages.size() == 1) {
        if (ExportPageToFile(baseOutPath, layout.pages[0], paper, mimeType)) {
            generatedFiles.push_back(baseOutPath);
        }
        return generatedFiles;
    }

    size_t dotPos = baseOutPath.find_last_of(L'.');
    std::wstring prefix = (dotPos != std::wstring::npos) ? baseOutPath.substr(0, dotPos) : baseOutPath;
    std::wstring ext = (dotPos != std::wstring::npos) ? baseOutPath.substr(dotPos) : L".png";

    for (size_t i = 0; i < layout.pages.size(); ++i) {
        std::wstring pagePath = prefix + L"_Hal" + std::to_wstring(i + 1) + ext;
        if (ExportPageToFile(pagePath, layout.pages[i], paper, mimeType)) {
            generatedFiles.push_back(pagePath);
        }
    }
    return generatedFiles;
}

bool ImageProcessor::ExportToPdf(
    const std::wstring& outPath,
    const LayoutResult& layout,
    const PaperConfig& paper
) {
    if (layout.pages.empty()) return false;

    HANDLE hFile = CreateFileW(
        outPath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) return false;

    auto writeStr = [hFile](const std::string& str) {
        DWORD written = 0;
        WriteFile(hFile, str.data(), static_cast<DWORD>(str.size()), &written, nullptr);
    };

    auto writeBytes = [hFile](const void* data, size_t size) {
        DWORD written = 0;
        WriteFile(hFile, data, static_cast<DWORD>(size), &written, nullptr);
    };

    CLSID clsidJpeg;
    if (GetEncoderClsid(L"image/jpeg", &clsidJpeg) < 0) {
        CloseHandle(hFile);
        return false;
    }

    const double printDpi = 300.0;
    int pixelWidth = MmToPixels(paper.widthMm, printDpi);
    int pixelHeight = MmToPixels(paper.heightMm, printDpi);

    // Standard PDF points (72 points per inch)
    double ptW = (paper.widthMm / 25.4) * 72.0;
    double ptH = (paper.heightMm / 25.4) * 72.0;

    // 1. PDF Header (Standard binary header to prevent ASCII translation)
    writeStr("%PDF-1.4\n%\xe2\xe3\xcf\xd3\n");
    size_t curOffset = 15;

    size_t numPages = layout.pages.size();
    size_t totalObjs = 2 + numPages * 3; // Obj 1 (Catalog), Obj 2 (Pages), per page: Page, Contents, Image
    std::vector<size_t> objOffsets(totalObjs + 1, 0);

    // Obj 1: Catalog
    objOffsets[1] = curOffset;
    std::string catObj = "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n";
    writeStr(catObj);
    curOffset += catObj.size();

    // Obj 2: Pages
    objOffsets[2] = curOffset;
    std::string kidsStr = "";
    for (size_t i = 0; i < numPages; ++i) {
        size_t pageObjId = 3 + i * 3;
        kidsStr += std::to_string(pageObjId) + " 0 R ";
    }
    std::string pagesObj = "2 0 obj\n<< /Type /Pages /Kids [ " + kidsStr + "] /Count " + std::to_string(numPages) + " >>\nendobj\n";
    writeStr(pagesObj);
    curOffset += pagesObj.size();

    // Per-page objects
    for (size_t i = 0; i < numPages; ++i) {
        size_t pageObjId = 3 + i * 3;
        size_t contentObjId = 4 + i * 3;
        size_t imageObjId = 5 + i * 3;

        // Render page to 300 DPI high-quality bitmap
        Gdiplus::Bitmap exportBmp(pixelWidth, pixelHeight, PixelFormat24bppRGB);
        exportBmp.SetResolution(static_cast<Gdiplus::REAL>(printDpi), static_cast<Gdiplus::REAL>(printDpi));
        {
            Gdiplus::Graphics g(&exportBmp);
            RenderSheet(g, layout.pages[i], paper, printDpi, false, 1.0);
        }

        // Compress rendered sheet into in-memory JPEG stream with 95% quality
        IStream* pStream = nullptr;
        std::vector<uint8_t> jpegBytes;
        if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pStream))) {
            Gdiplus::EncoderParameters encoderParams;
            encoderParams.Count = 1;
            encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
            encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
            encoderParams.Parameter[0].NumberOfValues = 1;
            ULONG quality = 95;
            encoderParams.Parameter[0].Value = &quality;

            if (exportBmp.Save(pStream, &clsidJpeg, &encoderParams) == Gdiplus::Ok) {
                STATSTG stat;
                pStream->Stat(&stat, STATFLAG_NONAME);
                ULONG streamSize = static_cast<ULONG>(stat.cbSize.QuadPart);
                jpegBytes.resize(streamSize);

                LARGE_INTEGER liZero = { 0 };
                pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
                ULONG bytesRead = 0;
                pStream->Read(jpegBytes.data(), streamSize, &bytesRead);
            }
            pStream->Release();
        }

        if (jpegBytes.empty()) {
            CloseHandle(hFile);
            return false;
        }

        char buf[512];
        sprintf_s(buf, "%.2f %.2f", ptW, ptH);
        std::string mediaBoxStr = buf;

        // Page Object
        objOffsets[pageObjId] = curOffset;
        std::string pageObj = std::to_string(pageObjId) + " 0 obj\n"
            "<< /Type /Page /Parent 2 0 R /MediaBox [ 0 0 " + mediaBoxStr + " ] "
            "/Resources << /ProcSet [ /PDF /ImageC ] /XObject << /Im1 " + std::to_string(imageObjId) + " 0 R >> >> "
            "/Contents " + std::to_string(contentObjId) + " 0 R >>\nendobj\n";
        writeStr(pageObj);
        curOffset += pageObj.size();

        // Contents Object (Scales image to fit page dimension in pt)
        sprintf_s(buf, "q\n%.2f 0 0 %.2f 0 0 cm\n/Im1 Do\nQ\n", ptW, ptH);
        std::string contentStream = buf;

        objOffsets[contentObjId] = curOffset;
        std::string contentObj = std::to_string(contentObjId) + " 0 obj\n"
            "<< /Length " + std::to_string(contentStream.size()) + " >>\nstream\n"
            + contentStream + "endstream\nendobj\n";
        writeStr(contentObj);
        curOffset += contentObj.size();

        // Image Object (XObject DCTDecode JPEG stream)
        objOffsets[imageObjId] = curOffset;
        std::string imageHeader = std::to_string(imageObjId) + " 0 obj\n"
            "<< /Type /XObject /Subtype /Image /Width " + std::to_string(pixelWidth)
            + " /Height " + std::to_string(pixelHeight)
            + " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length "
            + std::to_string(jpegBytes.size()) + " >>\nstream\n";
        writeStr(imageHeader);
        curOffset += imageHeader.size();

        writeBytes(jpegBytes.data(), jpegBytes.size());
        curOffset += jpegBytes.size();

        std::string imageFooter = "\nendstream\nendobj\n";
        writeStr(imageFooter);
        curOffset += imageFooter.size();
    }

    // Cross-Reference Table
    size_t xrefOffset = curOffset;
    writeStr("xref\n0 " + std::to_string(totalObjs + 1) + "\n");
    writeStr("0000000000 65535 f \r\n");
    for (size_t i = 1; i <= totalObjs; ++i) {
        char xrefLine[32];
        sprintf_s(xrefLine, "%010u 00000 n \r\n", static_cast<unsigned int>(objOffsets[i]));
        writeStr(xrefLine);
    }

    // Trailer
    std::string trailer = "trailer\n<< /Size " + std::to_string(totalObjs + 1)
        + " /Root 1 0 R >>\nstartxref\n" + std::to_string(xrefOffset) + "\n%%EOF\n";
    writeStr(trailer);

    CloseHandle(hFile);
    return true;
}

} // namespace SoftwareCenter208
