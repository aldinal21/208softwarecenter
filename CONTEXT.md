# 208 Software Center - Domain Model

## Core Concepts

### Pas Foto
Foto identitas berukuran fisik standar yang dicetak untuk keperluan administrasi resmi di Indonesia.
- **Ukuran Standar**:
  - `2x3` (efektif 21.6 x 27.9 mm)
  - `3x4` (efektif 28.0 x 38.0 mm)
  - `4x6` (efektif 38.0 x 56.0 mm)
- **Background**: Latar belakang solid (Merah / Biru / Putih).

### Kertas Cetak (Sheet / Media)
Media kertas cetak tempat susunan foto diletakkan.
- **A4 (Primary)**: 210 x 297 mm (orientasi Portrait/Landscape).
- **Custom / Reusable Cutoff**: Sisa potongan kertas yang digunakan kembali.

### Ingestion (Universal Drag & Drop)
Kemampuan menerima file foto yang di-drag & drop dari sumber mana saja:
- Windows File Explorer / File Manager
- WhatsApp Desktop / WhatsApp Web
- Browser Download bar (Chrome, Edge, Firefox)
- Desktop / Folder lokal
- Mendukung format file: `.jpg`, `.jpeg`, `.png`, `.bmp`, `.webp` (via decoding library / WIC / GDI+).

### Framing & Aspect Ratio Fitting
Penyesuaian posisi (pan/zoom/center) foto input pelanggan agar masuk secara proporsional ke dalam rasio pas foto target tanpa terdistorsi/gepeng.

### Top-Aligned Packing (Hemat Kertas)
Algoritma penataan pas foto yang otomatis merapatkan susunan foto ke bagian atas-kiri kertas (Top-Left aligned), menyisakan bagian bawah lembar tetap bersih agar kertas A4 dapat dipakai ulang (reusable paper) di sesi cetak berikutnya.

### Cut Marks / Crop Lines
Garis batas potong tipis (abu-abu/putus-putus) di sekeliling tiap pas foto sebagai panduan pemotongan menggunakan gunting atau alat pemotong kertas (paper cutter).

### Output Pipeline
- **Direct Print**: Mencetak langsung ke printer Windows lokal via Win32 GDI Print API.
- **Export**: Menyimpan hasil tata letak ke file gambar beresolusi tinggi (300 DPI) siap cetak.
