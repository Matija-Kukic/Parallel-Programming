#include <cuda_runtime.h>
#include <math.h>
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

__host__ __device__ bool isPrime(int num) {
    if (num <= 1)
        return false;
    if (num == 2)
        return true;
    if (num % 2 == 0)
        return false;

    for (int i = 3; i <= num / i; i += 2) {
        if (num % i == 0)
            return false;
    }
    return true;
}

bool isPrimeCPU(int num) {
    if (num <= 1)
        return false;
    if (num == 2)
        return true;

    for (int i = 3; i < num; i++) {
        if (num % i == 0)
            return false;
    }
    return true;
}

int cpu_countPrimes(const int *array, int N) {
    int count = 0;
    for (int i = 0; i < N; i++) {
        if (isPrimeCPU(array[i])) {
            count++;
        }
    }
    return count;
}

__global__ void countPrimesNonAtomic(const int *d_array, int N,
                                     int *d_counter) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    for (int i = idx; i < N; i += stride) {
        if (isPrime(d_array[i])) {
            (*d_counter)++;
        }
    }
}

__global__ void countPrimesAtomic(const int *d_array, int N, int *d_counter) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;

    for (int i = idx; i < N; i += stride) {
        if (isPrime(d_array[i])) {
            atomicAdd(d_counter, 1);
        }
    }
}

int main() {
    int k = 11;
    int N = 1 << k;
    int B = 128;
    int L = 256;
    int G = B * L;

    printf("--- KONFIGURACIJA ---\n");
    printf("Velicina ulaza (N): 2^%d = %d\n", k, N);
    printf("Blokovi (B):        %d\n", B);
    printf("Dretve po bloku (L):%d\n", L);
    printf("Ukupno dretvi (G):  %d\n\n", G);

    int *h_array = (int *)malloc(N * sizeof(int));

    srand((unsigned int)time(NULL));
    printf("Generiram podatke...\n\n");

    unsigned int min = 10;
    unsigned int max = UINT_MAX >> 6;
    // printf("%ld\n", max);
    for (int i = 0; i < N; i++) {
        h_array[i] = min + rand() % (max - min + 1);
        if (i % 2 == 0) {
            h_array[i] = 5;
        }
    }

    int *d_array, *d_counter;
    CHECK(cudaMalloc((void **)&d_array, N * sizeof(int)));
    CHECK(cudaMalloc((void **)&d_counter, sizeof(int)));

    clock_t start, stop;
    double elapsed_ms;

    printf("Pocinjem CPU...\n\n");
    int h_counter_cpu = 0;
    start = clock();
    h_counter_cpu = cpu_countPrimes(h_array, N);
    stop = clock();
    elapsed_ms = ((double)(stop - start) / CLOCKS_PER_SEC) * 1000.0;

    printf("[SERIJSKA (CPU) VERZIJA]\n");
    printf(" -> Pronadjeno prostih brojeva: %d\n", h_counter_cpu);
    printf(" -> Vrijeme izvodenja: %.2f ms\n\n", elapsed_ms);

    int h_counter_non_atomic = 0;
    start = clock();

    CHECK(
        cudaMemcpy(d_array, h_array, N * sizeof(int), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(d_counter, &h_counter_non_atomic, sizeof(int),
                     cudaMemcpyHostToDevice));

    countPrimesNonAtomic<<<B, L>>>(d_array, N, d_counter);
    CHECK(cudaDeviceSynchronize());

    CHECK(cudaMemcpy(&h_counter_non_atomic, d_counter, sizeof(int),
                     cudaMemcpyDeviceToHost));

    stop = clock();
    elapsed_ms = ((double)(stop - start) / CLOCKS_PER_SEC) * 1000.0;

    printf("[VERZIJA BEZ ATOMARNE OPERACIJE]\n");
    printf(" -> Pronadjeno prostih brojeva: %d\n", h_counter_non_atomic);
    printf(" -> Vrijeme izvodenja: %.2f ms\n\n", elapsed_ms);

    int h_counter_atomic = 0;
    start = clock();

    CHECK(
        cudaMemcpy(d_array, h_array, N * sizeof(int), cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(d_counter, &h_counter_atomic, sizeof(int),
                     cudaMemcpyHostToDevice));

    countPrimesAtomic<<<B, L>>>(d_array, N, d_counter);
    CHECK(cudaDeviceSynchronize());

    CHECK(cudaMemcpy(&h_counter_atomic, d_counter, sizeof(int),
                     cudaMemcpyDeviceToHost));

    stop = clock();
    elapsed_ms = ((double)(stop - start) / CLOCKS_PER_SEC) * 1000.0;

    printf("[VERZIJA S ATOMARNOM OPERACIJOM]\n");
    printf(" -> Pronadjeno prostih brojeva: %d\n", h_counter_atomic);
    printf(" -> Vrijeme izvodenja: %.2f ms\n", elapsed_ms);

    if (h_counter_atomic == h_counter_cpu) {
        printf(" -> PROVJERA: TOCNO (poklapa se sa serijskom)\n\n");
    } else {
        printf(" -> PROVJERA: NETOCNO (razlikuje se od serijske)\n\n");
    }

    CHECK(cudaFree(d_array));
    CHECK(cudaFree(d_counter));
    free(h_array);
    return 0;
}
