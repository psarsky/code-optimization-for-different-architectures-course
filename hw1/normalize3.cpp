#include "normalize_common.hpp"

#include <cctype>
#include <cstring>
#include <string>
#include <string_view>

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

int main(int argc, char** argv) {
    return normalize::run_cli("normalize3", normalize_text_v3, argc, argv, true);
}
