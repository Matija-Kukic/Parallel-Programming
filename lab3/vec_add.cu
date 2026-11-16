#include <cuda_runtime.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

// -------- ERROR CHECK MACRO --------
#define CHECK(call)                                                            \
    do {                                                                       \
        cudaError_t err = call;                                                \
        if (err != cudaSuccess) {                                              \
            printf("CUDA error at %s:%d: %s\n", __FILE__, __LINE__,            \
                   cudaGetErrorString(err));                                   \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

// -------- KERNEL --------
__global__ void vecAdd(const float *A, const float *B, float *C, int N) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    if (i < N) {
        C[i] = A[i] + B[i];
    }
}

// -------- MAIN --------
int main() {
    int N = 1 << 20; // ~1M elements
    size_t size = N * sizeof(float);

    printf("Allocating %d elements (%zu bytes)\n", N, size);

    // -------- Host memory --------
    float *h_A = (float *)malloc(size);
    float *h_B = (float *)malloc(size);
    float *h_C = (float *)malloc(size);

    if (!h_A || !h_B || !h_C) {
        printf("Host malloc failed\n");
        return 1;
    }

    // Initialize data
    for (int i = 0; i < N; i++) {
        h_A[i] = (float)i;
        h_B[i] = (float)i;
    }

    // -------- Device memory --------
    float *d_A = NULL, *d_B = NULL, *d_C = NULL;

    CHECK(cudaMalloc((void **)&d_A, size));
    CHECK(cudaMalloc((void **)&d_B, size));
    CHECK(cudaMalloc((void **)&d_C, size));

    // -------- Copy to device --------
    CHECK(cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(d_B, h_B, size, cudaMemcpyHostToDevice));

    // -------- Kernel launch config --------
    int threadsPerBlock = 256;
    int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;

    printf("Launching kernel: %d blocks, %d threads/block\n", blocksPerGrid,
           threadsPerBlock);

    // -------- Launch kernel --------
    vecAdd<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, d_C, N);

    // Check for launch errors
    CHECK(cudaGetLastError());

    // Wait for GPU to finish
    CHECK(cudaDeviceSynchronize());

    // -------- Copy result back --------
    CHECK(cudaMemcpy(h_C, d_C, size, cudaMemcpyDeviceToHost));

    // -------- Verify result --------
    printf("C[0]=%f, C[1]=%f, C[2]=%f\n", h_C[0], h_C[1], h_C[2]);

    // Optional sanity check
    for (int i = 0; i < 5; i++) {
        printf("C[%d] = %f (expected %f)\n", i, h_C[i], (float)(2 * i));
    }

    // -------- Cleanup --------
    CHECK(cudaFree(d_A));
    CHECK(cudaFree(d_B));
    CHECK(cudaFree(d_C));

    free(h_A);
    free(h_B);
    free(h_C);

    return 0;
}
