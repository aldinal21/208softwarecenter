# Ticket: Offline AI Background Segmentation Integration (Level 2)

**Label**: `wayfinder:research`
**Parent**: [Map](map.md)
**Status**: `frontier`

## Question
Bagaimana mengintegrasikan model neural segmentation offline yang ringan (~20-40 MB, misal MODNet/RMBG via ncnn atau ONNX Runtime C++ API) ke dalam aplikasi native Win32 208 Software Center tanpa dependensi runtime Python/CUDA eksternal, sehingga mampu memotong background foto yang ramai/kompleks secara otomatis dengan fallback mulus ke Native Color Replacer?

## Scope & Investigation Details
1. **Engine Selection & Binary Footprint**:
   - Evaluasi runtime neural network C++ native (misal: `ncnn` dari Tencent atau `onnxruntime.dll` CPU build).
   - Pastikan binary dan bobot model (weights `.bin`/`.onnx`) dapat berjalan mandiri di CPU (x86/x64) tanpa GPU khusus dan tanpa mengorbankan portabilitas aplikasi.
2. **Asynchronous Execution & UI Feedback**:
   - Proses inferensi dijalankan di background thread (`std::thread` / `CreateThread`) agar UI tidak freeze.
   - Sediakan progress bar / spinner "Memproses AI..." saat foto dipotong.
3. **Graceful Fallback**:
   - Jika model AI belum didownload atau sistem hardware sangat terbatas (misal Windows XP 32-bit), sistem otomatis fallback ke Level 1 (Native Chroma/Tolerance Replacer).
