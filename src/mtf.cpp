#include "mtf.hpp"
#include <array>
#include <algorithm>
#include <cstring>

namespace szip {

std::vector<uint8_t> Mtf::compress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};
    std::vector<uint8_t> output(input.size());

    std::array<uint8_t, 256> table{};
    for (int i = 0; i < 256; ++i) table[i] = static_cast<uint8_t>(i);

    for (size_t i = 0; i < input.size(); ++i) {
        uint8_t val = input[i];
        // Find index of val
        uint8_t rank = 0;
        while (table[rank] != val) {
            rank++;
        }
        output[i] = rank;

        // Move to front
        if (rank > 0) {
            std::memmove(&table[1], &table[0], rank);
            table[0] = val;
        }
    }

    return output;
}

std::vector<uint8_t> Mtf::decompress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};
    std::vector<uint8_t> output(input.size());

    std::array<uint8_t, 256> table{};
    for (int i = 0; i < 256; ++i) table[i] = static_cast<uint8_t>(i);

    for (size_t i = 0; i < input.size(); ++i) {
        uint8_t rank = input[i];
        uint8_t val = table[rank];
        output[i] = val;

        // Move to front
        if (rank > 0) {
            std::memmove(&table[1], &table[0], rank);
            table[0] = val;
        }
    }

    return output;
}

} // namespace szip
