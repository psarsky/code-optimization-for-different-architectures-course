# Sprawozdanie - Zadanie 1 (normalizacja tekstu)

### Autor: Jakub Psarski

## 1. Cel i zakres

Zaimplementowano funkcje normalizacji dużego tekstu wykonujące:

1. usuwanie znaków spoza zakresu drukowalnego ASCII (`< 32` lub `> 126`),
2. zamianę sekwencji białych znaków na pojedynczą spację,
3. konwersję liter do małych,
4. konwersję interpunkcji na przecinki,
5. usuwanie duplikatów wyrazów występujących bezpośrednio po sobie.

Każda wersja optymalizacji znajduje się w osobnym pliku:

- `normalize0.cpp` - wariant referencyjny bez strojenia pod mikroarchitekturę (`-march/-mtune`),
- `normalize1.cpp` - wersja bazowa,
- `normalize2.cpp` - prealokacja pamięci,
- `normalize3.cpp` - in-place transformation,
- `normalize4.cpp` - iteratory i algorytmy standardowe,
- `normalize5.cpp` - bare-metal (wskaźniki + `memcpy`),
- `normalize6.cpp` - wersja równoległa (`std::thread`).

## 2. Środowisko i dopasowanie do CPU

Platforma testowa:

- CPU: **AMD Ryzen 7 3700X**, mikroarchitektura **Zen 2 (Matisse)**,
- 8 rdzeni / 16 wątków,
- cache: L1d 32 KiB/core, L1i 32 KiB/core, L2 512 KiB/core, L3 32 MiB,
- linia cache: 64 B,
- ISA: x86-64 + SSE/SSE2/SSE4.x + AVX + AVX2 + FMA.

Użyte flagi kompilacji:

`normalize0`: `-std=c++20 -pipe -Wall -Wextra -Wpedantic`  
`normalize1..6`: `-march=znver2 -mtune=znver2 -std=c++20 -pipe -Wall -Wextra -Wpedantic` (+ `-pthread` dla wersji wielowątkowej)

W ten sposób można bezpośrednio porównać wpływ strojenia pod Zen 2. Dla `normalize1..6` kompilator generuje kod strojony pod Zen 2 (m.in. harmonogramowanie i dobór instrukcji AVX2/FMA tam, gdzie to możliwe), a `normalize0` jest punktem odniesienia bez strojenia `-march/-mtune`.

## 3. Opis implementacji wersji

### Wersja 0 - `normalize0.cpp` (referencyjna bez strojenia pod CPU)

Algorytmicznie to samo co wersja bazowa (`normalize1`), ale kompilowana bez `-march/-mtune`. Ta wersja służy tylko do porównania, co zmienia samo dopasowanie kompilacji do docelowej mikroarchitektury.

### Wersja 1 - `normalize1.cpp` (bazowa)

```c
std::string normalize_text_v1(const std::string& input) {
    std::string stage;
    bool in_whitespace = false;

    for (char ch : input) {
        const unsigned char c = static_cast<unsigned char>(ch);

        if (!normalize::is_printable_ascii(c)) {
            continue;
        }

        if (normalize::is_ascii_whitespace(c)) {
            if (!in_whitespace) {
                stage.push_back(' ');
                in_whitespace = true;
            }
            continue;
        }

        in_whitespace = false;
        char out = normalize::to_ascii_lower(c);
        if (std::ispunct(static_cast<unsigned char>(out)) != 0) {
            out = ',';
        }
        stage.push_back(out);
    }

    return normalize::deduplicate_adjacent_words(stage, false);
}

bool is_printable_ascii(unsigned char c) {
    return c >= 32 && c <= 126;
}

bool is_ascii_whitespace(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

char to_ascii_lower(unsigned char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return static_cast<char>(c);
}

bool is_word_char(unsigned char c) {
    return std::isalnum(c) != 0;
}

std::string deduplicate_adjacent_words(const std::string& input, bool preallocate_output) {
    std::string output;
    if (preallocate_output) {
        output.reserve(input.size());
    }

    std::string current_word;
    std::string previous_word;
    std::string pending_separator;

    std::size_t i = 0;
    while (i < input.size()) {
        pending_separator.clear();
        while (i < input.size() && !is_word_char(static_cast<unsigned char>(input[i]))) {
            pending_separator.push_back(input[i]);
            i++;
        }
        if (i >= input.size()) {
            break;
        }

        const std::size_t word_start = i;
        while (i < input.size() && is_word_char(static_cast<unsigned char>(input[i]))) {
            i++;
        }

        current_word = input.substr(word_start, i - word_start);
        if (current_word != previous_word) {
            output.append(pending_separator);
            output.append(current_word);
            previous_word = current_word;
        }
    }

    return output;
}
```

