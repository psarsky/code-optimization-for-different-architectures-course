#include "normalize_common.hpp"

#include <cctype>
#include <string>

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

int main(int argc, char** argv) {
    return normalize::run_cli("normalize2", normalize_text_v2, argc, argv, true);
}
