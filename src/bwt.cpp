#include "bwt.hpp"
#include <algorithm>
#include <numeric>
#include <array>
#include <stdexcept>

namespace sxzip {

namespace {

struct BwtComparator {
    const uint8_t* data;
    size_t size;

    bool operator()(size_t i, size_t j) const {
        if (i == j) return false;
        size_t max_depth = std::min(size, static_cast<size_t>(1024));
        for (size_t k = 0; k < max_depth; ++k) {
            uint8_t c1 = data[(i + k) % size];
            uint8_t c2 = data[(j + k) % size];
            if (c1 != c2) return c1 < c2;
        }
        return i < j;
    }
};

void compress_block(const uint8_t* data, size_t size, std::vector<uint8_t>& output) {
    if (size == 0) return;

    std::vector<size_t> indices(size);
    std::iota(indices.begin(), indices.end(), 0);

    std::stable_sort(indices.begin(), indices.end(), BwtComparator{data, size});

    uint32_t primary_index = 0;
    for (size_t k = 0; k < size; ++k) {
        if (indices[k] == 0) {
            primary_index = static_cast<uint32_t>(k);
            break;
        }
    }

    // Write primary index (4 bytes)
    for (int i = 0; i < 4; ++i) {
        output.push_back(static_cast<uint8_t>((primary_index >> (i * 8)) & 0xFF));
    }

    // Write block size (4 bytes)
    uint32_t sz = static_cast<uint32_t>(size);
    for (int i = 0; i < 4; ++i) {
        output.push_back(static_cast<uint8_t>((sz >> (i * 8)) & 0xFF));
    }

    // Write transformed array L
    for (size_t k = 0; k < size; ++k) {
        size_t idx = indices[k];
        uint8_t last_char = data[(idx + size - 1) % size];
        output.push_back(last_char);
    }
}

void decompress_block(const uint8_t* data, size_t& read_idx, size_t total_size, std::vector<uint8_t>& output) {
    if (read_idx + 8 > total_size) {
        throw std::runtime_error("Corrupted BWT stream: header truncated");
    }

    uint32_t primary_index = 0;
    for (int i = 0; i < 4; ++i) {
        primary_index |= (static_cast<uint32_t>(data[read_idx++]) << (i * 8));
    }

    uint32_t block_size = 0;
    for (int i = 0; i < 4; ++i) {
        block_size |= (static_cast<uint32_t>(data[read_idx++]) << (i * 8));
    }

    if (read_idx + block_size > total_size) {
        throw std::runtime_error("Corrupted BWT stream: block data truncated");
    }

    if (block_size == 0) return;
    if (primary_index >= block_size) {
        throw std::runtime_error("Corrupted BWT stream: invalid primary index");
    }

    const uint8_t* L = data + read_idx;
    read_idx += block_size;

    std::array<uint32_t, 256> count{};
    for (size_t i = 0; i < block_size; ++i) count[L[i]]++;

    std::array<uint32_t, 256> C{};
    uint32_t sum = 0;
    for (int i = 0; i < 256; ++i) {
        C[i] = sum;
        sum += count[i];
    }

    std::vector<uint32_t> P(block_size);
    for (size_t i = 0; i < block_size; ++i) {
        P[i] = C[L[i]]++;
    }

    std::vector<uint8_t> block_orig(block_size);
    uint32_t curr = primary_index;
    for (size_t k = block_size; k > 0; --k) {
        block_orig[k - 1] = L[curr];
        curr = P[curr];
    }

    output.insert(output.end(), block_orig.begin(), block_orig.end());
}

} // anonymous namespace

std::vector<uint8_t> Bwt::compress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    if (input.empty()) return output;

    size_t offset = 0;
    const size_t n = input.size();

    while (offset < n) {
        size_t block_sz = std::min(DEFAULT_BLOCK_SIZE, n - offset);
        compress_block(input.data() + offset, block_sz, output);
        offset += block_sz;
    }

    return output;
}

std::vector<uint8_t> Bwt::decompress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    if (input.empty()) return output;

    size_t read_idx = 0;
    const size_t total_sz = input.size();

    while (read_idx < total_sz) {
        decompress_block(input.data(), read_idx, total_sz, output);
    }

    return output;
}

} // namespace sxzip
