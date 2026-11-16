import numpy as np
import time
from numba import guvectorize, float64, int64, cuda


@guvectorize(["void(int64[:], int64, float64[:])"], "(m),()->()", target="cuda")
def pi_sum_gu(i_block, N, out):
    tmp = 0.0
    for i in range(i_block.shape[0]):
        x = (i_block[i] - 0.5) / N
        tmp += 1.0 / (1.0 + x * x)
    out[0] = tmp


def run_gu_test(N, block_size):
    print(f"Velicina bloka: {block_size:<5}")
    for i in range(5):
        arr_2d = np.arange(1, N + 1, dtype=np.int64).reshape(-1, block_size)
        t1 = time.perf_counter()
        result = pi_sum_gu(arr_2d, N)
        cuda.synchronize()
        t2 = time.perf_counter()

        pi_val = result.sum() * 4 / N
        elapsed_ms = (t2 - t1) * 1000.0

        print(f"    [Dobiveni PI]: {pi_val:.7f} | [Vrijeme]: {elapsed_ms:.2f} ms")
        pi_val = 0


if __name__ == "__main__":
    print(f"CUDA status: {cuda.is_available()}")

    N = 2**28

    for size in [1024, 2048, 4096, 8192]:
        run_gu_test(N, size)