**Implementacja bazowa:** jedna pętla po wejściu, `push_back` do `stage`, bez `reserve` w głównym buforze.  
To oznacza, że gdy `stage` rośnie, `std::string` okresowo robi realokacje (nowy większy bufor + kopiowanie dotychczasowej zawartości). Dodatkowo jest drugi etap deduplikacji słów. Efekt: prosty kod, ale część czasu idzie na zarządzanie pamięcią, nie na samą logikę znaków.

### Wersja 2 - `normalize2.cpp` (prealokacja)

```c
std::string normalize_text_v2(const std::string& input) {
    std::string stage;
    stage.reserve(input.size());

    bool in_whitespace = false;
    for (char ch : input) {
        const unsigned char c = static_cast<unsigned char>(ch);

        if (!normalize::is_printable_ascii(c)) {
            continue;
        }

        if (normalize::is_ascii_whitespace(c)) {
            if (!in_whitespace) {
                stage.push_back(' ');
                in_whitespace = true;
            }
            continue;
        }

        in_whitespace = false;
        char out = normalize::to_ascii_lower(c);
        if (std::ispunct(static_cast<unsigned char>(out)) != 0) {
            out = ',';
        }
        stage.push_back(out);
    }

    return normalize::deduplicate_adjacent_words(stage, true);
}
```

Implementacja dodaje prealokację (`stage.reserve(input.size())`) i używa zoptymalizowanej deduplikacji z prealokacją wyniku.  
Dzięki temu można ograniczyć liczbę realokacji i kopiowań podczas budowania tekstu wynikowego. Logika transformacji znaków jest ta sama jak w v1, więc różnica wydajności wynika prawie wyłącznie z mniejszego narzutu pamięciowego.

**Wpływ na czas (wyniki z `bench_results.csv`):**
- `test_input_large.txt`: v1 **2462.300 ms**, v2 **2398.460 ms** (~**2.6% szybciej**),
- `test_input_low_mod.txt`: v1 **1711.390 ms**, v2 **1677.370 ms** (~**2.0% szybciej**),
- `test_input_mid_mod.txt`: v1 **1652.100 ms**, v2 **1618.900 ms** (~**2.0% szybciej**),
- `test_input_high_mod.txt`: v1 **1609.700 ms**, v2 **1578.200 ms** (~**2.0% szybciej**).

Wniosek: prealokacja daje minimalny zysk względem wersji bazowej.

### Wersja 3 - `normalize3.cpp` (in-place)

```c
std::string normalize_text_v3(std::string input) {
    std::size_t write_index = 0;
    bool in_whitespace = false;

    for (std::size_t read_index = 0; read_index < input.size(); ++read_index) {
        const unsigned char c = static_cast<unsigned char>(input[read_index]);

        if (!normalize::is_printable_ascii(c)) {
            continue;
        }

        if (normalize::is_ascii_whitespace(c)) {
            if (!in_whitespace) {
                input[write_index++] = ' ';
                in_whitespace = true;
            }
            continue;
        }

        in_whitespace = false;
        char out = normalize::to_ascii_lower(c);
        if (std::ispunct(static_cast<unsigned char>(out)) != 0) {
            out = ',';
        }
        input[write_index++] = out;
    }
    input.resize(write_index);

    std::size_t read_index = 0;
    write_index = 0;
    std::string previous_word;

    while (read_index < input.size()) {
        const std::size_t separator_start = read_index;
        while (read_index < input.size() &&
               !normalize::is_word_char(static_cast<unsigned char>(input[read_index]))) {
            ++read_index;
        }
        const std::size_t separator_end = read_index;

        if (read_index >= input.size()) {
            break;
        }

        const std::size_t word_start = read_index;
        while (read_index < input.size() &&
               normalize::is_word_char(static_cast<unsigned char>(input[read_index]))) {
            ++read_index;
        }
        const std::string_view word(&input[word_start], read_index - word_start);

        if (word != previous_word) {
            const std::size_t separator_len = separator_end - separator_start;
            if (separator_len != 0U) {
                std::memmove(&input[write_index], &input[separator_start], separator_len);
                write_index += separator_len;
            }
            std::memmove(&input[write_index], &input[word_start], word.size());
            write_index += word.size();
            previous_word.assign(word.data(), word.size());
        }
    }

    input.resize(write_index);
    return input;
}
```

