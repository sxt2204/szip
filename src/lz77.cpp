#include "lz77.hpp"
#include <algorithm>
#include <stdexcept>

namespace sxzip {

static inline void encode_varint(std::vector<uint8_t>& out, size_t val) {
    do {
        uint8_t byte = val & 0x7F;
        val >>= 7;
        if (val > 0) byte |= 0x80;
        out.push_back(byte);
    } while (val > 0);
}

static inline size_t decode_varint(const std::vector<uint8_t>& in, size_t& idx) {
    size_t val = 0;
    size_t shift = 0;
    while (idx < in.size()) {
        uint8_t byte = in[idx++];
        val |= static_cast<size_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) break;
        shift += 7;
    }
    return val;
}

std::vector<uint8_t> Lz77::compress(const std::vector<uint8_t>& input) {
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

    // 24-bit hash table (16M entries) for minimal collisions across massive blocks
    std::vector<int32_t> head(16777216, -1);
    std::vector<int32_t> prev(input.size(), -1);

    auto hash3 = [](const uint8_t* p) -> uint32_t {
        uint32_t h = (static_cast<uint32_t>(p[0]) << 16) | (static_cast<uint32_t>(p[1]) << 8) | p[2];
        return (h * 0x1E35A7BD) & 0xFFFFFF; // 24-bit mask
    };

    size_t i = 0;
    const size_t n = input.size();

    while (i < n) {
        size_t best_len = 0;
        size_t best_dist = 0;
        size_t max_possible_len = n - i; // Unbounded match length!

        uint32_t h = 0;
        if (max_possible_len >= MIN_MATCH_LEN) {
            h = hash3(&input[i]);
            int32_t match_idx = head[h];
            int limit = 64; // Max hash chain hops

            while (match_idx != -1 && limit-- > 0) {
                size_t match_len = 0;
                while (match_len < max_possible_len && input[match_idx + match_len] == input[i + match_len]) {
                    match_len++;
                }

                if (match_len > best_len) {
                    best_len = match_len;
                    best_dist = i - match_idx;
                    if (best_len == max_possible_len) break;
                }
                match_idx = prev[match_idx];
            }
        }

        // Insert current position into hash table
        if (max_possible_len >= MIN_MATCH_LEN) {
            prev[i] = head[h];
            head[h] = i;
        }

        if (best_len >= MIN_MATCH_LEN) {
            flush_literals();

            // Emit match packet with VarInt
            if (best_len < 130) {
                output.push_back(static_cast<uint8_t>(0x80 | (best_len - 3)));
            } else {
                output.push_back(0xFF);
                encode_varint(output, best_len - 130);
            }
            encode_varint(output, best_dist);

            // Insert intermediate positions to hash chain
            for (size_t k = 1; k < best_len; ++k) {
                if (i + k + MIN_MATCH_LEN <= n) {
                    uint32_t hk = hash3(&input[i + k]);
                    prev[i + k] = head[hk];
                    head[hk] = i + k;
                }
            }
            i += best_len;
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

std::vector<uint8_t> Lz77::decompress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    if (input.empty()) return output;

    size_t i = 0;
    const size_t n = input.size();

    while (i < n) {
        uint8_t header = input[i++];
        if ((header & 0x80) == 0) { // Literal packet
            size_t len = header + 1;
            if (i + len > n) {
                throw std::runtime_error("Corrupted LZ77 stream: truncated literal packet");
            }
            output.insert(output.end(), input.begin() + i, input.begin() + i + len);
            i += len;
        } else { // Match packet
            size_t len = 0;
            if (header < 0xFF) {
                len = (header & 0x7F) + 3;
            } else {
                len = decode_varint(input, i) + 130;
            }
            size_t dist = decode_varint(input, i);

            if (dist == 0 || dist > output.size()) {
                throw std::runtime_error("Corrupted LZ77 stream: invalid match distance");
            }

            if (dist >= len) {
                // Non-overlapping block fast insert
                size_t src_start = output.size() - dist;
                output.insert(output.end(), output.begin() + src_start, output.begin() + src_start + len);
            } else {
                // Overlapping repetition copy
                for (size_t k = 0; k < len; ++k) {
                    output.push_back(output[output.size() - dist]);
                }
            }
        }
    }

    return output;
}

} // namespace sxzip
