# Ticket: Photo Retouch & Lighting Enhancement (Brightness, Contrast, Auto-Enhance)

**Label**: `wayfinder:prototype`
**Parent**: [Map](map.md)
**Status**: `frontier`

## Question
Bagaimana mengimplementasikan pengaturan pencahayaan foto (Kecerahan, Kontras, dan 1-Click Auto Enhance) pada `EditPhotoDialog` dan `ImageProcessor` menggunakan transformasi GDI+ ColorMatrix yang cepat, ringan, dan zero-allocation?

## Scope & Implementation Details
1. **Core Data Structure**:
   - Tambahkan `double brightness = 0.0` (-0.5 s/d +0.5) dan `double contrast = 1.0` (0.5 s/d 1.5) pada `PhotoOrderItem` & `PlacedPhotoSlot`.
2. **UI Controls**:
   - Tambahkan slider Brightness & Contrast di `EditPhotoDialog`.
   - Tambahkan tombol "✨ Auto Enhance" yang menghitung kecerahan rata-rata dan menyesuaikan kontras secara otomatis.
   - Tambahkan tombol "Reset Retouch".
3. **Rendering**:
   - Integrasikan ke `ImageProcessor::DrawPhotoSlot` via `Gdiplus::ImageAttributes` ColorMatrix.
