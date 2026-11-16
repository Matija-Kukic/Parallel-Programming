import numpy as np
import time
from numba import vectorize, float64

# samo ako postoji nvidia GPU, također treba instalirati paket numba-cuda
from numba import cuda

print(cuda.is_available())  # provjera da CUDA radi


@vectorize(["float64(int64, int64)"], target="cuda")
def pi_sum_gpu(i, N):
    x = (i - 0.5) / N
    return 1.0 / (1.0 + x * x)


N = 2**28

for i in range(5):

    arr = np.arange(1, N + 1, dtype=np.int64)
    t1 = time.perf_counter()
    result = pi_sum_gpu(arr, N)
    t2 = time.perf_counter()
    print(
        f"[Dobiveni PI]: {result.sum() * 4 / N:.7f}\n[Vrijeme izvodjenja]: {(t2 - t1) * 1000:.7f} ms"
    )
    result = 0
