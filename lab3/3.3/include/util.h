#ifndef CUDA_UTIL
#define CUDA_UTIL

#define CHECK(call)                                                            \
    do {                                                                       \
        cudaError_t err = call;                                                \
        if (err != cudaSuccess) {                                              \
            printf("CUDA error at %s:%d: %s\n", __FILE__, __LINE__,            \
                   cudaGetErrorString(err));                                   \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#endif // ! CUDA_UTIL
