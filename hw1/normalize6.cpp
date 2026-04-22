#include "normalize_common.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <thread>
#include <vector>

namespace {

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

}  // namespace

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

int main(int argc, char** argv) {
    return normalize::run_cli("normalize6", normalize_text_v6, argc, argv, false);
}
