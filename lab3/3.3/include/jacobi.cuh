// #include <nvtx3/nvToolsExt.h>
#define BLOCK_X 16
#define BLOCK_Y 16

void jacobistep(double *psinew, double *psi, int m, int n);

double deltasq(double *newarr, double *oldarr, int m, int n);
__global__ void parJacobistep(double *psinew, double *psi, int m, int n);
__global__ void parDeltasq(double *newarr, double *oldarr, double *res, int m,
                           int n);
