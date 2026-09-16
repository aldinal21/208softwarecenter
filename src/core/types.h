#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace SoftwareCenter208 {

// Dimensi standar Pas Foto & Cetak Foto (dalam milimeter / mm)
enum class PhotoSizePreset {
    Size2x3,          // 21.6 x 27.9 mm (2.2 x 2.8 cm - Standar Ijazah/KTP)
    Size3x4,          // 28.0 x 38.0 mm (2.8 x 3.8 cm)
    Size4x6,          // 38.0 x 56.0 mm (3.8 x 5.6 cm)
    SizeVisaUS,       // 50.8 x 50.8 mm (5.1 x 5.1 cm - 2x2 inch Visa US)
    SizeVisaSchengen, // 35.0 x 45.0 mm (3.5 x 4.5 cm - Visa Schengen / Paspor)
    SizeVisaChina,    // 33.0 x 48.0 mm (3.3 x 4.8 cm - Visa China / Umrah)
    SizeWallet,       // 54.0 x 86.0 mm (5.4 x 8.6 cm - Foto Dompet / KTP / ID Card)
    Size2R,           // 60.0 x 90.0 mm (6.0 x 9.0 cm)
    Size3R,           // 88.9 x 127.0 mm (8.9 x 12.7 cm)
    Size4R,           // 101.6 x 152.4 mm (10.2 x 15.2 cm)
    Size5R,           // 127.0 x 177.8 mm (12.7 x 17.8 cm)
    Size6R,           // 152.4 x 203.2 mm (15.2 x 20.3 cm)
    Size8R,           // 203.2 x 254.0 mm (20.3 x 25.4 cm)
    Custom            // Dimensi Kustom dalam cm
};

enum class PackingMode {
    SmartStrip, // ⚡ Smart Strip (Gunting Mudah & Hemat Kertas - DEFAULT)
    EasyCut,    // ✂️ Baris Murni (1 Baris 1 Ukuran Penuh)
    MaxDensity  // 📐 Hemat Maksimal (Pengisian Celah Kosong Bebas)
};

// Preset Pengganti Warna Latar Belakang (Pas Foto Background)
enum class BgColorPreset {
    None,       // Latar Asli (Tanpa Penggantian)
    Red,        // Merah Pas Foto KTP/SKCK (#D32F2F)
    Blue,       // Biru Pas Foto KTP/SKCK (#1976D2)
    White,      // Putih Bersih (#FFFFFF)
    Grey,       // Abu-abu Ijazah (#757575)
    Yellow,     // Kuning BUMN/Kedinasan (#FBC02D)
    Custom      // Warna Kustom Pilihan Pengguna
};

inline uint32_t GetBgPresetArgb(BgColorPreset preset, uint32_t customRgb = 0) {
    switch (preset) {
        case BgColorPreset::Red: return 0xFFD32F2F;    // #D32F2F (Merah Pas Foto)
        case BgColorPreset::Blue: return 0xFF1976D2;   // #1976D2 (Biru Pas Foto)
        case BgColorPreset::White: return 0xFFFFFFFF;  // #FFFFFF (Putih)
        case BgColorPreset::Grey: return 0xFF757575;   // #757575 (Abu-abu)
        case BgColorPreset::Yellow: return 0xFFFBC02D; // #FBC02D (Kuning)
        case BgColorPreset::Custom: return 0xFF000000 | (customRgb & 0x00FFFFFF);
        case BgColorPreset::None:
        default:
            return 0x00000000;
    }
}

inline const wchar_t* GetBgPresetName(BgColorPreset preset) {
    switch (preset) {
        case BgColorPreset::None: return L"Latar Asli";
        case BgColorPreset::Red: return L"Merah";
        case BgColorPreset::Blue: return L"Biru";
        case BgColorPreset::White: return L"Putih";
        case BgColorPreset::Grey: return L"Abu";
        case BgColorPreset::Yellow: return L"Kuning";
        case BgColorPreset::Custom: return L"Kustom";
        default: return L"";
    }
}

struct MillimeterSize {
    double widthMm;
    double heightMm;

    constexpr MillimeterSize(double w = 0.0, double h = 0.0) : widthMm(w), heightMm(h) {}
};

