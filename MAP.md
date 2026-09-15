# Wayfinder Map: 208 Software Center (Pas Foto MVP)

## Destination

Aplikasi desktop native standalone (single `.exe`) berbasis Win32 C/C++ GDI+ yang berjalan dari Windows XP SP3 hingga Windows 11 untuk mengotomasi alur kerja cetak Pas Foto di toko fotokopi 208 (Drag & Drop dari WhatsApp, interactive framing, top-aligned paper-saving layout di kertas A4, direct printing, dan high-res export).

## Notes

- **Target OS**: Windows XP SP3 (subsystem 5.1), Windows 7, Windows 10, Windows 11.
- **Compiler**: MinGW-w64 GCC (i686 32-bit) dengan `-static -mwindows -Wl,--subsystem,windows:5.1`.
- **Primary Media**: Kertas A4 (210 x 297 mm) dengan top-aligned packing untuk pemanfaatan sisa kertas.
- **Skills**: `grilling`, `domain-modeling`, `prototype`, `systematic-debugging`.

## Decisions so far

- [ADR 0001: Tech Stack & Architecture for Windows XP to Windows 11 Compatibility](docs/adr/0001-winxp-win11-native-stack.md): Dipilih C/C++ Win32 API + GDI+ (MinGW i686) untuk zero dependency runtime.

## Open Tickets

- [T-001: Setup Toolchain MinGW-w64 i686 & XP-Compatible Build System](.wayfinder/tickets/T-001.md) (Type: `wayfinder:task`) - *Unblocked*
- [T-002: Core Layout Engine & Top-Aligned Paper-Saving Packing](.wayfinder/tickets/T-002.md) (Type: `wayfinder:prototype`) - *Unblocked*
- [T-003: Win32 MainWindow & Drag-and-Drop Image Ingestion](.wayfinder/tickets/T-003.md) (Type: `wayfinder:prototype`) - *Blocked by T-001*
- [T-004: Interactive Image Framing & Aspect Ratio Fitting](.wayfinder/tickets/T-004.md) (Type: `wayfinder:prototype`) - *Blocked by T-003*
- [T-005: GDI+ Direct Print & 300 DPI Export Pipeline](.wayfinder/tickets/T-005.md) (Type: `wayfinder:prototype`) - *Blocked by T-002, T-004*

## Not yet specified

- Modul ganti/hapus background pas foto (Merah / Biru) semi-otomatis.
- Modul pencatatan transaksi kasir & print nota kecil untuk fotokopi 208.
- Preset media cetak lanjutan (Kertas 4R Photo Paper, F4/Folio).

## Out of scope

- Web-based backend / database server (Aplikasi full offline-first standalone).
- Framework berbasis Chromium / WebView2 / Electron (karena melanggar syarat kompatibilitas Windows XP).
