# 208 Software Center 🖨️📸

> **Aplikasi Desktop Native Ringan & Cepat untuk Layout dan Cetak Pas Foto Otomatis (A4) — Mendukung Windows XP SP3 hingga Windows 11.**

[![Platform](https://img.shields.io/badge/platform-Windows%20XP%20SP3%20%7C%207%20%7C%2010%20%7C%2011-blue.svg)](https://github.com/aldinal21/208softwarecenter)
[![Language](https://img.shields.io/badge/language-C%2B%2B17%20%2F%20Win32-green.svg)](https://github.com/aldinal21/208softwarecenter)
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)

---

## ✨ Fitur Utama

- **🚀 Ringan & Mandiri (Zero Runtime Dependency)**:
  - Dibuat dengan **Pure Win32 API + GDI+** (C++17).
  - Tidak memerlukan .NET Framework, Visual C++ Redistributable, atau WebView2.
  - Ukuran binary sangat kecil (< 2 MB) dan konsumsi RAM sangat hemat (< 15 MB).
- **📸 Multi-Preset per Foto**:
  - 1 foto dapat diatur jumlah cetaknya untuk beberapa ukuran sekaligus (`2x3`, `3x4`, dan `4x6`) secara simultan menggunakan kontrol stepper horizontal `- [ 0 ] +`.
- **✂️ Algoritma Smart Strip (Default)**:
  - Penataan otomatis berbasis partisi guillotine horizontal.
  - Menjamin garis potong 100% lurus ujung-ke-ujung (mudah dipotong dengan gunting/paper trimmer) dengan efisiensi sisa kertas maksimal.
  - Pilihan mode packing: **⚡ Smart Strip**, **✂️ Baris Murni (EasyCut)**, dan **📐 Hemat Maksimal (MaxDensity)**.
- **🖼️ Dedicated Modal Print Preview**:
  - Pratinjau cetak layar penuh dengan studio dark slate canvas (`RGB(50, 52, 56)`).
  - Efek drop shadow A4 realistis, kontrol zoom (`Zoom +`, `Zoom -`, `Pas Layar / 100%`, `Ctrl + MouseWheel`).
  - Navigasi multi-halaman (`◀ Sebelumnya`, `Berikutnya ▶`, panah keyboard).
- **📄 Multi-Page Pagination**:
  - Otomatis membagi order dalam jumlah banyak ke beberapa lembar A4.
  - Multi-page direct print spooling dan multi-page high-resolution 300 DPI image export.
- **📥 Universal Ingestion (Drag & Drop)**:
  - Mendukung drag & drop langsung dari Windows Explorer, WhatsApp Desktop, browser download bar, atau tombol Tambah Foto.
- **⚡ Pintasan Keyboard (Accelerators)**:
  - `Ctrl + P`: Pratinjau & Cetak langsung ke printer
  - `Ctrl + S`: Ekspor hasil layout ke file gambar 300 DPI (PNG/JPEG)
  - `Ctrl + O`: Tambah file foto
  - `Delete`: Hapus foto dari antrian
  - `Esc`: Tutup dialog pratinjau

---

## 🛠️ Persyaratan Kompilasi (Build from Source)

- **OS**: Windows XP SP3 s.d. Windows 11
- **Toolchain**: MinGW-w64 GCC (i686 32-bit atau x86_64)

### Langkah Kompilasi:

Jalankan script `build.bat`:

```cmd
build.bat
```

Hasil binary `.exe` yang siap pakai akan berada di folder `bin\208softwarecenter.exe`.

---

## 📦 Ukuran Pas Foto Standar

| Preset | Ukuran Fisik (mm) |
| :--- | :--- |
| **2x3** | 21.6 × 27.9 mm |
| **3x4** | 28.0 × 38.0 mm |
| **4x6** | 38.0 × 56.0 mm |

---

## 📄 Lisensi

Proyek ini dilisensikan di bawah lisensi [MIT](LICENSE).
