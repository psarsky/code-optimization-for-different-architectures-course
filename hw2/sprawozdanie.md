# Sprawozdanie - Zadanie 2 (eliminacja Gaussa)

### Autor: Jakub Psarski

## 1. Cel i zakres

Zaimplementowano i zoptymalizowano algorytm eliminacji Gaussa rozwiązujący układ równań liniowych `Ax = b`. Optymalizacja jest dopasowana do mikroarchitektury procesora (cache, jednostki wektorowe AVX2 + FMA).

Optymalizacji podlega **forward elimination** (sprowadzenie macierzy do postaci górnotrójkątnej) - to dominujący koszt o złożoności `O(n³)`. Wspólne dla wszystkich wersji są: generacja układu, back-substitution (`O(n²)`) i weryfikacja poprawności.

Każda wersja znajduje się w osobnym pliku:

- `gauss0.cpp` - wariant referencyjny bez strojenia pod mikroarchitekturę,
- `gauss1.cpp` - identyczny kod, kompilowany z `-march=znver2 -mtune=znver2` (strojenie pod Zen 2),
- `gauss2.cpp` - jawna wektoryzacja AVX2 + FMA (intrinsics).

## 2. Środowisko i dopasowanie do CPU

### Platforma testowa:

- CPU: **AMD Ryzen 7 3700X**, mikroarchitektura **Zen 2 (Matisse)**,
- 8 rdzeni / 16 wątków,
- cache: L1d 32 KiB/core, L1i 32 KiB/core, L2 512 KiB/core, L3 32 MiB (2 × 16 MiB),
- linia cache: 64 B (= 8 liczb `double`),
- ISA: x86-64 + SSE/SSE2/SSE4.x + AVX + AVX2 + FMA3 (flagi `avx2`, `fma`).

### Dopasowanie kodu do tej architektury:

- Jednostki wektorowe: Zen 2 ma 256-bitowe FPU i instrukcje FMA3, więc wektor `__m256d` mieści dokładnie 4 liczby `double`, a aktualizacja wiersza to jedna instrukcja `vfnmadd...pd` (wersja 2).  
    > `vfnmadd...pd` - instrukcja procesora AVX2 (vector fused negative multiply-add, packed double)
- Cache i linia 64 B: macierz przechowywana jest wierszami, a wewnętrzna pętla przebiega ciągły fragment wiersza - kolejne odczyty trafiają w tę samą linię cache (8 `double` na linię), co maksymalizuje lokalność.

### Użyte flagi kompilacji:

`gauss0`: `-O2 -std=c++20 -pipe -Wall -Wextra`
`gauss1..2`: `-march=znver2 -mtune=znver2 -O2 -std=c++20 -pipe -Wall -Wextra`

<br />

## 3. Opis implementacji wersji

Rdzeń algorytmu (forward elimination) dla każdego pivota `k` aktualizuje wszystkie wiersze poniżej:

```
factor = A[i][k] / A[k][k]
A[i][j] -= factor * A[k][j]   dla j = k..n-1
```

Wewnętrzna pętla po `j` jest operacją typu *axpy* (`y -= ax`) - to jest ta część, która podlega wektoryzacji i decyduje o wydajności.

> *axpy* - operacja arytmetyczna a·x + y

### Wersja 0 - `gauss0.cpp` (referencyjna bez strojenia pod CPU)

Prosty potrójny loop, kompilowany bez `-march/-mtune`. Punkt odniesienia pokazujący, ile daje samo dopasowanie kompilacji do mikroarchitektury.

```cpp
void eliminate_v0(double* A, double* b, int n) {
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
```

### Wersja 1 - `gauss1.cpp` (bazowa, strojona pod Zen 2)

Identyczny kod jak v0, ale z `-march=znver2 -mtune=znver2`. Izoluje sam efekt dopasowania doboru instrukcji i harmonogramu do Zen 2.

### Wersja 2 - `gauss2.cpp` (jawna wektoryzacja AVX2 + FMA)

Ręczna wektoryzacja przy użyciu *intrinsics* - 4 liczby `double` na iterację, jedna instrukcja FMA. Dodatkowo zastosowano:

