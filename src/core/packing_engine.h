#pragma once

#include "types.h"
#include <vector>

namespace SoftwareCenter208 {

class PackingEngine {
public:
    // Menata daftar pesanan foto ke dalam layout kertas A4 (top-aligned, paper saving)
    static LayoutResult CalculateLayout(const std::vector<PhotoOrderItem>& items, const PaperConfig& paper);

private:
    struct RectNode {
        double x, y, w, h;
    };
};

} // namespace SoftwareCenter208
