#include <iostream>
#include <cassert>
#include <cmath>
#include "../src/core/types.h"
#include "../src/core/packing_engine.h"

using namespace SoftwareCenter208;

void TestSinglePhotoPacking() {
    std::cout << "[RUN] TestSinglePhotoPacking...\n";
    PaperConfig paper;
    std::vector<PhotoOrderItem> items;

    PhotoOrderItem item;
    item.id = 1;
    item.sourceFilePath = L"test.jpg";
    item.qty2x3 = 0;
    item.qty3x4 = 2; // 2 pcs 3x4 (28 x 38 mm)
    item.qty4x6 = 0;
    items.push_back(item);

    LayoutResult layout = PackingEngine::CalculateLayout(items, paper);
    assert(layout.slots.size() == 2);
    assert(layout.fitsOnSinglePage == true);
    assert(layout.usedHeightMm > 0.0);
    assert(layout.remainingHeightMm > 0.0);
    // Row height is 38mm, margin is 8mm + 8mm -> usedHeight is ~ 8 + 38 + 8 = 54mm
    std::cout << "  Used Height: " << layout.usedHeightMm << " mm, Remaining: " << layout.remainingHeightMm << " mm\n";
    std::cout << "[PASS] TestSinglePhotoPacking\n";
}

void TestMultiPresetPerPhoto() {
    std::cout << "[RUN] TestMultiPresetPerPhoto (1 foto pesan 2 pcs 2x3, 5 pcs 3x4 dan 5 pcs 4x6)...\n";
    PaperConfig paper;
    std::vector<PhotoOrderItem> items;

    PhotoOrderItem item;
    item.id = 1;
    item.sourceFilePath = L"customer_photo.jpg";
    item.qty2x3 = 2;
    item.qty3x4 = 5;
    item.qty4x6 = 5;
    items.push_back(item);

    // Test with SmartStrip (Default)
    paper.packingMode = PackingMode::SmartStrip;
    LayoutResult layoutSmart = PackingEngine::CalculateLayout(items, paper);
    assert(layoutSmart.slots.size() == 12);
    assert(layoutSmart.fitsOnSinglePage == true);
    std::cout << "  [SmartStrip] Total placed: " << layoutSmart.totalPhotosPlaced
              << ", Used Height: " << layoutSmart.usedHeightMm << " mm (Leaves scrap: "
              << layoutSmart.remainingHeightMm << " mm)\n";

    // Test with EasyCut
    paper.packingMode = PackingMode::EasyCut;
    LayoutResult layoutEasy = PackingEngine::CalculateLayout(items, paper);
    assert(layoutEasy.slots.size() == 12);
    assert(layoutEasy.fitsOnSinglePage == true);
    std::cout << "  [EasyCut] Total placed: " << layoutEasy.totalPhotosPlaced
              << ", Used Height: " << layoutEasy.usedHeightMm << " mm\n";

    // Test with MaxDensity
    paper.packingMode = PackingMode::MaxDensity;
    LayoutResult layoutDensity = PackingEngine::CalculateLayout(items, paper);
    assert(layoutDensity.slots.size() == 12);
    assert(layoutDensity.fitsOnSinglePage == true);
    std::cout << "  [MaxDensity] Total placed: " << layoutDensity.totalPhotosPlaced
              << ", Used Height: " << layoutDensity.usedHeightMm << " mm (Leaves scrap: "
              << layoutDensity.remainingHeightMm << " mm)\n";

    // SmartStrip should be more compact than or equal to pure EasyCut
    assert(layoutSmart.usedHeightMm <= layoutEasy.usedHeightMm);
    std::cout << "[PASS] TestMultiPresetPerPhoto\n";
}

void TestMixedPresesDensity() {
    std::cout << "[RUN] TestMixedPresesDensity...\n";
    PaperConfig paper;
    std::vector<PhotoOrderItem> items;

    // 4 pcs 4x6 (38 x 56 mm)
    PhotoOrderItem item1;
    item1.id = 1;
    item1.sourceFilePath = L"photo1.jpg";
    item1.qty4x6 = 4;
    items.push_back(item1);

    // 6 pcs 3x4 (28 x 38 mm)
    PhotoOrderItem item2;
    item2.id = 2;
    item2.sourceFilePath = L"photo2.jpg";
    item2.qty3x4 = 6;
    items.push_back(item2);

    // 8 pcs 2x3 (21.6 x 27.9 mm)
    PhotoOrderItem item3;
    item3.id = 3;
    item3.sourceFilePath = L"photo3.jpg";
    item3.qty2x3 = 8;
    items.push_back(item3);

    LayoutResult layout = PackingEngine::CalculateLayout(items, paper);
    assert(layout.slots.size() == 18);
    assert(layout.fitsOnSinglePage == true);
    assert(layout.usedHeightMm < paper.heightMm);
    assert(layout.remainingHeightMm > 50.0); // Should leave substantial scrap at the bottom

    std::cout << "  Total photos placed: " << layout.totalPhotosPlaced << "\n";
    std::cout << "  Used Height: " << layout.usedHeightMm << " mm\n";
    std::cout << "  Remaining Height for Scrap: " << layout.remainingHeightMm << " mm\n";
    std::cout << "[PASS] TestMixedPresesDensity\n";
}

void TestOverflowAndMultiPagePagination() {
    std::cout << "[RUN] TestOverflowAndMultiPagePagination...\n";
    PaperConfig paper;
    std::vector<PhotoOrderItem> items;

    // 100 pcs 4x6 (pasti butuh banyak halaman A4)
    PhotoOrderItem item;
    item.id = 1;
    item.sourceFilePath = L"overflow.jpg";
    item.qty4x6 = 100;
    items.push_back(item);

    LayoutResult layout = PackingEngine::CalculateLayout(items, paper);
    assert(layout.fitsOnSinglePage == false);
    assert(layout.totalPages > 1);
    assert(layout.pages.size() == static_cast<size_t>(layout.totalPages));
    assert(layout.totalPhotosPlaced == 100);

    // Verifikasi akumulasi slot di semua halaman sama dengan total slots
    size_t accumulatedSlots = 0;
    for (size_t i = 0; i < layout.pages.size(); ++i) {
        accumulatedSlots += layout.pages[i].slots.size();
        assert(layout.pages[i].pageIndex == static_cast<int>(i));
        assert(layout.pages[i].usedHeightMm <= paper.heightMm);
        for (const auto& slot : layout.pages[i].slots) {
            assert(slot.pageIndex == static_cast<int>(i));
        }
    }
    assert(accumulatedSlots == 100);

    std::cout << "  Multi-page pagination success: 100 photos 4x6 distributed across "
              << layout.totalPages << " A4 pages.\n";
    std::cout << "[PASS] TestOverflowAndMultiPagePagination\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  208 Software Center - Packing Tests   \n";
    std::cout << "========================================\n";
    TestSinglePhotoPacking();
    TestMultiPresetPerPhoto();
    TestMixedPresesDensity();
    TestOverflowAndMultiPagePagination();
    std::cout << "========================================\n";
    std::cout << "  ALL PACKING ENGINE TESTS PASSED!      \n";
    std::cout << "========================================\n";
    return 0;
}
