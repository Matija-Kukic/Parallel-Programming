#include <cuda_runtime.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHECK(call)                                                            \
    do {                                                                       \
        cudaError_t err = call;                                                \
        if (err != cudaSuccess) {                                              \
            printf("CUDA error at %s:%d: %s\n", __FILE__, __LINE__,            \
                   cudaGetErrorString(err));                                   \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

__host__ __device__ bool isPrime(unsigned int num) {
    if (num <= 1)
        return false;
    if (num == 2)
        return true;
    if (num % 2 == 0)
        return false;

    for (unsigned int i = 3; i <= num / i; i += 2) {
        if (num % i == 0)
            return false;
    }
    return true;
}

__global__ void countPrimesNonAtomic(const unsigned int *d_array, int N,
                                     int *d_counter) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    for (int i = idx; i < N; i += stride) {
        if (isPrime(d_array[i])) {
            (*d_counter)++;
        }
    }
}

__global__ void countPrimesAtomic(const unsigned int *d_array, int N,
                                  int *d_counter) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    for (int i = idx; i < N; i += stride) {
        if (isPrime(d_array[i])) {
            atomicAdd(d_counter, 1);
        }
    }
}

int main() {
    int k_values[] = {20, 22, 24};
    int B_values[] = {16, 32, 64, 128, 256};
    int L_values[] = {32, 64, 128, 256, 512};

    srand((unsigned int)time(NULL));

    printf("%-6s | %-5s | %-5s | %-7s | %-19s | %-19s | %-16s | %-16s\n", "N",
           "B", "L", "G", "Vrijeme Bez atomika", "Vrijeme Sa atomikom",
           "Broj Bez atomika", "Broj Sa atomikom");
    printf("-------------------------------------------------------------------"
           "-----------------------------------------------\n");

    for (int ki = 0; ki < 3; ki++) {
        int k = k_values[ki];
        int N = 1 << k;

        for (int bi = 0; bi < 5; bi++) {
            for (int li = 0; li < 5; li++) {
                int B = B_values[bi];
                int L = L_values[li];
                int G = B * L;

                clock_t start, stop;
                double elapsed_ms_non_atomic, elapsed_ms_atomic;

                unsigned int min = 10;
                unsigned int max = UINT_MAX >> 4;

                unsigned int *h_array_non_atomic =
                    (unsigned int *)malloc(N * sizeof(unsigned int));
                for (int i = 0; i < N; i++) {
                    h_array_non_atomic[i] = min + rand() % (max - min + 1);
                }

                start = clock();

                unsigned int *d_array_non_atomic;
                int *d_counter_non_atomic;
                CHECK(
                    cudaMalloc(&d_array_non_atomic, N * sizeof(unsigned int)));
                CHECK(cudaMalloc(&d_counter_non_atomic, sizeof(int)));

                CHECK(cudaMemcpy(d_array_non_atomic, h_array_non_atomic,
                                 N * sizeof(unsigned int),
                                 cudaMemcpyHostToDevice));

                int h_counter_non_atomic = 0;
                CHECK(cudaMemcpy(d_counter_non_atomic, &h_counter_non_atomic,
                                 sizeof(int), cudaMemcpyHostToDevice));

                countPrimesNonAtomic<<<B, L>>>(d_array_non_atomic, N,
                                               d_counter_non_atomic);
                CHECK(cudaDeviceSynchronize());

                CHECK(cudaMemcpy(&h_counter_non_atomic, d_counter_non_atomic,
                                 sizeof(int), cudaMemcpyDeviceToHost));

                stop = clock();
                elapsed_ms_non_atomic =
                    ((double)(stop - start) / CLOCKS_PER_SEC) * 1000.0;

                CHECK(cudaFree(d_array_non_atomic));
                CHECK(cudaFree(d_counter_non_atomic));
                free(h_array_non_atomic);

                unsigned int *h_array_atomic =
                    (unsigned int *)malloc(N * sizeof(unsigned int));
                for (int i = 0; i < N; i++) {
                    h_array_atomic[i] = min + rand() % (max - min + 1);
                }

                start = clock();

                unsigned int *d_array_atomic;
                int *d_counter_atomic;
                CHECK(cudaMalloc(&d_array_atomic, N * sizeof(unsigned int)));
                CHECK(cudaMalloc(&d_counter_atomic, sizeof(int)));

                CHECK(cudaMemcpy(d_array_atomic, h_array_atomic,
                                 N * sizeof(unsigned int),
                                 cudaMemcpyHostToDevice));

                int h_counter_atomic = 0;
                CHECK(cudaMemcpy(d_counter_atomic, &h_counter_atomic,
                                 sizeof(int), cudaMemcpyHostToDevice));

                countPrimesAtomic<<<B, L>>>(d_array_atomic, N,
                                            d_counter_atomic);
                CHECK(cudaDeviceSynchronize());

                CHECK(cudaMemcpy(&h_counter_atomic, d_counter_atomic,
                                 sizeof(int), cudaMemcpyDeviceToHost));

                stop = clock();
                elapsed_ms_atomic =
                    ((double)(stop - start) / CLOCKS_PER_SEC) * 1000.0;

                CHECK(cudaFree(d_array_atomic));
                CHECK(cudaFree(d_counter_atomic));
                free(h_array_atomic);

                char n_str[10];
                sprintf(n_str, "2^%d", k);

                printf("%-6s | %-5d | %-5d | %-7d | %-16.2f ms | %-16.2f ms | "
                       "%-16d | %-16d\n",
                       n_str, B, L, G, elapsed_ms_non_atomic, elapsed_ms_atomic,
                       h_counter_non_atomic, h_counter_atomic);
            }
        }

        printf("---------------------------------------------------------------"
               "---------------------------------------------------\n");
    }

    printf("\nTestiranje zavrseno!\n");
    return 0;
}
