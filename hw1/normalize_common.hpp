#pragma once

#include <chrono>
#include <cctype>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace normalize {

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

std::string read_file_binary(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open input file: " + path);
    }

    input.seekg(0, std::ios::end);
    const std::streamsize size = input.tellg();
    input.seekg(0, std::ios::beg);

    if (size < 0) {
        throw std::runtime_error("cannot determine input file size: " + path);
    }

    std::string data(static_cast<std::size_t>(size), '\0');
    if (!data.empty() && !input.read(data.data(), size)) {
        throw std::runtime_error("failed to read input file: " + path);
    }

    return data;
}

void write_file_binary(const std::string& path, const std::string& data) {
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        throw std::runtime_error("cannot open output file: " + path);
    }
    output.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!output.good()) {
        throw std::runtime_error("failed to write output file: " + path);
    }
}

template <typename Normalizer>
int run_cli(
    const std::string& version_name,
    Normalizer normalizer,
    int argc,
    char** argv,
    bool preallocate_output_buffer) {
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> [repeats]\n";
        return 1;
    }

    const std::string input_path = argv[1];
    const std::string output_path = argv[2];

    int repeats = 1;
    if (argc == 4) {
        repeats = std::stoi(argv[3]);
        if (repeats <= 0) {
            std::cerr << "repeats must be positive\n";
            return 1;
        }
    }

    try {
        const std::string input = read_file_binary(input_path);

        std::string output;
        if (preallocate_output_buffer) {
            output.reserve(input.size());
        }

        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < repeats; ++i) {
            output = normalizer(input);
        }
        const auto t1 = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::milli> elapsed = t1 - t0;

        write_file_binary(output_path, output);

        const double avg_ms = elapsed.count() / static_cast<double>(repeats);
        std::cout << "version=" << version_name
                  << " bytes_in=" << input.size()
                  << " bytes_out=" << output.size()
                  << " repeats=" << repeats
                  << " total_ms=" << elapsed.count()
                  << " avg_ms=" << avg_ms << '\n';
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}

}  // namespace normalize
