# ADR 0001: Tech Stack & Architecture for Windows XP to Windows 11 Compatibility

## Status
Accepted

## Context
Aplikasi "208 Software Center" dirancang khusus untuk toko fotokopi 208, dengan fungsi awal (MVP) otomatisasi pas foto (Drag & Drop, Framing, Top-Aligned Paper Saving Layout, Direct Print, & Export).
Aplikasi harus berjalan secara native, standalone (single binary/portable), tanpa dependency runtime (.NET modern, WebView2, VC++ Redistributable berat) di rentang sistem operasi dari **Windows XP SP3 hingga Windows 11**.

## Decision
1. **Bahasa & Toolchain**: C/C++ dengan Win32 API murni + GDI/GDI+ (dicompile dengan MinGW-w64 GCC target `i686` 32-bit dengan flag `-Wl,--subsystem,windows:5.1`).
2. **UI Framework**: Native Win32 Controls + Custom Double-Buffered GDI/GDI+ Canvas untuk Drag & Drop dan Image Framing interaktif.
3. **Image & Print Processing**: GDI+ (tersedia native di Windows XP SP1 ke atas hingga Windows 11) untuk render resolusi tinggi (300 DPI) dan direct printing via `winspool.drv` / standard Win32 print dialog.
4. **Zero External Runtime**: Static linking untuk runtime standard library (`-static -static-libgcc -static-libstdc++`) sehingga menghasilkan single portable `.exe` (< 2MB).

## Consequences
- **Positif**:
  - Berjalan 100% instan di PC kasir Windows XP, Windows 7, Windows 10, hingga Windows 11 tanpa setup installer atau runtime tambahan.
  - Startup instan (< 100ms) dan penggunaan memori RAM sangat kecil (< 15MB).
  - Native Drag & Drop (`WM_DROPFILES` / OLE) dan native GDI printing terbukti kompatibel dengan semua jenis printer lama dan baru (Epson L-series, Canon G-series, HP Deskjet, dll).
- **Konsekuensi / Mitigasi**:
  - UI menggunakan native Win32 controls (diberi Windows Visual Styles via Manifest file `ComCtl32 v6` agar modern di Windows 10/11 dan retro-native di Windows XP).
