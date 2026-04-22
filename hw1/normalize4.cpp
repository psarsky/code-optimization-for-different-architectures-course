#include "normalize_common.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>

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

int main(int argc, char** argv) {
    return normalize::run_cli("normalize4", normalize_text_v4, argc, argv, false);
}