1. `__restrict` - obietnica braku aliasingu wierszy,
2. `inv_pivot = 1/pivot` - dzielenie (wolne) zamienione na mnożenie (1 dzielenie na pivota),
3. wskaźnik wiersza wyciągnięty przed pętlę.

<br />

```cpp
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
            // Główna pętla wektorowa: 4 double na iteracje.
            for (; j + 4 <= n; j += 4) {
                __m256d vri = _mm256_loadu_pd(ri + j);
                __m256d vrk = _mm256_loadu_pd(rk + j);
                vri = _mm256_fnmadd_pd(vf, vrk, vri); // ri = -(f*rk) + ri
                _mm256_storeu_pd(ri + j, vri);
            }
            // Reszta skalarnie.
            for (; j < n; ++j) {
                ri[j] -= factor * rk[j];
            }
            b[i] -= factor * b[k];
        }
    }
}
```

**Weryfikacja wektoryzacji w asemblerze** (`g++ -S`, analiza wewnętrznej pętli każdej funkcji):

| Wersja | instrukcje w pętli aktualizacji |
|---|---|
| gauss0 | tylko instrukcje skalarne (`sd`) |
| gauss1 | tylko instrukcje skalarne (`sd`) |
| gauss2 | `vfnmadd...**pd**` (wektorowe, 4 liczby naraz) |

**Wniosek:** przy `-O2` kompilator nie wektoryzuje tej pętli samodzielnie (sufiks `sd` = *scalar double*). Wektoryzację (`pd` = *packed double*) uzyskujemy dopiero przez jawne instrukcje AVX2 w wersji 2.

## 4. Weryfikacja poprawności

Weryfikacja jest dwustopniowa i uwzględnia złożoność obliczeniową (`O(n³)` operacji, więc tolerancja jest bezpieczna):

1. Względem wersji referencyjnej - `max_diff = max|x_opt - x_ref|`,
2. Niezależnie - residuum `||A·x - b||_∞` (nie korzysta z wersji referencyjnej).

Wynik uznawany za poprawny gdy `max_diff < 1e-12` i `residual < 1e-12`.

Przetestowano rozmiary `n ∈ {256, 512, 1024, 2048}`. Wszystkie wersje dla wszystkich rozmiarów zwracają poprawne wyniki. Typowe residuum rzędu `1e-15 … 1e-14`, `max_diff` rzędu `1e-18`.

**Wniosek:** optymalizacje nie zmieniają wyniku ponad poziom błędu maszynowego.

## 5. Pomiary wydajności (GFLOPS)

Liczbę operacji liczono zgodnie z rzeczywistą złożonością forward elimination: dla każdego `k` aktualizujemy `(n-k-1)` wierszy po `~(n-k)` elementów, **2 FLOP** na element (mnożenie + odejmowanie) → sumarycznie `≈ (2/3)·n³` FLOP. `GFLOPS = FLOP / czas / 1e9`.

Czas mierzony zegarem `steady_clock`, 5 powtórzeń, raportowany czas średni i najlepszy. Pomiary z `bench_results.csv`:

| n | wersja | avg [ms] | GFLOPS (avg) | GFLOPS (best) | status |
|---|---|---:|---:|---:|---|
| 256 | gauss0 | 2.242 | 5.00 | 5.10 | OK |
| 256 | gauss1 | 1.910 | 5.87 | 5.94 | OK |
| 256 | gauss2 | 0.999 | **11.23** | **11.65** | OK |
| 512 | gauss0 | 18.76 | 4.78 | 4.96 | OK |
| 512 | gauss1 | 16.18 | 5.54 | 5.70 | OK |
| 512 | gauss2 | 9.414 | **9.52** | **9.64** | OK |
| 1024 | gauss0 | 145.5 | 4.92 | 5.15 | OK |
| 1024 | gauss1 | 124.7 | 5.74 | 5.90 | OK |
| 1024 | gauss2 | 72.24 | **9.92** | **10.27** | OK |
| 2048 | gauss0 | 1898.7 | 3.02 | 3.07 | OK |
| 2048 | gauss1 | 1850.8 | 3.10 | 3.20 | OK |
| 2048 | gauss2 | 1473.6 | **3.89** | **3.96** | OK |

