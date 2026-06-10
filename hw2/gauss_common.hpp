#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace gauss {

using ElimFn = void (*)(double* A, double* b, int n);

inline void make_system(std::vector<double>& A, std::vector<double>& b, int n, std::uint32_t seed = 12345) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(-1.0, 1.0);

    A.assign(static_cast<std::size_t>(n) * n, 0.0);
    b.assign(n, 0.0);

    for (int i = 0; i < n; ++i) {
        double off_sum = 0.0;
        for (int j = 0; j < n; ++j) {
            if (i == j) {
                continue;
            }
            double v = dist(rng);
            A[static_cast<std::size_t>(i) * n + j] = v;
            off_sum += std::fabs(v);
        }
        // Dominacja diagonalna: |a_ii| > suma |a_ij| dla j != i.
        A[static_cast<std::size_t>(i) * n + i] = off_sum + 1.0;
        b[i] = dist(rng);
    }
}

inline void back_substitution(const double* A, const double* b, double* x, int n) {
    for (int i = n - 1; i >= 0; --i) {
        double sum = b[i];
        for (int j = i + 1; j < n; ++j) {
            sum -= A[static_cast<std::size_t>(i) * n + j] * x[j];
        }
        x[i] = sum / A[static_cast<std::size_t>(i) * n + i];
    }
}

inline void reference_eliminate(double* A, double* b, int n) {
    for (int k = 0; k < n; ++k) {
        const double pivot = A[static_cast<std::size_t>(k) * n + k];
        for (int i = k + 1; i < n; ++i) {
            const double factor = A[static_cast<std::size_t>(i) * n + k] / pivot;
            for (int j = k; j < n; ++j) {
                A[static_cast<std::size_t>(i) * n + j] -=
                    factor * A[static_cast<std::size_t>(k) * n + j];
            }
            b[i] -= factor * b[k];
        }
    }
}

inline std::vector<double> solve(ElimFn elim, const std::vector<double>& A0, const std::vector<double>& b0, int n) {
    std::vector<double> A = A0;
    std::vector<double> b = b0;
    std::vector<double> x(n, 0.0);
    elim(A.data(), b.data(), n);
    back_substitution(A.data(), b.data(), x.data(), n);
    return x;
}

inline double elimination_flops(int n) {
    double flops = 0.0;
    for (int k = 0; k < n; ++k) {
        const double rows = static_cast<double>(n - k - 1);
        const double cols = static_cast<double>(n - k);
        flops += rows * (2.0 * cols + 1.0);
    }
    return flops;
}

inline double now_seconds() {
    using clock = std::chrono::steady_clock;
    return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
}

inline double max_abs_diff(const std::vector<double>& a, const std::vector<double>& b) {
    double m = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        m = std::max(m, std::fabs(a[i] - b[i]));
    }
    return m;
}

inline double residual_inf(const std::vector<double>& A, const std::vector<double>& b, const std::vector<double>& x, int n) {
    double m = 0.0;
    for (int i = 0; i < n; ++i) {
        double s = -b[i];
        for (int j = 0; j < n; ++j) {
            s += A[static_cast<std::size_t>(i) * n + j] * x[j];
        }
        m = std::max(m, std::fabs(s));
    }
    return m;
}

inline int run_cli(const char* name, ElimFn elim, int argc, char** argv) {
    int n = 1024;
    int repeats = 3;
    if (argc >= 2) {
        n = std::atoi(argv[1]);
    }
    if (argc >= 3) {
        repeats = std::atoi(argv[2]);
    }
    if (n <= 0 || repeats <= 0) {
        std::cerr << "usage: " << name << " <n> [repeats]\n";
        return 1;
    }

    std::vector<double> A0;
    std::vector<double> b0;
    make_system(A0, b0, n);

    const std::vector<double> x_ref = solve(reference_eliminate, A0, b0, n);

    double best_s = 1e300;
    double sum_s = 0.0;
    std::vector<double> x_opt;
    for (int r = 0; r < repeats; ++r) {
        std::vector<double> A = A0;
        std::vector<double> b = b0;
        const double t0 = now_seconds();
        elim(A.data(), b.data(), n);
        const double t1 = now_seconds();
        const double dt = t1 - t0;
        sum_s += dt;
        best_s = std::min(best_s, dt);
        if (r == repeats - 1) {
            std::vector<double> x(n, 0.0);
            back_substitution(A.data(), b.data(), x.data(), n);
            x_opt = std::move(x);
        }
    }

    const double avg_s = sum_s / repeats;
    const double flops = elimination_flops(n);
    const double gflops_avg = flops / avg_s / 1e9;
    const double gflops_best = flops / best_s / 1e9;

    const double diff = max_abs_diff(x_ref, x_opt);
    const double res = residual_inf(A0, b0, x_opt, n);
    const bool ok = diff < 1e-12 && res < 1e-12;

    std::cout << name
              << " n=" << n
              << " avg_ms=" << (avg_s * 1e3)
              << " best_ms=" << (best_s * 1e3)
              << " gflops_avg=" << gflops_avg
              << " gflops_best=" << gflops_best
              << " max_diff=" << diff
              << " residual=" << res
              << " status=" << (ok ? "OK" : "FAIL")
              << "\n";

    return ok ? 0 : 2;
}

}
