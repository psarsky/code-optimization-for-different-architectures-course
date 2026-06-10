#include "gauss_common.hpp"
#include <immintrin.h>

void eliminate_v2(double* __restrict A, double* __restrict b, int n) {
    for (int k = 0; k < n; ++k) {
        const double* __restrict rk = A + (std::size_t)k * n;
        const double inv_pivot = 1.0 / rk[k];
        for (int i = k + 1; i < n; ++i) {
            double* __restrict ri = A + (std::size_t)i * n;
            const double factor = ri[k] * inv_pivot;
            ri[k] = 0.0;

            const __m256d vf = _mm256_set1_pd(factor);
            int j = k + 1;
            for (; j + 4 <= n; j += 4) {
                __m256d vri = _mm256_loadu_pd(ri + j);
                __m256d vrk = _mm256_loadu_pd(rk + j);
                vri = _mm256_fnmadd_pd(vf, vrk, vri);
                _mm256_storeu_pd(ri + j, vri);
            }
            for (; j < n; ++j) {
                ri[j] -= factor * rk[j];
            }
            b[i] -= factor * b[k];
        }
    }
}

int main(int argc, char** argv) {
    return gauss::run_cli("gauss2", eliminate_v2, argc, argv);
}
