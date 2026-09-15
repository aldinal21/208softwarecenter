#include "packing_engine.h"
#include <algorithm>

namespace SoftwareCenter208 {

LayoutResult PackingEngine::CalculateLayout(const std::vector<PhotoOrderItem>& items, const PaperConfig& paper) {
    LayoutResult result;
    result.fitsOnSinglePage = true;
    result.totalPhotosPlaced = 0;

    const double printableWidth = paper.widthMm - (2.0 * paper.marginMm);
    const double printableHeight = paper.heightMm - (2.0 * paper.marginMm);

    if (printableWidth <= 0 || printableHeight <= 0 || items.empty()) {
        result.usedHeightMm = 0.0;
        result.remainingHeightMm = paper.heightMm;
        return result;
    }

    // Flatten order items into individual instances to pack
    struct InstanceToPack {
        uint32_t orderItemId;
        std::wstring filePath;
        double widthMm;
        double heightMm;
        double cropCenterX;
        double cropCenterY;
        double zoomLevel;
        PhotoSizePreset preset;
    };

    std::vector<InstanceToPack> instances;
    for (const auto& item : items) {
        auto addInstances = [&](PhotoSizePreset preset, int qty) {
            if (qty <= 0) return;
            MillimeterSize size = GetPresetDimensionMm(preset);
            for (int q = 0; q < qty; ++q) {
                InstanceToPack inst;
                inst.orderItemId = item.id;
                inst.filePath = item.sourceFilePath;
                inst.widthMm = size.widthMm;
                inst.heightMm = size.heightMm;
                inst.cropCenterX = item.cropCenterX;
                inst.cropCenterY = item.cropCenterY;
                inst.zoomLevel = item.zoomLevel;
                inst.preset = preset;
                instances.push_back(inst);
            }
        };

        addInstances(PhotoSizePreset::Size4x6, item.qty4x6);
        addInstances(PhotoSizePreset::Size3x4, item.qty3x4);
        addInstances(PhotoSizePreset::Size2x3, item.qty2x3);
    }

    if (instances.empty()) {
        PageLayout emptyPage;
        emptyPage.pageIndex = 0;
        emptyPage.usedHeightMm = 0.0;
        emptyPage.remainingHeightMm = paper.heightMm;
        emptyPage.totalPhotosPlaced = 0;
        result.pages.push_back(emptyPage);
        result.totalPages = 1;
        result.fitsOnSinglePage = true;
        result.usedHeightMm = 0.0;
        result.remainingHeightMm = paper.heightMm;
        return result;
    }

    int currentPageIndex = 0;
    std::vector<PageLayout> pages;
    pages.push_back(PageLayout{});
    pages[0].pageIndex = 0;

    double pageMaxBottomY = paper.marginMm;

    auto startNewPage = [&]() {
        pages[currentPageIndex].usedHeightMm = (pages[currentPageIndex].slots.empty() ? 0.0 : pageMaxBottomY + paper.marginMm);
        if (pages[currentPageIndex].usedHeightMm > paper.heightMm) {
            pages[currentPageIndex].usedHeightMm = paper.heightMm;
        }
        pages[currentPageIndex].remainingHeightMm = paper.heightMm - pages[currentPageIndex].usedHeightMm;
        if (pages[currentPageIndex].remainingHeightMm < 0.0) {
            pages[currentPageIndex].remainingHeightMm = 0.0;
        }

        currentPageIndex++;
        PageLayout newPage;
        newPage.pageIndex = currentPageIndex;
        pages.push_back(newPage);
        pageMaxBottomY = paper.marginMm;
    };

    auto placeSlot = [&](const InstanceToPack& inst, double x, double y) {
        PlacedPhotoSlot slot;
        slot.orderItemId = inst.orderItemId;
        slot.sourceFilePath = inst.filePath;
        slot.xMm = x;
        slot.yMm = y;
        slot.widthMm = inst.widthMm;
        slot.heightMm = inst.heightMm;
        slot.cropCenterX = inst.cropCenterX;
        slot.cropCenterY = inst.cropCenterY;
        slot.zoomLevel = inst.zoomLevel;
        slot.pageIndex = currentPageIndex;

        pages[currentPageIndex].slots.push_back(slot);
        pages[currentPageIndex].totalPhotosPlaced++;
        result.slots.push_back(slot);
        result.totalPhotosPlaced++;

        double bottom = y + inst.heightMm;
        if (bottom > pageMaxBottomY) {
            pageMaxBottomY = bottom;
        }
    };

    if (paper.packingMode == PackingMode::SmartStrip) {
        // Mode 1: SmartStrip (Gunting Mudah & Hemat Kertas - DEFAULT)
        // Partisi guillotine strip cerdas: menata foto dalam strip horizontal guillotine,
        // namun memanfaatkan sisa lebar strip untuk ukuran yang kompatibel (misal: 2x3 & 3x4 mengisi celah kanan 4x6).
        // 100% potongan lurus dari ujung ke ujung tanpa sudut L-shape.

        std::vector<InstanceToPack> list4x6;
        std::vector<InstanceToPack> list3x4;
        std::vector<InstanceToPack> list2x3;
        std::vector<InstanceToPack> listCustom;

        for (const auto& inst : instances) {
            if (inst.preset == PhotoSizePreset::Size4x6) list4x6.push_back(inst);
            else if (inst.preset == PhotoSizePreset::Size3x4) list3x4.push_back(inst);
            else if (inst.preset == PhotoSizePreset::Size2x3) list2x3.push_back(inst);
            else listCustom.push_back(inst);
        }

        double currentY = paper.marginMm;
        const double maxRightEdge = paper.widthMm - paper.marginMm;

        size_t i4x6 = 0;
        size_t i3x4 = 0;
        size_t i2x3 = 0;

        const MillimeterSize dim4x6 = GetPresetDimensionMm(PhotoSizePreset::Size4x6);
        const MillimeterSize dim3x4 = GetPresetDimensionMm(PhotoSizePreset::Size3x4);
        const MillimeterSize dim2x3 = GetPresetDimensionMm(PhotoSizePreset::Size2x3);

        // 1. Process 4x6 strips (Strip Height 56.0 mm - lurus & rata penuh)
        while (i4x6 < list4x6.size()) {
            if (currentY + dim4x6.heightMm > paper.heightMm - paper.marginMm + 0.001) {
                startNewPage();
                currentY = paper.marginMm;
            }

            double currentX = paper.marginMm;

            // Isi 4x6 sebanyak yang muat pada baris ini
            while (i4x6 < list4x6.size() && (currentX + dim4x6.widthMm <= maxRightEdge + 0.001)) {
                placeSlot(list4x6[i4x6++], currentX, currentY);
                currentX += dim4x6.widthMm + paper.gapMm;
            }

            // Manfaatkan sisa lebar strip 56mm:
            // a) Foto 3x4 (tinggi 38.0mm <= 56.0mm)
            while (i3x4 < list3x4.size() && (currentX + dim3x4.widthMm <= maxRightEdge + 0.001)) {
                placeSlot(list3x4[i3x4++], currentX, currentY);
                currentX += dim3x4.widthMm + paper.gapMm;
            }

            // b) Sisa lebar diisi 2x3 single (tinggi 27.9mm <= 56.0mm)
            while (i2x3 < list2x3.size() && (currentX + dim2x3.widthMm <= maxRightEdge + 0.001)) {
                placeSlot(list2x3[i2x3++], currentX, currentY);
                currentX += dim2x3.widthMm + paper.gapMm;
            }

            currentY += dim4x6.heightMm + paper.gapMm;
        }

        // 2. Process 3x4 strips (Strip Height 38.0 mm)
        while (i3x4 < list3x4.size()) {
            if (currentY + dim3x4.heightMm > paper.heightMm - paper.marginMm + 0.001) {
                startNewPage();
                currentY = paper.marginMm;
            }

            double currentX = paper.marginMm;

            // Isi 3x4 sebanyak yang muat pada baris ini
            while (i3x4 < list3x4.size() && (currentX + dim3x4.widthMm <= maxRightEdge + 0.001)) {
                placeSlot(list3x4[i3x4++], currentX, currentY);
                currentX += dim3x4.widthMm + paper.gapMm;
            }

            // Manfaatkan sisa lebar strip 38mm untuk 2x3 (tinggi 27.9mm <= 38.0mm)
            while (i2x3 < list2x3.size() && (currentX + dim2x3.widthMm <= maxRightEdge + 0.001)) {
                placeSlot(list2x3[i2x3++], currentX, currentY);
                currentX += dim2x3.widthMm + paper.gapMm;
            }

            currentY += dim3x4.heightMm + paper.gapMm;
        }

        // 3. Process 2x3 strips (Strip Height 27.9 mm)
        while (i2x3 < list2x3.size()) {
            if (currentY + dim2x3.heightMm > paper.heightMm - paper.marginMm + 0.001) {
                startNewPage();
                currentY = paper.marginMm;
            }

            double currentX = paper.marginMm;

            while (i2x3 < list2x3.size() && (currentX + dim2x3.widthMm <= maxRightEdge + 0.001)) {
                placeSlot(list2x3[i2x3++], currentX, currentY);
                currentX += dim2x3.widthMm + paper.gapMm;
            }

            currentY += dim2x3.heightMm + paper.gapMm;
        }

        // 4. Custom sizes
        for (const auto& inst : listCustom) {
            if (currentY + inst.heightMm > paper.heightMm - paper.marginMm + 0.001) {
                startNewPage();
                currentY = paper.marginMm;
            }
            placeSlot(inst, paper.marginMm, currentY);
            currentY += inst.heightMm + paper.gapMm;
        }
    } else if (paper.packingMode == PackingMode::EasyCut) {
        // Mode 2: EasyCut (Baris Murni per Ukuran)
        // Urutkan berdasarkan preset (4x6 -> 3x4 -> 2x3) agar potongan horizontal lurus penuh
        std::stable_sort(instances.begin(), instances.end(), [](const InstanceToPack& a, const InstanceToPack& b) {
            if (a.heightMm != b.heightMm) {
                return a.heightMm > b.heightMm;
            }
            return a.widthMm > b.widthMm;
        });

        double currentX = paper.marginMm;
        double currentY = paper.marginMm;
        double currentRowHeight = 0.0;
        PhotoSizePreset currentPreset = instances.front().preset;

        for (const auto& inst : instances) {
            // Jika ukuran preset berubah dan baris sebelumnya tidak kosong, buat baris baru
            bool presetChanged = (inst.preset != currentPreset && currentX > paper.marginMm);
            bool rowFull = (currentX + inst.widthMm > paper.widthMm - paper.marginMm + 0.001 && currentX > paper.marginMm);

            if (presetChanged || rowFull) {
                currentX = paper.marginMm;
                currentY += currentRowHeight + paper.gapMm;
                currentRowHeight = 0.0;
                currentPreset = inst.preset;
            }

            if (currentY + inst.heightMm > paper.heightMm - paper.marginMm + 0.001) {
                startNewPage();
                currentX = paper.marginMm;
                currentY = paper.marginMm;
                currentRowHeight = 0.0;
                currentPreset = inst.preset;
            }

            placeSlot(inst, currentX, currentY);
            currentX += inst.widthMm + paper.gapMm;
            if (inst.heightMm > currentRowHeight) {
                currentRowHeight = inst.heightMm;
            }
        }
    } else {
        // Mode 3: MaxDensity (Efisiensi Kertas Maksimal - Best-Fit Shelf Gap Filling)
        std::stable_sort(instances.begin(), instances.end(), [](const InstanceToPack& a, const InstanceToPack& b) {
            if (a.heightMm != b.heightMm) {
                return a.heightMm > b.heightMm;
            }
            return a.widthMm > b.widthMm;
        });

        struct Shelf {
            double y;
            double height;
            double currentX;
        };

        std::vector<Shelf> shelves;

        for (const auto& inst : instances) {
            int bestShelfIdx = -1;
            double minRemainingSpace = 999999.0;

            // Cari shelf yang masih cukup menampung lebar dan tingginya
            for (size_t i = 0; i < shelves.size(); ++i) {
                if (inst.heightMm <= shelves[i].height) {
                    double remainingW = (paper.widthMm - paper.marginMm) - shelves[i].currentX;
                    if (inst.widthMm <= remainingW + 0.001) {
                        double spaceAfter = remainingW - inst.widthMm;
                        if (spaceAfter < minRemainingSpace) {
                            minRemainingSpace = spaceAfter;
                            bestShelfIdx = static_cast<int>(i);
                        }
                    }
                }
            }

            if (bestShelfIdx >= 0) {
                // Masukkan ke shelf yang sudah ada
                Shelf& shelf = shelves[bestShelfIdx];
                placeSlot(inst, shelf.currentX, shelf.y);
                shelf.currentX += inst.widthMm + paper.gapMm;
            } else {
                // Buat shelf baru
                double newShelfY = paper.marginMm;
                if (!shelves.empty()) {
                    newShelfY = shelves.back().y + shelves.back().height + paper.gapMm;
                }

                if (newShelfY + inst.heightMm > paper.heightMm - paper.marginMm + 0.001) {
                    startNewPage();
                    shelves.clear();
                    newShelfY = paper.marginMm;
                }

                Shelf newShelf;
                newShelf.y = newShelfY;
                newShelf.height = inst.heightMm;
                newShelf.currentX = paper.marginMm + inst.widthMm + paper.gapMm;

                placeSlot(inst, paper.marginMm, newShelfY);
                shelves.push_back(newShelf);
            }
        }
    }

    // Finalize last page
    pages[currentPageIndex].usedHeightMm = (pages[currentPageIndex].slots.empty() ? 0.0 : pageMaxBottomY + paper.marginMm);
    if (pages[currentPageIndex].usedHeightMm > paper.heightMm) {
        pages[currentPageIndex].usedHeightMm = paper.heightMm;
    }
    pages[currentPageIndex].remainingHeightMm = paper.heightMm - pages[currentPageIndex].usedHeightMm;
    if (pages[currentPageIndex].remainingHeightMm < 0.0) {
        pages[currentPageIndex].remainingHeightMm = 0.0;
    }

    result.pages = std::move(pages);
    result.totalPages = static_cast<int>(result.pages.size());
    result.fitsOnSinglePage = (result.totalPages <= 1);
    result.usedHeightMm = result.pages.empty() ? 0.0 : result.pages[0].usedHeightMm;
    result.remainingHeightMm = result.pages.empty() ? paper.heightMm : result.pages[0].remainingHeightMm;

    return result;
}

} // namespace SoftwareCenter208