Implementacja przechodzi na model in-place: funkcja przyjmuje `input` przez wartość i nadpisuje ten sam bufor indeksami read/write.  
Dzięki temu normalizacja znaków i deduplikacja sąsiednich słów wykonują się na tym samym stringu, a wynik jest finalnie przycinany (`resize`) do faktycznej długości. W fazie deduplikacji używane są przesunięcia `memmove`, co ogranicza liczbę alokacji i kopiowań.

**Wpływ na czas (wyniki z `bench_results.csv`):**
- `test_input_large.txt`: v1 **2462.300 ms**, v3 **1165.800 ms** (~**52.7% szybciej**),
- `test_input_low_mod.txt`: v1 **1711.390 ms**, v3 **818.250 ms** (~**52.2% szybciej**),
- `test_input_mid_mod.txt`: v1 **1652.100 ms**, v3 **784.606 ms** (~**52.5% szybciej**),
- `test_input_high_mod.txt`: v1 **1609.700 ms**, v3 **740.106 ms** (~**54.0% szybciej**).

Wniosek: to najsilniejsza optymalizacja w całym zadaniu i najszybsza wersja we wszystkich testach.

### Wersja 4 - `normalize4.cpp` (algorytmy STL)

```c
std::string normalize_text_v4(const std::string& input) {
    std::string stage;
    stage.reserve(input.size());

    std::copy_if(
        input.begin(), input.end(), std::back_inserter(stage),
        [](char ch) { return normalize::is_printable_ascii(static_cast<unsigned char>(ch)); });

    std::transform(stage.begin(), stage.end(), stage.begin(), [](char ch) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (normalize::is_ascii_whitespace(c)) {
            return ' ';
        }
        char out = normalize::to_ascii_lower(c);
        if (std::ispunct(static_cast<unsigned char>(out)) != 0) {
            return ',';
        }
        return out;
    });

    const auto unique_end = std::unique(stage.begin(), stage.end(), [](char left, char right) {
        return left == ' ' && right == ' ';
    });
    stage.erase(unique_end, stage.end());

    return normalize::deduplicate_adjacent_words(stage, false);
}
```

Wersja przepisuje główną logikę na algorytmy standardowe (`std::copy_if`, `std::transform`, `std::unique`) i iteratory.  
Kod jest bardziej deklaratywny i czytelny, ale kosztuje to dodatkowe pełne przejścia po danych oraz więcej pracy na pośrednich etapach transformacji.

**Wpływ na czas (wyniki z `bench_results.csv`):**
- `test_input_large.txt`: v1 **2462.300 ms**, v4 **3358.930 ms** (~**36.4% wolniej**),
- `test_input_low_mod.txt`: v1 **1711.390 ms**, v4 **2289.030 ms** (~**33.8% wolniej**),
- `test_input_mid_mod.txt`: v1 **1652.100 ms**, v4 **2183.810 ms** (~**32.2% wolniej**),
- `test_input_high_mod.txt`: v1 **1609.700 ms**, v4 **2105.610 ms** (~**30.8% wolniej**).

Wniosek: poprawa czytelności kodu przełożyła się negatywnie na wydajność dla tego charakteru obciążenia.

### Wersja 5 - `normalize5.cpp` (bare-metal)

