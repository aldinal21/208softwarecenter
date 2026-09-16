# Map: Photo Studio & Copy Center Capabilities Upgrade

## Destination
Mewujudkan suite fitur studio & toko fotokopi profesional pada 208 Software Center: Filter Hitam Putih (B&W), Duplikasi Item Antrean Cepat, Pengganti Latar Belakang Cerdas Hybrid (Level 1 Native Color Replacer + Level 2 Offline AI Segmentation) ber-preset pas foto Indonesia (Merah/Biru/Putih/Abu/Kuning), dan Retouch Pencahayaan (Brightness, Contrast, Auto-Enhance).

## Notes
- **Domain**: Desktop Windows native (Win32 API + GDI+ C++17) dengan target kompatibilitas Windows XP SP3 hingga Windows 11.
- **Skills**: `domain-modeling`, `grilling`, `wayfinder`.
- **Standing Preferences**:
  - Performa instan, zero external heavy dependency pada core engine, rendering konsisten di Canvas, Print Preview, Spooler Printer Fisik, dan PDF/Image Exporter.
  - Skema hybrid untuk ganti background: Level 1 Native Chroma (<0.01s, 0% CPU overhead untuk background polos/tembok) dan Level 2 Offline AI untuk latar kompleks.

## Decisions so far

*(Belum ada tiket yang ditutup - semua tiket berada di frontier awal)*

## Frontier Tickets
- [001: Black & White Filter and Duplicate Order Item](001-black-and-white-and-duplicate-item.md)
- [002: Photo Retouch & Lighting Enhancement](002-photo-retouch-enhancement.md)
- [003: Native Studio Background Color Replacer with Indonesian Pas Foto Presets](003-native-background-color-replacer.md)
- [004: Offline AI Background Segmentation Integration (Level 2)](004-offline-ai-background-segmentation.md)

## Not yet specified
- *Preset Paket Cetak Siap Pakai*: Preset 1-click kombinasi jumlah cetak umum studio (misal: "Paket Lamaran Kerja": 4 lembar 3x4 + 2 lembar 4x6, "Paket Ijazah": 6 lembar 3x4 B&W + 4 lembar 4x6 Warna).
- *Watermark / Studio Stamp*: Fitur opsional menambahkan cap studio atau tanggal cetak di sudut belakang/samping foto.

## Out of scope
- Layanan cloud upload / background removal API berbasis internet (aplikasi harus 100% offline demi privasi pelanggan dan keandalan di toko tanpa internet).
- Fitur photo editing kompleks layer-based seperti Photoshop (aplikasi ini fokus pada kecepatan alur kerja layout pas foto cetak).