inline MillimeterSize GetPresetDimensionMm(PhotoSizePreset preset) {
    switch (preset) {
        case PhotoSizePreset::Size2x3:          return MillimeterSize(21.6, 27.9);
        case PhotoSizePreset::Size3x4:          return MillimeterSize(28.0, 38.0);
        case PhotoSizePreset::Size4x6:          return MillimeterSize(38.0, 56.0);
        case PhotoSizePreset::SizeVisaUS:       return MillimeterSize(50.8, 50.8);
        case PhotoSizePreset::SizeVisaSchengen: return MillimeterSize(35.0, 45.0);
        case PhotoSizePreset::SizeVisaChina:    return MillimeterSize(33.0, 48.0);
        case PhotoSizePreset::SizeWallet:       return MillimeterSize(54.0, 86.0);
        case PhotoSizePreset::Size2R:           return MillimeterSize(60.0, 90.0);
        case PhotoSizePreset::Size3R:           return MillimeterSize(88.9, 127.0);
        case PhotoSizePreset::Size4R:           return MillimeterSize(101.6, 152.4);
        case PhotoSizePreset::Size5R:           return MillimeterSize(127.0, 177.8);
        case PhotoSizePreset::Size6R:           return MillimeterSize(152.4, 203.2);
        case PhotoSizePreset::Size8R:           return MillimeterSize(203.2, 254.0);
        case PhotoSizePreset::Custom:
        default:
            return MillimeterSize(28.0, 38.0);
    }
}

inline const wchar_t* GetPresetName(PhotoSizePreset preset) {
    switch (preset) {
        case PhotoSizePreset::Size2x3:          return L"2x3 (2.2 x 2.8 cm)";
        case PhotoSizePreset::Size3x4:          return L"3x4 (2.8 x 3.8 cm)";
        case PhotoSizePreset::Size4x6:          return L"4x6 (3.8 x 5.6 cm)";
        case PhotoSizePreset::SizeVisaUS:       return L"Visa US / 2x2\" (5.1 x 5.1 cm)";
        case PhotoSizePreset::SizeVisaSchengen: return L"Visa Schengen / Paspor (3.5 x 4.5 cm)";
        case PhotoSizePreset::SizeVisaChina:    return L"Visa China / Umrah (3.3 x 4.8 cm)";
        case PhotoSizePreset::SizeWallet:       return L"Dompet / KTP / ID Card (5.4 x 8.6 cm)";
        case PhotoSizePreset::Size2R:           return L"2R (6.0 x 9.0 cm)";
        case PhotoSizePreset::Size3R:           return L"3R (8.9 x 12.7 cm)";
        case PhotoSizePreset::Size4R:           return L"4R (10.2 x 15.2 cm)";
        case PhotoSizePreset::Size5R:           return L"5R (12.7 x 17.8 cm)";
        case PhotoSizePreset::Size6R:           return L"6R (15.2 x 20.3 cm)";
        case PhotoSizePreset::Size8R:           return L"8R (20.3 x 25.4 cm)";
        case PhotoSizePreset::Custom:           return L"Custom (cm)";
        default: return L"Unknown";
    }
}

// Preset Ukuran Kertas
enum class PaperSizePreset {
    A4,        // 210 x 297 mm (Default)
    F4_Folio,  // 215 x 330 mm (Folio / F4 Standar Toko Fotokopi)
    A3,        // 297 x 420 mm
    A3Plus,    // 329 x 483 mm (Super A3 / A3+)
    Letter,    // 215.9 x 279.4 mm
    Photo4R    // 101.6 x 152.4 mm (Kertas Foto 4R Satuan)
};

inline MillimeterSize GetPaperPresetDimensionMm(PaperSizePreset preset) {
    switch (preset) {
        case PaperSizePreset::A4:        return MillimeterSize(210.0, 297.0);
        case PaperSizePreset::F4_Folio:  return MillimeterSize(215.0, 330.0);
        case PaperSizePreset::A3:        return MillimeterSize(297.0, 420.0);
        case PaperSizePreset::A3Plus:    return MillimeterSize(329.0, 483.0);
        case PaperSizePreset::Letter:    return MillimeterSize(215.9, 279.4);
        case PaperSizePreset::Photo4R:   return MillimeterSize(101.6, 152.4);
        default:                         return MillimeterSize(210.0, 297.0);
    }
}

inline const wchar_t* GetPaperPresetName(PaperSizePreset preset) {
    switch (preset) {
        case PaperSizePreset::A4:        return L"A4 (21.0 x 29.7 cm)";
        case PaperSizePreset::F4_Folio:  return L"F4 / Folio (21.5 x 33.0 cm)";
        case PaperSizePreset::A3:        return L"A3 (29.7 x 42.0 cm)";
        case PaperSizePreset::A3Plus:    return L"A3+ (32.9 x 48.3 cm)";
        case PaperSizePreset::Letter:    return L"Letter (21.6 x 27.9 cm)";
        case PaperSizePreset::Photo4R:   return L"4R Satuan (10.2 x 15.2 cm)";
        default:                         return L"A4 (21.0 x 29.7 cm)";
    }
}

// Spesifikasi ukuran cetak dinamis per item foto
struct PhotoPrintSize {
    PhotoSizePreset preset = PhotoSizePreset::Size3x4;
    std::wstring label = L"3x4 (2.8 x 3.8 cm)";
    double widthMm = 28.0;
    double heightMm = 38.0;
    int quantity = 1;
    bool isLandscape = false;
};

