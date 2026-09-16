# Domain Context: 208 Software Center

## Glossary

### Photo Order Item
Item foto pesanan pelanggan yang dimasukkan ke antrean cetak. Menyimpan path file gambar sumber, kuantitas cetak per ukuran standar (2x3, 3x4, 4x6), parameter framing & orientasi, serta konfigurasi filter warna/background.

### Framing Parameters
Konfigurasi pemotongan (crop) dan sudut pandang pada foto:
- `cropCenterX`: Titik pusat horizontal crop (0.0 - 1.0).
- `cropCenterY`: Titik pusat vertikal crop (0.0 - 1.0, default 0.45 dengan bias atas untuk pas foto).
- `zoomLevel`: Tingkat perbesaran (1.0x - 3.0x).
- `rotation`: Sudut putar foto dalam kelipatan 90 derajat (0°, 90°, 180°, 270°).

### Black & White (B&W / Grayscale) Filter
Filter desaturasi luminansi warna standar (NTSC/ITU-R BT.601: `0.299 R + 0.587 G + 0.114 B`) untuk mencetak pas foto hitam-putih resmi dokumen/ijazah, dapat diaktifkan per item foto tanpa merusak file asli.

### Photo Retouch & Enhancement
Penyesuaian visual pencahayaan foto pelanggan toko:
- `brightness`: Pengatur kecerahan (-50% s/d +50%).
- `contrast`: Pengatur ketajaman kontras (-50% s/d +50%).
- `autoEnhance`: Koreksi histogram otomatis untuk foto yang redup/gelap.

### Standard Pas Foto Background Presets
Standar warna latar pas foto resmi di Indonesia:
- **Merah Pas Foto** (`#D32F2F`): KTP & dokumen resmi untuk tahun kelahiran ganjil.
- **Biru Pas Foto** (`#1976D2`): KTP & dokumen resmi untuk tahun kelahiran genap.
- **Putih** (`#FFFFFF`): Dokumen visa, paspor internasional, dan buku nikah.
- **Abu-Abu** (`#757575`): Dokumen kartu pegawai & instansi BUMN/swasta.
- **Kuning** (`#FBC02D`): Pendaftaran instansi khusus/akademik.
- **Custom Color**: Pemilihan warna bebas menggunakan dialog warna.

### Smart Hybrid Background Removal
Arsitektur penggantian latar belakang dua tahap:
1. **Native Color Tool (Level 1)**: Penggantian latar foto polos/tembok berbasis toleransi warna tepi & *edge feathering* halus, 100% native Win32/GDI+ sangat cepat (< 0.01 detik) dan kompatibel dari Windows XP s/d Windows 11.
2. **Offline AI Segmentation (Level 2)**: Model inferensi neural network offline berukuran kompak (~20–40 MB) untuk memisahkan subjek dari latar belakang yang ramai/kompleks.

### Auto EXIF Normalization
Proses otomatis pembacaan tag orientasi EXIF (`0x0112`) saat file gambar di-load dari disk, memutar gambar ke orientasi tegak natural sebelum ditampilkan di antarmuka.

### Interactive Framing Modal
Dialog interaktif modal native Win32 untuk menyesuaikan framing foto:
- **Canvas Viewport**: Drag / pan foto dengan mouse untuk menggeser posisi kepala/wajah.
- **Zoom Slider / Scroll Wheel**: Mengatur perbesaran foto.
- **Rotate Buttons**: Tombol putar 90° searah / berlawanan jarum jam.
- **Aspect Ratio Preview**: Toggle preview bingkai rasio 2x3, 3x4, dan 4x6 secara instan.
- **Color & Background Toolbar**: Toolbar kontrol B&W, retouch, dan ganti latar belakang.

### Smart Strip Packing
Mesin tata letak (layout engine) berorientasi pemotongan guillotine lurus horizontal, mengelompokkan baris foto agar mudah dipotong dalam satu tarikan pisau cutter/mesin pemotong kertas.