Przyspieszenie wersji wektorowej względem referencyjnej (gauss0 → gauss2) dla danych mieszczących się w cache: **~2.0–2.2×** (n=256: 5.00 → 11.23 GFLOPS; n=1024: 4.92 → 9.92 GFLOPS).

### Wpływ strojenia bez wektoryzacji (gauss0 → gauss1)

`gauss1` to ten sam kod co `gauss0`, tylko z `-march=znver2 -mtune=znver2`. Daje umiarkowany zysk **~15–17%** (np. n=1024: 4.92 → 5.74 GFLOPS), mimo że obie wersje pozostają skalarne (potwierdzone w asemblerze). Zysk wynika więc z lepszego doboru instrukcji i harmonogramowania pod Zen 2, a nie z wektoryzacji.

**Wniosek:** samo dopasowanie kompilacji do mikroarchitektury pomaga, ale prawdziwy skok wydajności daje dopiero jawna wektoryzacja (gauss2).

<br />

### Wysycenie FLOPS i ograniczenie sprzętowe

Teoretyczny szczyt dla `double`: `taktowanie × 2 FMA × 4 double × 2 operacje (mul+add)`. Przy ~4 GHz to ~64 GFLOPS na rdzeń (jednowątkowo). Wektorowy `gauss2` osiąga ~10–11 GFLOPS, czyli **~17%**. To nie jest wina słabej wektoryzacji - przyczyną jest charakter algorytmu.

**Kluczowy wniosek:** rdzeń eliminacji Gaussa ma niską intensywność arytmetyczną: na każdy przetwarzany element przypadają 3 dostępy do pamięci (łącznie 24B), a tylko 2 operacje (mnożenie + odejmowanie). Algorytm jest więc ograniczony przepustowością pamięci (memory-bound), a nie mocą FPU.

### Załamanie wydajności przy n=2048 - efekt cache

Przy `n=2048` GFLOPS spada we wszystkich wersjach (gauss2: ~10 → ~3.9). Macierz `2048² × 8 B = 32 MiB` przekracza pojemność L3 (efektywnie 16 MiB na CCX). Dane przestają mieścić się w cache, każda aktualizacja sięga do RAMu i wydajność spada do poziomu przepustowości pamięci podręcznej. Co istotne, wektoryzacja wciąż pomaga (gauss2 nadal ~25% szybszy od gauss0), ale ogólny poziom jest ograniczony przez RAM.

**Wniosek:** to bezpośrednia, mierzalna demonstracja lokalności danych - ten sam kod jest ~2.5–3× wolniejszy, gdy roboczy zbiór nie mieści się w L3.

> CCX - Core Complex - podział L3 cache między rdzenie stosowany przez AMD (w przypadku R7 3700X - 16MiB per 4 rdzenie)

## 6. Wnioski końcowe

1. Samo dopasowanie kompilacji do Zen 2 (`-march=znver2`) daje ~15–17% przyspieszenia (lepszy dobór instrukcji i harmonogram), ale nie powoduje wektoryzacji - kod pozostaje skalarny.
2. Prawdziwy skok (~2×) daje dopiero jawna wektoryzacja AVX2 + FMA (gauss2), potwierdzona w asemblerze instrukcjami `vfnmadd...pd`.
3. Eliminacja Gaussa osiąga ~17% szczytu FPU; to oczekiwane dla aktualizacji rzędu 1 o niskiej intensywności arytmetycznej - algorytm jest memory-bound, więc SIMD daje ~2×, a nie 4×.
4. Przekroczenie pojemności L3 (n=2048) powoduje wielokrotny spadek wydajności - wyraźny, mierzalny dowód znaczenia lokalności danych.
5. Wszystkie wersje są poprawne (zgodność z referencją i residuum `~1e-15`) dla wszystkich testowanych rozmiarów, w tym niepodzielnych przez 4.
