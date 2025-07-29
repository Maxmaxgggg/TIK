// CUDA-ядро для расчёта весового спектра кодовых слов
// Использует константную и shared-память

#include <cuda_runtime.h>
#include <cstdint>
#include <stdexcept> 
// В начале .cu-файла (после include)
#ifdef __cplusplus
extern "C" {
#endif
// 1. Константная память для упакованной порождающей матрицы
constexpr int MAX_CONST_ELEMENTS = 8192;  // максимум элементов (64KB / 8 байт)
__constant__ uint64_t d_matrix[MAX_CONST_ELEMENTS];

// 2. Глобальный массив спектра в глобальной памяти устройства
// d_spectrum выделяется и обнуляется на хосте

// 3. Ядро CUDA
__global__ void computeSpectrumKernel(uint64_t* d_spectrum,
    int n,
    int k,
    int blockCount,
    uint64_t numComb)
{
    // 3.1 Shared-память для локального спектра каждого блока
    extern __shared__ uint64_t s_spectrum[];  // размер = n+1

    int tid = threadIdx.x;
    int threadsPerBlock = blockDim.x;
    // инициализируем локальный спектр нулями
    for (int idx = tid; idx <= n; idx += threadsPerBlock) {
        s_spectrum[idx] = 0ULL;
    }
    __syncthreads();

    // 3.2 Вычисляем глобальный индекс и общее число потоков
    uint64_t globalTid = blockIdx.x * blockDim.x + tid;
    uint64_t totalThreads = gridDim.x * blockDim.x;

    // 3.3 Stride-цикл по всем комбинациям
    uint64_t codeword[16];  // предполагается blockCount<=16
    for (uint64_t mask = globalTid; mask < numComb; mask += totalThreads) {
        // обнуляем codeword
        for (int b = 0; b < blockCount; ++b) {
            codeword[b] = 0ULL;
        }
        // XOR строк из константной памяти
        for (int row = 0; row < k; ++row) {
            if (mask & (1ULL << row)) {
                int offset = row * blockCount;
                for (int b = 0; b < blockCount; ++b) {
                    codeword[b] ^= d_matrix[offset + b];
                }
            }
        }
        // подсчёт веса
        int weight = 0;
        for (int b = 0; b < blockCount; ++b) {
            weight += __popcll(codeword[b]);
        }
        // накапливаем в shared-памяти
        atomicAdd(&s_spectrum[weight], 1ULL);
    }
    __syncthreads();

    // 3.4 Сводим локальный спектр в глобальный
    if (tid == 0) {
        for (int i = 0; i <= n; ++i) {
            atomicAdd(&d_spectrum[i], s_spectrum[i]);
        }
    }
}

// 4. Хостовая обёртка для запуска
// Принимает: массив matrix на хосте, массив spectrum на хосте, параметры n,k, blockCount, threadsPerBlock, blocks
void launchSpectrumKernel(const uint64_t* h_matrixPacked,
    uint64_t* h_spectrum,
    int n,
    int k,
    int blockCount,
    int threadsPerBlock,
    int blocks)
{
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err != cudaSuccess || deviceCount == 0) {
        throw std::runtime_error("Error: CUDA-compatible GPU not found or CUDA is not available.");
    }
    // 4.1 Копируем порождающую матрицу в константную память устройства
    size_t matrixBytes = size_t(k) * blockCount * sizeof(uint64_t);
    cudaMemcpyToSymbol(d_matrix,
        h_matrixPacked,
        matrixBytes);

    // 4.2 Выделяем глобальный массив спектра на устройстве
    uint64_t* d_spectrum;
    size_t spectrumBytes = size_t(n + 1) * sizeof(uint64_t);
    cudaMalloc(&d_spectrum, spectrumBytes);
    // Обнуляем спектр на устройстве
    cudaMemset(d_spectrum, 0, spectrumBytes);

    // 4.3 Вычисляем число комбинаций
    uint64_t numComb = 1ULL << k;

    // 4.4 Размер shared-памяти для каждого блока
    size_t sharedBytes = spectrumBytes;

    // 4.5 Запуск ядра с заданными blocks и threadsPerBlock
    computeSpectrumKernel << <blocks, threadsPerBlock, sharedBytes >> > (
        d_spectrum,
        n,
        k,
        blockCount,
        numComb);

    // Ждём окончания выполнения на GPU
    cudaDeviceSynchronize();

    // 4.6 Копируем результат обратно на хост
    cudaMemcpy(h_spectrum, d_spectrum, spectrumBytes, cudaMemcpyDeviceToHost);

    // 4.7 Освобождаем устройство
    cudaFree(d_spectrum);
}
#ifdef __cplusplus
} // extern "C"
#endif