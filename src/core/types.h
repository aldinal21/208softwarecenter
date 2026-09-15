#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace SoftwareCenter208 {

// Dimensi standar Pas Foto Indonesia (dalam milimeter / mm)
enum class PhotoSizePreset {
    Size2x3, // 21.6 x 27.9 mm (asli standar KTP/Ijazah 2x3)
    Size3x4, // 28.0 x 38.0 mm
    Size4x6, // 38.0 x 56.0 mm
    Custom
};

enum class PackingMode {
    SmartStrip, // ⚡ Smart Strip (Gunting Mudah & Hemat Kertas - DEFAULT)
    EasyCut,    // ✂️ Baris Murni (1 Baris 1 Ukuran Penuh)
    MaxDensity  // 📐 Hemat Maksimal (Pengisian Celah Kosong Bebas)
};

struct MillimeterSize {
    double widthMm;
    double heightMm;

    constexpr MillimeterSize(double w = 0.0, double h = 0.0) : widthMm(w), heightMm(h) {}
};

inline MillimeterSize GetPresetDimensionMm(PhotoSizePreset preset) {
    switch (preset) {
        case PhotoSizePreset::Size2x3: return MillimeterSize(21.6, 27.9);
        case PhotoSizePreset::Size3x4: return MillimeterSize(28.0, 38.0);
        case PhotoSizePreset::Size4x6: return MillimeterSize(38.0, 56.0);
        case PhotoSizePreset::Custom:
        default:
            return MillimeterSize(28.0, 38.0);
    }
}

inline const wchar_t* GetPresetName(PhotoSizePreset preset) {
    switch (preset) {
        case PhotoSizePreset::Size2x3: return L"2x3 (2.2 x 2.8 cm)";
        case PhotoSizePreset::Size3x4: return L"3x4 (2.8 x 3.8 cm)";
        case PhotoSizePreset::Size4x6: return L"4x6 (3.8 x 5.6 cm)";
        case PhotoSizePreset::Custom: return L"Custom";
        default: return L"Unknown";
    }
}

// Konfigurasi kertas (default A4 portrait: 210 x 297 mm)
struct PaperConfig {
    double widthMm = 210.0;
    double heightMm = 297.0;
    double marginMm = 8.0;      // Margin aman cetak printer tepi
    double gapMm = 2.5;         // Jarak antar foto untuk pemotongan
    bool drawCutLines = true;   // Garis panduan potong gunting / cutter
    PackingMode packingMode = PackingMode::SmartStrip; // Default: Smart Strip (Mudah & Hemat)
};

// Item foto pesanan pelanggan (Mendukung jumlah berbeda per ukuran secara simultan)
struct PhotoOrderItem {
    uint32_t id = 0;
    std::wstring sourceFilePath;
    int qty2x3 = 0;
    int qty3x4 = 0;
    int qty4x6 = 0;

    // Framing & cropping parameters (0.0 to 1.0)
    double cropCenterX = 0.5;
    double cropCenterY = 0.45; // Bias sedikit ke atas kepala untuk pas foto
    double zoomLevel = 1.0;

    int GetTotalQuantity() const {
        return qty2x3 + qty3x4 + qty4x6;
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
    int pageIndex = 0; // Index halaman (0-based)
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
