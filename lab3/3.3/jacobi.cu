#include <cuda_runtime.h>
#include <stdio.h>

#include "jacobi.cuh"

void jacobistep(double *psinew, double *psi, int m, int n) {
    int i, j;

    for (i = 1; i <= m; i++) {
        for (j = 1; j <= n; j++) {
            psinew[i * (m + 2) + j] =
                0.25 *
                (psi[(i - 1) * (m + 2) + j] + psi[(i + 1) * (m + 2) + j] +
                 psi[i * (m + 2) + j - 1] + psi[i * (m + 2) + j + 1]);
        }
    }
}

double deltasq(double *newarr, double *oldarr, int m, int n) {
    int i, j;

    double dsq = 0.0;
    double tmp;

    for (i = 1; i <= m; i++) {
        for (j = 1; j <= n; j++) {
            tmp = newarr[i * (m + 2) + j] - oldarr[i * (m + 2) + j];
            dsq += tmp * tmp;
        }
    }

    return dsq;
}
__global__ void parJacobistep(double *psinew, double *psi, int m, int n) {
    int tid_col = threadIdx.x;
    int tid_row = threadIdx.y;
    int block_row = blockIdx.y;
    int block_col = blockIdx.x;
    int threads_col = blockDim.x;
    int threads_row = blockDim.y;

    int j = block_col * threads_col + tid_col; // col
    int i = block_row * threads_row + tid_row; // row
    ++i;
    ++j; // boundary conditions

    if (j <= n && i <= m) {
        psinew[i * (m + 2) + j] =
            0.25 * (psi[(i - 1) * (m + 2) + j] + psi[(i + 1) * (m + 2) + j] +
                    psi[i * (m + 2) + j - 1] + psi[i * (m + 2) + j + 1]);
    }
}

__global__ void parDeltasq(double *newarr, double *oldarr, double *res, int m,
                           int n) {
    int j = blockIdx.x * blockDim.x + threadIdx.x;
    int i = blockIdx.y * blockDim.y + threadIdx.y;
    ++i;
    ++j;

    if (j <= n && i <= m) {
        int idx = i * (m + 2) + j;
        double tmp = newarr[idx] - oldarr[idx];
        double val = tmp * tmp;

        atomicAdd(res, val);
    }
}
