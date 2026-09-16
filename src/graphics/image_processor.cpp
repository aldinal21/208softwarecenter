#include "image_processor.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace SoftwareCenter208 {

ImageProcessor::ImageProcessor() {
}

ImageProcessor::~ImageProcessor() {
    ClearCache();
}

void ImageProcessor::ClearCache() {
    m_imageCache.clear();
    m_bgProcessedCache.clear();
}

Gdiplus::Bitmap* ImageProcessor::GetOrProcessBgBitmap(
    const std::wstring& filePath,
    BgColorPreset bgPreset,
    uint32_t customBgColor,
    int tolerance
) {
    if (bgPreset == BgColorPreset::None) {
        return GetOrLoadBitmap(filePath);
    }

    wchar_t cacheKey[512];
    swprintf_s(cacheKey, L"%ls_bg%d_%08X_%d", filePath.c_str(), static_cast<int>(bgPreset), customBgColor, tolerance);
    std::wstring key(cacheKey);

    auto it = m_bgProcessedCache.find(key);
    if (it != m_bgProcessedCache.end()) {
        return it->second.get();
    }

    Gdiplus::Bitmap* srcBmp = GetOrLoadBitmap(filePath);
    if (!srcBmp || srcBmp->GetWidth() == 0 || srcBmp->GetHeight() == 0) {
        return nullptr;
    }

    auto processed = ProcessStudioBackground(srcBmp, bgPreset, customBgColor, tolerance);
    if (!processed) {
        return srcBmp;
    }

    Gdiplus::Bitmap* rawPtr = processed.get();
    m_bgProcessedCache[key] = std::move(processed);
    return rawPtr;
}