```c
std::string normalize_text_v5(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    auto buffer = std::make_unique<char[]>(input.size());
    const unsigned char* read_ptr = reinterpret_cast<const unsigned char*>(input.data());
    const unsigned char* read_end = read_ptr + input.size();
    char* write_ptr = buffer.get();

    bool in_whitespace = false;
    while (read_ptr < read_end) {
        const unsigned char c = *read_ptr++;

        if (!normalize::is_printable_ascii(c)) {
            continue;
        }

        if (normalize::is_ascii_whitespace(c)) {
            if (!in_whitespace) {
                *write_ptr++ = ' ';
                in_whitespace = true;
            }
            continue;
        }

        in_whitespace = false;
        char out = normalize::to_ascii_lower(c);
        if (std::ispunct(static_cast<unsigned char>(out)) != 0) {
            out = ',';
        }
        *write_ptr++ = out;
    }

    const std::size_t compact_size = static_cast<std::size_t>(write_ptr - buffer.get());
    std::string stage(compact_size, '\0');
    if (compact_size != 0U) {
        std::memcpy(stage.data(), buffer.get(), compact_size);
    }

    return normalize::deduplicate_adjacent_words(stage, false);
}
```

Wersja bare-metal realizuje główną pętlę na surowych wskaźnikach (`char*`) i buforze zaalokowanym na surowej tablicy znaków.  
Po zakończeniu etapu filtrowania/normalizacji wynik jest kopiowany `memcpy` do `std::string`, a następnie wykonywana jest deduplikacja. Dzięki temu maleje narzut abstrakcji STL w najbardziej gorącej części kodu.

**Wpływ na czas (wyniki z `bench_results.csv`):**
- `test_input_large.txt`: v1 **2462.300 ms**, v5 **1571.240 ms** (~**36.2% szybciej**),
- `test_input_low_mod.txt`: v1 **1711.390 ms**, v5 **1142.800 ms** (~**33.2% szybciej**),
- `test_input_mid_mod.txt`: v1 **1652.100 ms**, v5 **1096.880 ms** (~**33.6% szybciej**),
- `test_input_high_mod.txt`: v1 **1609.700 ms**, v5 **1072.020 ms** (~**33.4% szybciej**).

Wniosek: wersja bare-metal daje duży zysk względem bazowej, ale nadal przegrywa z v3 (in-place) przez koszt dodatkowego bufora i kopiowania.

### Wersja 6 - `normalize6.cpp` (równoległa)

```c
std::string normalize_chunk(const std::string& input, std::size_t begin, std::size_t end) {
    std::string local;
    local.reserve(end - begin);

    bool in_whitespace = false;
    for (std::size_t i = begin; i < end; ++i) {
        const unsigned char c = static_cast<unsigned char>(input[i]);

        if (!normalize::is_printable_ascii(c)) {
            continue;
        }

        if (normalize::is_ascii_whitespace(c)) {
            if (!in_whitespace) {
                local.push_back(' ');
                in_whitespace = true;
            }
            continue;
        }

        in_whitespace = false;
        char out = normalize::to_ascii_lower(c);
        if (std::ispunct(static_cast<unsigned char>(out)) != 0) {
            out = ',';
        }
        local.push_back(out);
    }

    return local;
}

std::string normalize_text_v6(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    const unsigned int hw_threads = std::max(1U, std::thread::hardware_concurrency());
    const std::size_t min_chunk_size = 1U << 20;
    std::size_t chunk_count = (input.size() + min_chunk_size - 1U) / min_chunk_size;
    chunk_count = std::max<std::size_t>(1, std::min<std::size_t>(chunk_count, hw_threads));

    if (chunk_count == 1) {
        return normalize::deduplicate_adjacent_words(normalize_chunk(input, 0, input.size()), false);
    }

    std::vector<std::string> partial_results(chunk_count);
    std::vector<std::thread> workers;
    workers.reserve(chunk_count);

    const std::size_t base_chunk = input.size() / chunk_count;
    std::size_t begin = 0;
    for (std::size_t idx = 0; idx < chunk_count; ++idx) {
        const std::size_t end = (idx + 1 == chunk_count) ? input.size() : begin + base_chunk;
        workers.emplace_back([&, idx, begin, end]() {
            partial_results[idx] = normalize_chunk(input, begin, end);
        });
        begin = end;
    }

    for (std::thread& worker : workers) {
        worker.join();
    }

    std::string merged;
    merged.reserve(input.size());
    for (std::size_t i = 0; i < partial_results.size(); ++i) {
        const std::string& part = partial_results[i];
        if (part.empty()) {
            continue;
        }
        if (!merged.empty() && merged.back() == ' ' && part.front() == ' ') {
            merged.append(part.begin() + 1, part.end());
        } else {
            merged.append(part);
        }
    }

    std::string collapsed;
    collapsed.reserve(merged.size());
    bool in_whitespace = false;
    for (char ch : merged) {
        if (ch == ' ') {
            if (!in_whitespace) {
                collapsed.push_back(' ');
                in_whitespace = true;
            }
            continue;
        }
        in_whitespace = false;
        collapsed.push_back(ch);
    }

    return normalize::deduplicate_adjacent_words(collapsed, false);
}
```

