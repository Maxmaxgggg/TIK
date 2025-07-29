// ==== spectrum_kernel.h ====
#ifndef SPECTRUM_KERNEL_H
#define SPECTRUM_KERNEL_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

    // ������ CUDA-���� ��� ������� �������� �������
    // h_matrixPacked  - ��������� �� ����������� ������� �� ����� (k * blockCount ���������)
    // h_spectrum      - ��������� �� ������ ������� �� ����� (n+1 ���������)
    // n               - ����� �������� �����
    // k               - ����� ����� ����������� �������
    // blockCount      - ����� 64-������ ���� �� ������
    // threadsPerBlock - ����� ������� � ����� �����
    // blocks          - ����� ������ � �����
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