// ==== spectrum_kernel.h ====
#ifndef SPECTRUM_KERNEL_H
#define SPECTRUM_KERNEL_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

    // Запуск CUDA-ядра для расчёта весового спектра
    // h_matrixPacked  - указатель на упакованную матрицу на хосте (k * blockCount элементов)
    // h_spectrum      - указатель на массив спектра на хосте (n+1 элементов)
    // n               - длина кодового слова
    // k               - число строк порождающей матрицы
    // blockCount      - число 64-битных слов на строку
    // threadsPerBlock - число потоков в одном блоке
    // blocks          - число блоков в сетке
    void launchSpectrumKernel(
        const uint64_t* h_matrixPacked,
        uint64_t*       h_spectrum,
        int             n,
        int             k,
        int             blockCount,
        int             threadsPerBlock,
        int             blocks);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SPECTRUM_KERNEL_H