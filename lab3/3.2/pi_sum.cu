#include <cuda_runtime.h>
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

__global__ void calcPi(long long unsigned int N, double *res) {
    int id = threadIdx.x;
    int bid = blockIdx.x;
    int B = blockDim.x;
    int G = gridDim.x;
    int tid = bid * B + id;

    double sum = 0;
    for (int i = tid; i < N; i += B * G) {
        double x = (i - 0.5) / (double)N;
        sum += 1.0 / (1.0 + x * x);
    }
    res[tid] = sum;
}

int main() {
    int n;
    scanf("%d", &n);
    long long unsigned int N = (long long unsigned int)2 << n;
    printf("Unesena duljina reda N = 2^%d = %llu\n\n", n, N);

    for (int jj = 0; jj < 5; jj++) {

        printf("--------------------------\n\n");
        printf("Izvodjenje %d\n", jj + 1);

        clock_t start, stop;
        start = clock();
        double *Result;
        int threadsPerBlock = 256;
        int blocksPerGrid = 28 * 12; // 28 SM and 16 theoretical maximum
                                     //

        int T = threadsPerBlock * blocksPerGrid;
        CHECK(cudaMalloc(&Result, (size_t)T * sizeof(double)));
        calcPi<<<blocksPerGrid, threadsPerBlock>>>(N, Result);
        CHECK(cudaDeviceSynchronize());
        // printf("GOTOV SA GPU RACUNOM\n");
        double *FRes = (double *)malloc((size_t)T * sizeof(double));
        CHECK(cudaMemcpy(FRes, Result, (size_t)T * sizeof(double),
                         cudaMemcpyDeviceToHost));
        double sum = 0;

        // printf("IDEM SUMIRAT\n");
        for (int i = 0; i < T; i++) {
            sum += FRes[i];
        }
        sum *= 4.0 / N;
        printf("[Dobiveni PI]: %lf\n", sum);

        stop = clock();
        double elapsed_time =
            ((double)(stop - start) / CLOCKS_PER_SEC) * 1000.0;

        printf("[Vrijeme izvodjenja]: %lf ms\n", elapsed_time);
        printf("--------------------------\n\n");

        CHECK(cudaFree(Result));
        free(FRes);
        CHECK(cudaDeviceSynchronize());
    }
    return 0;
}