inline PhotoPrintSize CreateDefaultPrintSize(PhotoSizePreset preset = PhotoSizePreset::Size3x4, bool isLandscape = false) {
    PhotoPrintSize ps;
    ps.preset = preset;
    ps.isLandscape = isLandscape;
    MillimeterSize dim = GetPresetDimensionMm(preset);
    if (isLandscape && dim.widthMm < dim.heightMm) {
        ps.widthMm = dim.heightMm;
        ps.heightMm = dim.widthMm;
    } else {
        ps.widthMm = dim.widthMm;
        ps.heightMm = dim.heightMm;
    }

    std::wstring baseLabel = GetPresetName(preset);
    if (isLandscape && preset != PhotoSizePreset::Custom) {
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
    ps.quantity = 1;
    return ps;
}

// Konfigurasi kertas (default A4 portrait: 210 x 297 mm)
struct PaperConfig {
    PaperSizePreset paperPreset = PaperSizePreset::A4;
    double widthMm = 210.0;
    double heightMm = 297.0;
    double marginMm = 8.0;      // Margin aman cetak printer tepi
    double gapMm = 2.5;         // Jarak antar foto untuk pemotongan
    bool drawCutLines = true;   // Garis panduan potong gunting / cutter
    PackingMode packingMode = PackingMode::SmartStrip; // Default: Smart Strip (Mudah & Hemat)
};

// Item foto pesanan pelanggan (Mendukung multi-item ukuran cetak secara dinamis)
struct PhotoOrderItem {
    uint32_t id = 0;
    std::wstring sourceFilePath;
    std::vector<PhotoPrintSize> printSizes; // Daftar ukuran cetak per foto

    // Framing & cropping parameters (0.0 to 1.0)
    double cropCenterX = 0.5;
    double cropCenterY = 0.45; // Bias sedikit ke atas kepala untuk pas foto
    double zoomLevel = 1.0;
    int rotationAngle = 0;     // 0, 90, 180, 270 derajat

    // Filter & efek warna
    bool isBlackAndWhite = false;
    BgColorPreset bgPreset = BgColorPreset::None;
    uint32_t customBgColor = 0x00FFFFFF;
    int bgTolerance = 30; // Rentang toleransi warna (10-80, default 30)

    int GetTotalQuantity() const {
        int total = 0;
        for (const auto& s : printSizes) total += s.quantity;
        return total;
    }
};

// Hasil penataan foto pada kertas (Layout Result)
struct PlacedPhotoSlot {
    uint32_t orderItemId = 0;
    std::wstring sourceFilePath;
    double xMm = 0.0;
    double yMm = 0.0;
    double widthMm = 0.0;
    double heightMm = 0.0;
    double cropCenterX = 0.5;
    double cropCenterY = 0.45;
    double zoomLevel = 1.0;
    int rotationAngle = 0; // Sudut putar (0, 90, 180, 270)
    bool isBlackAndWhite = false; // Filter Hitam Putih
    BgColorPreset bgPreset = BgColorPreset::None; // Preset Warna Background
    uint32_t customBgColor = 0x00FFFFFF;
    int bgTolerance = 30;
    int pageIndex = 0; // Index halaman (0-based)
    PhotoSizePreset preset = PhotoSizePreset::Size3x4;
};

// Layout untuk 1 lembar halaman kertas
struct PageLayout {
    int pageIndex = 0;                  // Index halaman (0, 1, 2...)
    std::vector<PlacedPhotoSlot> slots; // Foto-foto yang berada di halaman ini
    double usedHeightMm = 0.0;          // Total tinggi terpakai dari atas pada halaman ini
    double remainingHeightMm = 0.0;     // Sisa kertas di bagian bawah pada halaman ini
    int totalPhotosPlaced = 0;          // Jumlah foto di halaman ini
};

struct LayoutResult {
    std::vector<PageLayout> pages;      // Seluruh halaman hasil packing
    std::vector<PlacedPhotoSlot> slots; // Seluruh slot foto (gabungan semua halaman)
    double usedHeightMm = 0.0;          // Total tinggi kertas yang terpakai pada halaman pertama
    double remainingHeightMm = 0.0;     // Sisa kertas di bagian bawah pada halaman pertama
    bool fitsOnSinglePage = true;       // True jika muat hanya dalam 1 lembar A4 (totalPages <= 1)
    int totalPhotosPlaced = 0;          // Total semua foto dari semua halaman
    int totalPages = 1;                 // Total lembar kertas yang dibutuhkan
};

// Konversi unit mm <-> pixels pada DPI tertentu (default print: 300 DPI)
inline int MmToPixels(double mm, double dpi = 300.0) {
    return static_cast<int>((mm / 25.4) * dpi + 0.5);
}

inline double PixelsToMm(int pixels, double dpi = 300.0) {
    return (static_cast<double>(pixels) / dpi) * 25.4;
}

} // namespace SoftwareCenter208