std::unique_ptr<Gdiplus::Bitmap> ImageProcessor::ProcessStudioBackground(
    Gdiplus::Bitmap* srcBmp,
    BgColorPreset bgPreset,
    uint32_t customBgColor,
    int tolerance
) {
    if (!srcBmp || bgPreset == BgColorPreset::None) return nullptr;

    UINT width = srcBmp->GetWidth();
    UINT height = srcBmp->GetHeight();
    if (width == 0 || height == 0) return nullptr;

    auto resultBmp = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppARGB);
    if (resultBmp->GetLastStatus() != Gdiplus::Ok) return nullptr;

    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::BitmapData srcData;
    Gdiplus::BitmapData dstData;

    if (srcBmp->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &srcData) != Gdiplus::Ok) {
        return nullptr;
    }
    if (resultBmp->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &dstData) != Gdiplus::Ok) {
        srcBmp->UnlockBits(&srcData);
        return nullptr;
    }

    const BYTE* srcScan0 = reinterpret_cast<const BYTE*>(srcData.Scan0);
    BYTE* dstScan0 = reinterpret_cast<BYTE*>(dstData.Scan0);
    int srcStride = srcData.Stride;
    int dstStride = dstData.Stride;

    // 1. Sample Background Seed Colors from top corners and perimeter
    std::vector<std::pair<int, int>> seedPositions;
    for (int x = 0; x < static_cast<int>(width); x += std::max(1, static_cast<int>(width) / 10)) {
        seedPositions.push_back({x, 0});
        seedPositions.push_back({x, 1});
        seedPositions.push_back({x, 2});
    }
    for (int y = 0; y < std::min(10, static_cast<int>(height)); ++y) {
        seedPositions.push_back({0, y});
        seedPositions.push_back({static_cast<int>(width) - 1, y});
    }

    struct BgSeed {
        double r, g, b;
    };
    std::vector<BgSeed> seeds;
    for (const auto& pos : seedPositions) {
        const BYTE* pixel = srcScan0 + pos.second * srcStride + pos.first * 4;
        seeds.push_back({static_cast<double>(pixel[2]), static_cast<double>(pixel[1]), static_cast<double>(pixel[0])});
    }

    // Helper: Redmean perceptual color distance
    auto calcRedmeanDistance = [](double r1, double g1, double b1, double r2, double g2, double b2) -> double {
        double rmean = (r1 + r2) * 0.5;
        double dr = r1 - r2;
        double dg = g1 - g2;
        double db = b1 - b2;
        return std::sqrt((2.0 + rmean / 256.0) * dr * dr + 4.0 * dg * dg + (2.0 + (255.0 - rmean) / 256.0) * db * db);
    };

    // Target Color ARGB
    uint32_t targetArgb = GetBgPresetArgb(bgPreset, customBgColor);
    BYTE targetA = (targetArgb >> 24) & 0xFF;
    BYTE targetR = (targetArgb >> 16) & 0xFF;
    BYTE targetG = (targetArgb >> 8) & 0xFF;
    BYTE targetB = targetArgb & 0xFF;

    // Threshold calculation scaled for Redmean metric
    double tolDist = (static_cast<double>(tolerance) / 100.0) * 220.0;
    double tolSoft = tolDist * 1.6;

    // 2. Multi-seed Connected Flood Fill (Queue BFS)
    std::vector<uint8_t> bgMask(width * height, 0);
    std::vector<int> queue;
    queue.reserve(width * height / 4);

    auto isBgMatch = [&](int x, int y) -> bool {
        const BYTE* p = srcScan0 + y * srcStride + x * 4;
        double pr = p[2], pg = p[1], pb = p[0];
        for (const auto& s : seeds) {
            double dist = calcRedmeanDistance(pr, pg, pb, s.r, s.g, s.b);
            if (dist <= tolSoft) return true;
        }
        return false;
    };

    // Push initial boundary seeds
    for (int x = 0; x < static_cast<int>(width); ++x) {
        if (isBgMatch(x, 0)) {
            bgMask[0 * width + x] = 1;
            queue.push_back(0 * width + x);
        }
    }
    for (int y = 0; y < static_cast<int>(height); ++y) {
        if (!bgMask[y * width + 0] && isBgMatch(0, y)) {
            bgMask[y * width + 0] = 1;
            queue.push_back(y * width + 0);
        }
        if (!bgMask[y * width + (width - 1)] && isBgMatch(width - 1, y)) {
            bgMask[y * width + (width - 1)] = 1;
            queue.push_back(y * width + (width - 1));
        }
    }

    // BFS Expansion
    size_t head = 0;
    while (head < queue.size()) {
        int idx = queue[head++];
        int qx = idx % width;
        int qy = idx / width;

        const int dx[] = { 1, -1, 0, 0 };
        const int dy[] = { 0, 0, 1, -1 };

        for (int d = 0; d < 4; ++d) {
            int nx = qx + dx[d];
            int ny = qy + dy[d];
            if (nx >= 0 && nx < static_cast<int>(width) && ny >= 0 && ny < static_cast<int>(height)) {
                int nidx = ny * width + nx;
                if (!bgMask[nidx] && isBgMatch(nx, ny)) {
                    bgMask[nidx] = 1;
                    queue.push_back(nidx);
                }
            }
        }
    }

    // 3. Compute Alpha Matte Map (0.0 = Background, 1.0 = Foreground Subject)
    std::vector<float> alphaMatte(width * height, 1.0f);
    for (UINT y = 0; y < height; ++y) {
        const BYTE* srcRow = srcScan0 + y * srcStride;
        for (UINT x = 0; x < width; ++x) {
            int idx = y * width + x;
            if (bgMask[idx]) {
                const BYTE* sp = srcRow + x * 4;
                double pr = sp[2], pg = sp[1], pb = sp[0];
                double minDist = 9999.0;
                for (const auto& s : seeds) {
                    double dist = calcRedmeanDistance(pr, pg, pb, s.r, s.g, s.b);
                    if (dist < minDist) minDist = dist;
                }

                if (minDist <= tolDist) {
                    alphaMatte[idx] = 0.0f;
                } else if (minDist >= tolSoft) {
                    alphaMatte[idx] = 1.0f;
                } else {
                    float a = static_cast<float>((minDist - tolDist) / (tolSoft - tolDist));
                    alphaMatte[idx] = std::max(0.0f, std::min(1.0f, a));
                }
            }
        }
    }

    // 4. Smooth Alpha Transition at Boundary (3x3 Box Blur on edge pixels for anti-aliased hair/clothing)
    std::vector<float> smoothedAlpha = alphaMatte;
    for (int y = 1; y < static_cast<int>(height) - 1; ++y) {
        for (int x = 1; x < static_cast<int>(width) - 1; ++x) {
            int idx = y * width + x;
            float val = alphaMatte[idx];
            // Only smooth transition regions to avoid blurring solid subject interior
            if (val > 0.0f && val < 1.0f) {
                float sum = 0.0f;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        sum += alphaMatte[(y + dy) * width + (x + dx)];
                    }
                }
                smoothedAlpha[idx] = sum / 9.0f;
            }
        }
    }

    // 5. Final Pixel Composition with Color Despill / Decontamination
    for (UINT y = 0; y < height; ++y) {
        const BYTE* srcRow = srcScan0 + y * srcStride;
        BYTE* dstRow = dstScan0 + y * dstStride;

        for (UINT x = 0; x < width; ++x) {
            int idx = y * width + x;
            const BYTE* sp = srcRow + x * 4;
            BYTE* dp = dstRow + x * 4;
            float alpha = smoothedAlpha[idx];

            if (alpha <= 0.001f) {
                // Pure Background
                dp[0] = targetB;
                dp[1] = targetG;
                dp[2] = targetR;
                dp[3] = targetA;
            } else if (alpha >= 0.999f) {
                // Pure Foreground Subject
                dp[0] = sp[0];
                dp[1] = sp[1];
                dp[2] = sp[2];
                dp[3] = sp[3];
            } else {
                // Soft Boundary / Hair / Despill blend
                dp[0] = static_cast<BYTE>(sp[0] * alpha + targetB * (1.0f - alpha) + 0.5f);
                dp[1] = static_cast<BYTE>(sp[1] * alpha + targetG * (1.0f - alpha) + 0.5f);
                dp[2] = static_cast<BYTE>(sp[2] * alpha + targetR * (1.0f - alpha) + 0.5f);
                dp[3] = 255;
            }
        }
    }

    srcBmp->UnlockBits(&srcData);
    resultBmp->UnlockBits(&dstData);
    return resultBmp;
}

