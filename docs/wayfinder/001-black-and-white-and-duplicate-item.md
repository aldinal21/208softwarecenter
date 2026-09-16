# Ticket: Black & White Filter and Duplicate Order Item

**Label**: `wayfinder:prototype`
**Parent**: [Map](map.md)
**Status**: `frontier`

## Question
Bagaimana mengintegrasikan filter Black & White (B&W / Grayscale) dan fitur "Duplikat Foto" ke dalam alur kerja antarmuka utama, dialog edit foto, mesin render GDI+, spooling cetak printer, serta PDF/Image exporter secara mulus dan konsisten?

## Scope & Implementation Details
1. **Core Data Structure**:
   - Tambahkan `bool isBlackAndWhite = false` pada `PhotoOrderItem` dan `PlacedPhotoSlot`.
2. **UI Controls**:
   - Tambahkan tombol "📋 Duplikat Foto" di panel antrean jendela utama (`IDC_BTN_DUPLICATE`).
   - Tambahkan checkbox/toggle "Hitam Putih (B&W)" di panel kontrol samping jendela utama dan di dalam `EditPhotoDialog`.
3. **Rendering & Export**:
   - Implementasikan ColorMatrix Grayscale GDI+ (`Gdiplus::ColorMatrix` dengan konstanta NTSC `0.299 R + 0.587 G + 0.114 B`) pada `ImageProcessor::DrawPhotoSlot`.
   - Pastikan efek B&W otomatis ter-render pada Live Preview Canvas, Modal Print Preview, Direct Printer Spooler, PDF 1.4 exporter (Grayscale/JPEG stream), dan High-Res PNG/JPEG exporter.
