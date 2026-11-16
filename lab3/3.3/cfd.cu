#include <cuda_runtime.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "arraymalloc.h"
#include "boundary.h"
#include "cfdio.h"
#include "jacobi.cuh"

int main(int argc, char **argv) {
    int printfreq = 1000; // output frequency
    double error, bnorm;
    double tolerance = 0.0; // tolerance for convergence. <=0 means do not check

    // main arrays
    double *psi;
    // temporary versions of main arrays
    double *psitmp;

    // command line arguments
    int scalefactor, numiter;

    // simulation sizes
    int bbase = 10;
    int hbase = 15;
    int wbase = 5;
    int mbase = 32;
    int nbase = 32;

    int irrotational = 1, checkerr = 0;

    int m, n, b, h, w;
    int iter;
    int i, j;

    double tstart, tstop, ttot, titer;

    // do we stop because of tolerance?
    if (tolerance > 0) {
        checkerr = 1;
    }

    // check command line parameters and parse them

    if (argc < 3 || argc > 4) {
        printf("Usage: cfd <scale> <numiter>\n");
        return 0;
    }

    scalefactor = atoi(argv[1]);
    numiter = atoi(argv[2]);

    if (!checkerr) {
        printf("Scale Factor = %i, iterations = %i\n", scalefactor, numiter);
    } else {
        printf("Scale Factor = %i, iterations = %i, tolerance= %g\n",
               scalefactor, numiter, tolerance);
    }

    printf("Irrotational flow\n");

    // Calculate b, h & w and m & n
    b = bbase * scalefactor;
    h = hbase * scalefactor;
    w = wbase * scalefactor;
    m = mbase * scalefactor;
    n = nbase * scalefactor;

    printf("Running CFD on %d x %d grid in serial\n", m, n);

    // allocate arrays
    psi = (double *)malloc((m + 2) * (n + 2) * sizeof(double));
    psitmp = (double *)malloc((m + 2) * (n + 2) * sizeof(double));

    // zero the psi array
    for (i = 0; i < m + 2; i++) {
        for (j = 0; j < n + 2; j++) {
            psi[i * (m + 2) + j] = 0.0;
        }
    }

    // set the psi boundary conditions
    boundarypsi(psi, m, n, b, h, w);

    // compute normalisation factor for error
    bnorm = 0.0;

    for (i = 0; i < m + 2; i++) {
        for (j = 0; j < n + 2; j++) {
            bnorm += psi[i * (m + 2) + j] * psi[i * (m + 2) + j];
        }
    }
    bnorm = sqrt(bnorm);

    // begin iterative Jacobi loop
    printf("\nStarting main loop...\n\n");
    tstart = gettime();

    for (iter = 1; iter <= numiter; iter++) {

        // calculate psi for next iteration
        jacobistep(psitmp, psi, m, n);

        // calculate current error if required
        if (checkerr || iter == numiter) {
            error = deltasq(psitmp, psi, m, n);

            error = sqrt(error);
            error = error / bnorm;
        }

        // quit early if we have reached required tolerance
        if (checkerr) {
            if (error < tolerance) {
                printf("Converged on iteration %d\n", iter);
                break;
            }
        }

        // copy back
        for (i = 1; i <= m; i++) {
            for (j = 1; j <= n; j++) {
                psi[i * (m + 2) + j] = psitmp[i * (m + 2) + j];
            }
        }

        // print loop information
        if (iter % printfreq == 0) {
            if (!checkerr) {
                printf("Completed iteration %d\n", iter);
            } else {
                printf("Completed iteration %d, error = %g\n", iter, error);
            }
        }
    } // iter

    if (iter > numiter)
        iter = numiter;

    tstop = gettime();

    ttot = tstop - tstart;
    titer = ttot / (double)iter;

    // print out some stats
    printf("\n... calculation done\n");
    printf("After %d iterations, the error is %g\n", iter, error);
    printf("Time for %d iterations was %g seconds\n", iter, ttot);
    printf("Each iteration took %g seconds\n", titer);

    // output results
    // writedatafiles(psi,m,n, scalefactor);
    // writeplotfile(m,n,scalefactor);
    double error_gpu = 0;
    printf("... finished serial part\n");
    printf("\n------------------------\n\n");

    // zero the psi array for parallel run
    for (i = 0; i < m + 2; i++) {
        for (j = 0; j < n + 2; j++) {
            psi[i * (m + 2) + j] = 0.0;
        }
    }

    boundarypsi(psi, m, n, b, h, w);

    double *d_psi, *d_psitemp, *d_res; // Maknut Err
    CHECK(cudaMalloc(&d_psi, (size_t)(m + 2) * (n + 2) * sizeof(double)));
    CHECK(cudaMalloc(&d_psitemp, (size_t)(m + 2) * (n + 2) * sizeof(double)));
    CHECK(cudaMalloc(&d_res, (size_t)sizeof(double)));

    double tstart_gpu, tstop_gpu, ttot_gpu, titer_gpu;
    tstart_gpu = gettime();

    CHECK(cudaMemcpy(d_psi, psi, (size_t)(m + 2) * (n + 2) * sizeof(double),
                     cudaMemcpyHostToDevice));

    CHECK(cudaMemcpy(d_psitemp, d_psi,
                     (size_t)(m + 2) * (n + 2) * sizeof(double),
                     cudaMemcpyDeviceToDevice));

    dim3 j_block(16, 16);
    int num_blocks_x = (n + j_block.x - 1) / j_block.x;
    int num_blocks_y = (m + j_block.y - 1) / j_block.y;
    dim3 j_grid(num_blocks_x, num_blocks_y);

    for (iter = 1; iter <= numiter; iter++) {

        parJacobistep<<<j_grid, j_block>>>(d_psitemp, d_psi, m, n);
        CHECK(cudaDeviceSynchronize());

        if (checkerr || iter == numiter) {

            CHECK(cudaMemset(d_res, 0, sizeof(double)));

            parDeltasq<<<j_grid, j_block>>>(d_psitemp, d_psi, d_res, m, n);
            CHECK(cudaDeviceSynchronize());

            CHECK(cudaMemcpy(&error_gpu, d_res, sizeof(double),
                             cudaMemcpyDeviceToHost));

            error_gpu = sqrt(error_gpu);
            error_gpu = error_gpu / bnorm;
        }

        if (checkerr) {
            if (error_gpu < tolerance) {
                printf("Converged on iteration %d\n", iter);
                break;
            }
        }

        double *temp = d_psi;
        d_psi = d_psitemp;
        d_psitemp = temp;

        // print loop information
        if (iter % printfreq == 0) {
            if (!checkerr) {
                printf("Completed iteration %d\n", iter);
            } else {
                printf("Completed iteration %d, error = %g\n", iter, error_gpu);
            }
        }
    }
    if (iter > numiter)
        iter = numiter;

    tstop_gpu = gettime();

    ttot_gpu = tstop_gpu - tstart_gpu;
    titer_gpu = ttot_gpu / (double)iter;

    // print out some stats
    printf("\n... calculation done GPU\n");
    printf("After %d iterations, the error is %g\n", iter, error_gpu);
    printf("Time for %d iterations was %g seconds\n", iter, ttot_gpu);
    printf("Each iteration took %g seconds\n", titer_gpu);
    if (fabs(error_gpu - error) < 1e-10) {
        printf("Error cpu and gpu match!\n");
    } else {
        printf("Error cpu and gpu DON'T match!\n");
    }
    double speedup = ttot / ttot_gpu;
    printf("SPEEDUP (CPU time / GPU time): %.2fx\n", speedup);
    printf("... finished GPU\n");
    // free un-needed arrays
    free(psi);
    free(psitmp);

    CHECK(cudaFree(d_psitemp));
    CHECK(cudaFree(d_psi));
    CHECK(cudaFree(d_res));

    return 0;
}
