# Ticket: Native Studio Background Color Replacer with Indonesian Pas Foto Presets

**Label**: `wayfinder:prototype`
**Parent**: [Map](map.md)
**Status**: `frontier`

## Question
Bagaimana membangun modul pengganti background foto native berbasis toleransi warna tepi (chroma/flood keying + edge feathering) yang ringan dan instan (<0.01s) untuk foto berlatar polos/tembok dengan preset warna standar Indonesia (Merah `#D32F2F`, Biru `#1976D2`, Putih `#FFFFFF`, Abu `#757575`, Kuning `#FBC02D`, dan Custom)?

## Scope & Implementation Details
1. **Core Data Structure**:
   - Tambahkan enum `BgReplaceMode { None, PresetRed, PresetBlue, PresetWhite, PresetGrey, PresetYellow, Custom }` dan `uint32_t customBgColor` pada `PhotoOrderItem` & `PlacedPhotoSlot`.
2. **Algorithm**:
   - Algoritma penggantian warna latar native: mendeteksi warna sampel dari sudut-sudut foto (corner sampling), menghitung jarak warna delta-E / RGB Euclidean dengan batas toleransi, dan melakukan alpha blending / feathering pada tepi rambut & bahu.
3. **UI Controls**:
   - Di `EditPhotoDialog`, sediakan palette tombol warna: 🔴 Merah, 🔵 Biru, ⚪ Putih, 🔘 Abu, 🟡 Kuning, 🎨 Custom, dan ❌ Asli.
   - Sediakan slider "Toleransi Latar" (Tolerance) untuk mengatur sensitivitas pembersihan tepi.
