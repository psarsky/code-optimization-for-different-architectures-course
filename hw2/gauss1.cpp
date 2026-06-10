#include "gauss_common.hpp"

void eliminate_v1(double* A, double* b, int n) {
    for (int k = 0; k < n; ++k) {
        const double pivot = A[(std::size_t)k * n + k];
        for (int i = k + 1; i < n; ++i) {
            const double factor = A[(std::size_t)i * n + k] / pivot;
            for (int j = k; j < n; ++j) {
                A[(std::size_t)i * n + j] -= factor * A[(std::size_t)k * n + j];
            }
            b[i] -= factor * b[k];
        }
    }
}

int main(int argc, char** argv) {
    return gauss::run_cli("gauss1", eliminate_v1, argc, argv);
}