Wersja równoległa dzieli wejście na fragmenty i normalizuje je równolegle (`std::thread`) w osobnych chunkach. Następnie wyniki są scalane, redukowane są graniczne duplikaty separatorów i wykonywana jest finalna deduplikacja słów.  
Liczba wątków jest ograniczana przez `hardware_concurrency` i minimalny rozmiar chunku, aby nie tworzyć nadmiernego narzutu dla małych danych.

**Wpływ na czas (wyniki z `bench_results.csv`):**
- `test_input_large.txt`: v1 **2462.300 ms**, v6 **2172.840 ms** (~**11.8% szybciej**),
- `test_input_low_mod.txt`: v1 **1711.390 ms**, v6 **1543.050 ms** (~**9.8% szybciej**),
- `test_input_mid_mod.txt`: v1 **1652.100 ms**, v6 **1499.310 ms** (~**9.2% szybciej**),
- `test_input_high_mod.txt`: v1 **1609.700 ms**, v6 **1477.610 ms** (~**8.2% szybciej**).

Wniosek: wielowątkowość poprawia czas względem bazowej, ale koszt podziału/scalenia i finalnej fazy sekwencyjnej ogranicza zysk względem v3.

## 4. Dane testowe

Wygenerowano:

- `test_input_large.txt` (~34 MiB) - duży plik zawierający wszystkie wymagane klasy przypadków,
- `test_input_low_mod.txt` (~19 MiB) - mniejsza proporcja modyfikacji,
- `test_input_mid_mod.txt` (~19 MiB) - średnia proporcja modyfikacji,
- `test_input_high_mod.txt` (~19 MiB) - duża proporcja modyfikacji.

Generator: `generate_test_input.py`.

Pliki zawierają:

- znaki sterujące (`<32`), `DEL` (`127`) i bajty `>127`,
- sekwencje spacji/tab/newline,
- litery mieszane (upper/lower),
- dużo znaków interpunkcyjnych,
- powtarzające się sąsiednie wyrazy.

## 5. Pomiary czasowe

Metodyka:

- uruchomienia: `run_bench.sh`,
- metryka: średni czas z 5 powtórzeń (`avg_ms`),
- wynik: `bench_results.csv`.

| Input | normalize0 | normalize1 | normalize2 | normalize3 | normalize4 | normalize5 | normalize6 |
|---|---:|---:|---:|---:|---:|---:|---:|
| `test_input_large.txt` | 2389.770 ms | 2462.300 ms | 2398.460 ms | **1165.800 ms** | 3358.930 ms | 1571.240 ms | 2172.840 ms |
| `test_input_low_mod.txt` | 1648.640 ms | 1711.390 ms | 1677.370 ms | **818.250 ms** | 2289.030 ms | 1142.800 ms | 1543.050 ms |
| `test_input_mid_mod.txt` | 1610.870 ms | 1652.100 ms | 1618.900 ms | **784.606 ms** | 2183.810 ms | 1096.880 ms | 1499.310 ms |
| `test_input_high_mod.txt` | 1571.240 ms | 1609.700 ms | 1578.200 ms | **740.106 ms** | 2105.610 ms | 1072.020 ms | 1477.610 ms |

Najlepsza wersja: `normalize3` (in-place), przyspieszenie względem `normalize1`:

- `test_input_large.txt`: ~2.11x,
- `test_input_low_mod.txt`: ~2.09x,
- `test_input_mid_mod.txt`: ~2.11x,
- `test_input_high_mod.txt`: ~2.18x.

Wnioski z pomiarów czasowych:

- Zmiana z v1 na v2 daje minimalny zysk rzędu 2-3%, co potwierdza, że sama prealokacja pomaga głównie przez ograniczenie realokacji, ale nie zmienia charakteru obliczeń.
- Największy efekt daje v3 (in-place): skrócenie czasu o ponad połowę względem bazy pokazuje, że kluczowym kosztem był ruch danych i liczba pośrednich kopii, a nie same operacje warunkowe na znakach.
- V4 (algorytmy STL) jest najwolniejsza we wszystkich przypadkach. W praktyce koszt wielu pełnych przejść po danych i dodatkowych etapów transformacji przewyższa zysk z bardziej deklaratywnego zapisu.
- V5 (bare-metal) wyraźnie poprawia wynik względem bazy i v4, ale nadal przegrywa z v3, bo wciąż ponosi koszt dodatkowego bufora i kopiowania do `std::string` po etapie wskaźnikowym.
- V6 (równoległa) poprawia czas względem v1, ale nie dogania v3: dla tego zadania narzut podziału wejścia, synchronizacji i scalania wyników ogranicza opłacalność wielowątkowości.
- Różnice między `low_mod`, `mid_mod` i `high_mod` są spójne jakościowo (ranking wersji normalizacji się nie zmienia), co oznacza, że uzyskane wnioski nie zależą od jednego konkretnego rozkładu danych wejściowych.

## 6. Profilowanie CPU i pamięci

### 6.1 Gprof (hotspoty funkcji)

Profilowanie wykonano wariantami `-pg` (pliki `gprof_normalize*.txt`).

Wnioski z top hotspotów:

- `normalize0`, `normalize1`, `normalize2`, `normalize5`: duży koszt `deduplicate_adjacent_words(...)`,
- `normalize3`: koszt skupiony głównie w `normalize_text_v3(...)` (mniej narzutów alokacji),
- `normalize4`: największy koszt pętli algorytmów STL (`normalize_text_v4(...)`),
- `normalize6`: koszt dzieli się między `normalize_chunk(...)` i finalną fazę scalania/deduplikacji.

### 6.2 Profil CPU + pamięć (`/usr/bin/time`)

Metryki na `test_input_large.txt` (`time_profile.txt`):

| Wersja | elapsed [s] | user [s] | sys [s] | CPU | max RSS [KiB] |
|---|---:|---:|---:|---:|---:|
| normalize0 | 2.49 | 2.30 | 0.08 | 95% | 108640 |
| normalize1 | 2.56 | 2.47 | 0.08 | 99% | 108360 |
| normalize2 | 2.53 | 2.38 | 0.05 | 96% | 83128 |
| normalize3 | **1.19** | 1.14 | 0.04 | 99% | **68752** |
| normalize4 | 3.35 | 3.28 | 0.06 | 99% | 94552 |
| normalize5 | 1.62 | 1.52 | 0.09 | 99% | 126516 |
| normalize6 | 2.18 | 3.57 | 0.10 | 168% | 147328 |

Uwagi:

- `normalize3` ma najlepszy czas i najniższe zużycie pamięci,
- `normalize6` wykorzystuje wiele wątków (CPU > 100%), ale zwiększa RSS i narzut zarządzania wątkami.

## 7. Wnioski końcowe

1. Największy wpływ na czas miała zmiana algorytmiczna in-place (v3), która ograniczyła alokacje i kopiowanie danych.
2. Prealokacja (v2) dała minimalny zysk względem wersji bazowej.
3. Wersja STL (v4) poprawia czytelność kodu, ale dla tego obciążenia była wyraźnie wolniejsza od bazy.
4. Wersja bare-metal (v5) znacząco przyspieszyła wykonanie względem bazy, lecz nadal ustępuje v3.
5. Wersja równoległa (v6) poprawia czas względem bazy, ale koszt podziału i scalania danych ogranicza zysk końcowy.