Gdiplus::Bitmap* ImageProcessor::GetOrLoadBitmap(const std::wstring& filePath) {
    auto it = m_imageCache.find(filePath);
    if (it != m_imageCache.end()) {
        return it->second.get();
    }

    auto bmp = std::make_unique<Gdiplus::Bitmap>(filePath.c_str());
    if (bmp->GetLastStatus() != Gdiplus::Ok || bmp->GetWidth() == 0 || bmp->GetHeight() == 0) {
        return nullptr;
    }

    // Auto EXIF Orientation Normalization
    UINT propSize = bmp->GetPropertyItemSize(PropertyTagOrientation);
    if (propSize > 0) {
        std::vector<BYTE> buffer(propSize);
        Gdiplus::PropertyItem* prop = reinterpret_cast<Gdiplus::PropertyItem*>(buffer.data());
        if (bmp->GetPropertyItem(PropertyTagOrientation, propSize, prop) == Gdiplus::Ok && prop->value) {
            short orientation = *reinterpret_cast<short*>(prop->value);
            switch (orientation) {
                case 2: bmp->RotateFlip(Gdiplus::RotateNoneFlipX); break;
                case 3: bmp->RotateFlip(Gdiplus::Rotate180FlipNone); break;
                case 4: bmp->RotateFlip(Gdiplus::Rotate180FlipX); break;
                case 5: bmp->RotateFlip(Gdiplus::Rotate90FlipX); break;
                case 6: bmp->RotateFlip(Gdiplus::Rotate90FlipNone); break;
                case 7: bmp->RotateFlip(Gdiplus::Rotate270FlipX); break;
                case 8: bmp->RotateFlip(Gdiplus::Rotate270FlipNone); break;
                default: break;
            }
        }
    }

    // Ensure status is reset after property queries
    bmp->GetWidth();

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
        Gdiplus::Bitmap* bmp = (slot.bgPreset != BgColorPreset::None)
            ? GetOrProcessBgBitmap(slot.sourceFilePath, slot.bgPreset, slot.customBgColor, slot.bgTolerance)
            : GetOrLoadBitmap(slot.sourceFilePath);
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

    if (bmp != nullptr && bmp->GetWidth() > 0 && bmp->GetHeight() > 0) {
        double imgW = bmp->GetWidth();
        double imgH = bmp->GetHeight();

        if (imgW > 0 && imgH > 0) {
            int rot = (slot.rotationAngle % 360 + 360) % 360;
            double effectiveImgW = (rot == 90 || rot == 270) ? imgH : imgW;
            double effectiveImgH = (rot == 90 || rot == 270) ? imgW : imgH;

            bool isSlotLandscape = (slot.widthMm > slot.heightMm);
            bool isImageLandscape = (effectiveImgW > effectiveImgH);
            bool needSlotRotation = (isSlotLandscape != isImageLandscape);

            // Target Aspect Ratio (Visual W / Visual H) for cropping the upright image
            double targetAspect = needSlotRotation
                ? (slot.heightMm / slot.widthMm)
                : (slot.widthMm / slot.heightMm);

            double srcAspect = effectiveImgW / effectiveImgH;

            double cropW, cropH;
            if (srcAspect > targetAspect) {
                // Gambar visual lebih lebar: fit height, crop width
                cropH = effectiveImgH;
                cropW = cropH * targetAspect;
            } else {
                // Gambar visual lebih tinggi: fit width, crop height
                cropW = effectiveImgW;
                cropH = cropW / targetAspect;
            }

            // Terapkan Zoom Level
            double zoom = (slot.zoomLevel > 0.1) ? slot.zoomLevel : 1.0;
            cropW /= zoom;
            cropH /= zoom;

            // Pastikan crop tidak melebihi dimensi gambar visual
            if (cropW > effectiveImgW) cropW = effectiveImgW;
            if (cropH > effectiveImgH) cropH = effectiveImgH;

            // Hitung Crop Center pada koordinat visual
            double centerX = effectiveImgW * slot.cropCenterX;
            double centerY = effectiveImgH * slot.cropCenterY;

            double effCropX = centerX - (cropW / 2.0);
            double effCropY = centerY - (cropH / 2.0);

            // Clamp crop box agar tetap di dalam gambar visual
            if (effCropX < 0) effCropX = 0;
            if (effCropY < 0) effCropY = 0;
            if (effCropX + cropW > effectiveImgW) effCropX = effectiveImgW - cropW;
            if (effCropY + cropH > effectiveImgH) effCropY = effectiveImgH - cropH;

            // Map crop visual ke koordinat bitmap asli & titik destinasi (GDI+ Parallelogram 3-point affine)
            double origSrcX = 0.0, origSrcY = 0.0, origSrcW = 0.0, origSrcH = 0.0;
            Gdiplus::PointF destPoints[3];

            if (rot == 0) {
                origSrcX = effCropX;
                origSrcY = effCropY;
                origSrcW = cropW;
                origSrcH = cropH;

                if (!needSlotRotation) {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY + destH));
                } else {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY));
                }
            } else if (rot == 90) {
                origSrcX = effCropY;
                origSrcY = imgH - (effCropX + cropW);
                origSrcW = cropH;
                origSrcH = cropW;

                if (!needSlotRotation) {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY));
                } else {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY));
                }
            } else if (rot == 180) {
                origSrcX = imgW - (effCropX + cropW);
                origSrcY = imgH - (effCropY + cropH);
                origSrcW = cropW;
                origSrcH = cropH;

                if (!needSlotRotation) {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY));
                } else {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY + destH));
                }
            } else if (rot == 270) {
                origSrcX = imgW - (effCropY + cropH);
                origSrcY = effCropX;
                origSrcW = cropH;
                origSrcH = cropW;

                if (!needSlotRotation) {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY + destH));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY + destH));
                } else {
                    destPoints[0] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY));
                    destPoints[1] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX + destW), static_cast<Gdiplus::REAL>(destY));
                    destPoints[2] = Gdiplus::PointF(static_cast<Gdiplus::REAL>(destX), static_cast<Gdiplus::REAL>(destY + destH));
                }
            }

            Gdiplus::ImageAttributes imgAttr;
            if (slot.isBlackAndWhite) {
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
                destPoints,
                3,
                static_cast<Gdiplus::REAL>(origSrcX),
                static_cast<Gdiplus::REAL>(origSrcY),
                static_cast<Gdiplus::REAL>(origSrcW),
                static_cast<Gdiplus::REAL>(origSrcH),
                Gdiplus::UnitPixel,
                slot.isBlackAndWhite ? &imgAttr : nullptr
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
