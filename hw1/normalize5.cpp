#include "normalize_common.hpp"

#include <cctype>
#include <cstring>
#include <memory>
#include <string>

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

int main(int argc, char** argv) {
    return normalize::run_cli("normalize5", normalize_text_v5, argc, argv, false);
}
