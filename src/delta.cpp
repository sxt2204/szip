#include "delta.hpp"

namespace szip {

std::vector<uint8_t> Delta::compress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};
    std::vector<uint8_t> output(input.size());

    output[0] = input[0];
    for (size_t i = 1; i < input.size(); ++i) {
        output[i] = static_cast<uint8_t>(input[i] - input[i - 1]);
    }
    return output;
}

std::vector<uint8_t> Delta::decompress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};
    std::vector<uint8_t> output(input.size());

    output[0] = input[0];
    for (size_t i = 1; i < input.size(); ++i) {
        output[i] = static_cast<uint8_t>(output[i - 1] + input[i]);
    }
    return output;
}

std::vector<uint8_t> Delta4::compress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};
    std::vector<uint8_t> output(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (i < 4) output[i] = input[i];
        else output[i] = static_cast<uint8_t>(input[i] - input[i - 4]);
    }
    return output;
}

std::vector<uint8_t> Delta4::decompress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};
    std::vector<uint8_t> output(input.size());

    for (size_t i = 0; i < input.size(); ++i) {
        if (i < 4) output[i] = input[i];
        else output[i] = static_cast<uint8_t>(output[i - 4] + input[i]);
    }
    return output;
}

} // namespace szip
