#include "packing_engine.h"
#include <algorithm>
#include <cmath>

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
        int rotationAngle;
        bool isBlackAndWhite;
        BgColorPreset bgPreset;
        uint32_t customBgColor;
        int bgTolerance;
        PhotoSizePreset preset;
    };

    std::vector<InstanceToPack> instances;
    for (const auto& item : items) {
        for (const auto& sizeItem : item.printSizes) {
            if (sizeItem.quantity <= 0) continue;
            for (int q = 0; q < sizeItem.quantity; ++q) {
                InstanceToPack inst;
                inst.orderItemId = item.id;
                inst.filePath = item.sourceFilePath;
                inst.widthMm = sizeItem.widthMm;
                inst.heightMm = sizeItem.heightMm;
                inst.cropCenterX = item.cropCenterX;
                inst.cropCenterY = item.cropCenterY;
                inst.zoomLevel = item.zoomLevel;
                inst.rotationAngle = item.rotationAngle;
                inst.isBlackAndWhite = item.isBlackAndWhite;
                inst.bgPreset = item.bgPreset;
                inst.customBgColor = item.customBgColor;
                inst.bgTolerance = item.bgTolerance;
                inst.preset = sizeItem.preset;
                instances.push_back(inst);
            }
        }
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
        slot.rotationAngle = inst.rotationAngle;
        slot.isBlackAndWhite = inst.isBlackAndWhite;
        slot.bgPreset = inst.bgPreset;
        slot.customBgColor = inst.customBgColor;
        slot.bgTolerance = inst.bgTolerance;
        slot.pageIndex = currentPageIndex;
        slot.preset = inst.preset;

        pages[currentPageIndex].slots.push_back(slot);
        pages[currentPageIndex].totalPhotosPlaced++;
        result.slots.push_back(slot);
        result.totalPhotosPlaced++;

        double bottom = y + inst.heightMm;
        if (bottom > pageMaxBottomY) {
            pageMaxBottomY = bottom;
        }
    };

    const double maxRightEdge = paper.widthMm - paper.marginMm;

    if (paper.packingMode == PackingMode::SmartStrip) {
        // Mode 1: SmartStrip (Gunting Mudah & Hemat Kertas - DEFAULT)
        // Partisi guillotine strip cerdas: menata foto dalam strip horizontal guillotine,
        // namun memanfaatkan sisa lebar strip untuk ukuran yang kompatibel (height <= stripHeight).
        // 100% potongan lurus dari ujung ke ujung tanpa sudut L-shape.

        // Sort items by height descending
        std::vector<InstanceToPack> remaining = instances;
        std::sort(remaining.begin(), remaining.end(), [](const InstanceToPack& a, const InstanceToPack& b) {
            if (std::abs(a.heightMm - b.heightMm) > 0.01) return a.heightMm > b.heightMm;
            return a.widthMm > b.widthMm;
        });

        std::vector<bool> packed(remaining.size(), false);
        size_t packedCount = 0;
        double currentY = paper.marginMm;

        while (packedCount < remaining.size()) {
            // Find the highest unpacked item to define the new strip height
            size_t leadIdx = remaining.size();
            for (size_t i = 0; i < remaining.size(); ++i) {
                if (!packed[i]) {
                    leadIdx = i;
                    break;
                }
            }
            if (leadIdx >= remaining.size()) break;

            double stripHeight = remaining[leadIdx].heightMm;

            if (currentY + stripHeight > paper.heightMm - paper.marginMm + 0.001) {
                startNewPage();
                currentY = paper.marginMm;
            }

            double currentX = paper.marginMm;

            // 1. Pack items of the exact same or near height first
            for (size_t i = 0; i < remaining.size(); ++i) {
                if (!packed[i] && std::abs(remaining[i].heightMm - stripHeight) <= 0.5) {
                    if (currentX + remaining[i].widthMm <= maxRightEdge + 0.001) {
                        placeSlot(remaining[i], currentX, currentY);
                        currentX += remaining[i].widthMm + paper.gapMm;
                        packed[i] = true;
                        packedCount++;
                    }
                }
            }

            // 2. Fill remaining horizontal space in this strip with smaller items (height <= stripHeight)
            for (size_t i = 0; i < remaining.size(); ++i) {
                if (!packed[i] && (remaining[i].heightMm <= stripHeight + 0.001)) {
                    if (currentX + remaining[i].widthMm <= maxRightEdge + 0.001) {
                        placeSlot(remaining[i], currentX, currentY);
                        currentX += remaining[i].widthMm + paper.gapMm;
                        packed[i] = true;
                        packedCount++;
                    }
                }
            }

            currentY += stripHeight + paper.gapMm;
        }
    } else if (paper.packingMode == PackingMode::EasyCut) {
        // Mode 2: EasyCut (Baris Murni - 1 Baris Hanya 1 Ukuran Sama)
        // Dikelompokkan ketat per ukuran/dimensi
        std::vector<InstanceToPack> remaining = instances;
        std::sort(remaining.begin(), remaining.end(), [](const InstanceToPack& a, const InstanceToPack& b) {
            if (std::abs(a.heightMm - b.heightMm) > 0.01) return a.heightMm > b.heightMm;
            return a.widthMm > b.widthMm;
        });

        std::vector<bool> packed(remaining.size(), false);
        size_t packedCount = 0;
        double currentY = paper.marginMm;

        while (packedCount < remaining.size()) {
            size_t leadIdx = remaining.size();
            for (size_t i = 0; i < remaining.size(); ++i) {
                if (!packed[i]) {
                    leadIdx = i;
                    break;
                }
            }
            if (leadIdx >= remaining.size()) break;

            double rowHeight = remaining[leadIdx].heightMm;
            double rowWidth = remaining[leadIdx].widthMm;

            if (currentY + rowHeight > paper.heightMm - paper.marginMm + 0.001) {
                startNewPage();
                currentY = paper.marginMm;
            }

            double currentX = paper.marginMm;

            for (size_t i = 0; i < remaining.size(); ++i) {
                if (!packed[i] &&
                    std::abs(remaining[i].heightMm - rowHeight) <= 0.1 &&
                    std::abs(remaining[i].widthMm - rowWidth) <= 0.1) {
                    if (currentX + remaining[i].widthMm <= maxRightEdge + 0.001) {
                        placeSlot(remaining[i], currentX, currentY);
                        currentX += remaining[i].widthMm + paper.gapMm;
                        packed[i] = true;
                        packedCount++;
                    }
                }
            }

            currentY += rowHeight + paper.gapMm;
        }
    } else {
        // Mode 3: MaxDensity (Hemat Maksimal - FFDH Shelf Packing)
        std::vector<InstanceToPack> sortedInstances = instances;
        std::sort(sortedInstances.begin(), sortedInstances.end(), [](const InstanceToPack& a, const InstanceToPack& b) {
            if (std::abs(a.heightMm - b.heightMm) > 0.01) return a.heightMm > b.heightMm;
            return a.widthMm > b.widthMm;
        });

        struct Shelf {
            double y;
            double height;
            double currentX;
        };

        std::vector<Shelf> shelves;

        for (const auto& inst : sortedInstances) {
            bool placed = false;
            size_t bestShelfIdx = (size_t)-1;
            double minRemainingWidth = 999999.0;

            for (size_t s = 0; s < shelves.size(); ++s) {
                if (shelves[s].height >= inst.heightMm - 0.001) {
                    double spaceLeft = (paper.widthMm - paper.marginMm) - shelves[s].currentX;
                    if (spaceLeft >= inst.widthMm - 0.001) {
                        double rem = spaceLeft - inst.widthMm;
                        if (rem < minRemainingWidth) {
                            minRemainingWidth = rem;
                            bestShelfIdx = s;
                        }
                    }
                }
            }

            if (bestShelfIdx != (size_t)-1) {
                Shelf& shelf = shelves[bestShelfIdx];
                placeSlot(inst, shelf.currentX, shelf.y);
                shelf.currentX += inst.widthMm + paper.gapMm;
            } else {
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
