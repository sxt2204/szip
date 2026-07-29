#include "rle.hpp"

namespace szip {

std::vector<uint8_t> Rle::compress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    if (input.empty()) return output;

    output.reserve(input.size());
    std::vector<uint8_t> literals;
    literals.reserve(128);

    auto flush_literals = [&]() {
        if (literals.empty()) return;
        size_t count = literals.size();
        output.push_back(static_cast<uint8_t>(count - 1)); // 0x00..0x7F
        output.insert(output.end(), literals.begin(), literals.end());
        literals.clear();
    };

    size_t i = 0;
    const size_t n = input.size();

    while (i < n) {
        // Find run length of identical bytes
        size_t run_len = 1;
        while (i + run_len < n && input[i + run_len] == input[i] && run_len < 0xFFFFFFFF) {
            run_len++;
        }

        if (run_len >= 128) {
            flush_literals();
            // Emit Long Run packet (0xFF)
            output.push_back(0xFF);
            uint32_t len_32 = static_cast<uint32_t>(run_len);
            for (int j = 0; j < 4; ++j) {
                output.push_back(static_cast<uint8_t>((len_32 >> (j * 8)) & 0xFF));
            }
            output.push_back(input[i]);
            i += run_len;
        } else if (run_len >= 3) {
            // Flush any pending literal bytes
            flush_literals();

            // Emit short run packet (0x80 | (run_len - 1))
            output.push_back(static_cast<uint8_t>(0x80 | (run_len - 1)));
            output.push_back(input[i]);
            i += run_len;
        } else {
            literals.push_back(input[i]);
            i++;
            if (literals.size() == 128) {
                flush_literals();
            }
        }
    }

    flush_literals();
    return output;
}

std::vector<uint8_t> Rle::decompress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    if (input.empty()) return output;

    size_t i = 0;
    const size_t n = input.size();

    while (i < n) {
        uint8_t header = input[i++];

        if (header == 0xFF) { // Long Run packet
            if (i + 4 >= n) {
                throw std::runtime_error("Corrupted RLE stream: truncated long run packet");
            }
            uint32_t len_32 = 0;
            for (int j = 0; j < 4; ++j) {
                len_32 |= (static_cast<uint32_t>(input[i++]) << (j * 8));
            }
            if (i >= n) throw std::runtime_error("Corrupted RLE stream: missing long run value");
            uint8_t val = input[i++];
            output.insert(output.end(), len_32, val);
        } else if (header & 0x80) { // Short Run packet
            size_t len = (header & 0x7F) + 1;
            if (i >= n) {
                throw std::runtime_error("Corrupted RLE stream: truncated run packet");
            }
            uint8_t val = input[i++];
            output.insert(output.end(), len, val);
        } else { // Literal packet
            size_t len = (header & 0x7F) + 1;
            if (i + len > n) {
                throw std::runtime_error("Corrupted RLE stream: truncated literal packet");
            }
            output.insert(output.end(), input.begin() + i, input.begin() + i + len);
            i += len;
        }
    }

    return output;
}

} // namespace szip
