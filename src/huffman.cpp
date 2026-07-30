#include "huffman.hpp"
#include "bit_stream.hpp"
#include <queue>
#include <stdexcept>
#include <cstring>

namespace sxzip {

namespace {

struct HuffmanNode {
    uint8_t symbol = 0;
    uint64_t freq = 0;
    uint32_t seq = 0;
    bool is_leaf = false;
    HuffmanNode* left = nullptr;
    HuffmanNode* right = nullptr;

    ~HuffmanNode() {
        delete left;
        delete right;
    }
};

struct NodeComparator {
    bool operator()(const HuffmanNode* a, const HuffmanNode* b) const {
        if (a->freq != b->freq) {
            return a->freq > b->freq;
        }
        return a->seq > b->seq;
    }
};

struct Code {
    uint64_t val = 0;
    size_t length = 0;
};

void build_codes(const HuffmanNode* node, uint64_t current_code, size_t depth, std::array<Code, 256>& codes) {
    if (!node) return;
    if (node->is_leaf) {
        codes[node->symbol] = {current_code, depth};
        return;
    }
    build_codes(node->left, (current_code << 1) | 0, depth + 1, codes);
    build_codes(node->right, (current_code << 1) | 1, depth + 1, codes);
}

HuffmanNode* build_tree_from_frequencies(const std::array<uint64_t, 256>& freqs) {
    uint32_t seq_counter = 0;
    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, NodeComparator> pq;

    for (int s = 0; s < 256; ++s) {
        if (freqs[s] > 0) {
            auto* node = new HuffmanNode();
            node->symbol = static_cast<uint8_t>(s);
            node->freq = freqs[s];
            node->seq = seq_counter++;
            node->is_leaf = true;
            pq.push(node);
        }
    }

    if (pq.empty()) return nullptr;

    if (pq.size() == 1) {
        auto* dummy = new HuffmanNode();
        uint8_t first_sym = pq.top()->symbol;
        dummy->symbol = (first_sym == 0) ? 1 : 0;
        dummy->freq = 0;
        dummy->seq = seq_counter++;
        dummy->is_leaf = true;
        pq.push(dummy);
    }

    while (pq.size() > 1) {
        auto* left = pq.top(); pq.pop();
        auto* right = pq.top(); pq.pop();

        auto* parent = new HuffmanNode();
        parent->freq = left->freq + right->freq;
        parent->seq = seq_counter++;
        parent->is_leaf = false;
        parent->left = left;
        parent->right = right;

        pq.push(parent);
    }

    return pq.top();
}

} // anonymous namespace

std::vector<uint8_t> Huffman::compress(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    uint64_t orig_size = input.size();

    // 1. Write original size (8 bytes)
    for (int i = 0; i < 8; ++i) {
        output.push_back(static_cast<uint8_t>((orig_size >> (i * 8)) & 0xFF));
    }

    if (input.empty()) {
        output.push_back(0); // 0 non-zero symbols
        output.push_back(0);
        return output;
    }

    // 2. Count frequencies
    std::array<uint64_t, 256> freqs{};
    for (uint8_t b : input) {
        freqs[b]++;
    }

    // 3. Count non-zero symbols
    uint16_t num_symbols = 0;
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) num_symbols++;
    }

    // Write num_symbols (2 bytes, little endian)
    output.push_back(static_cast<uint8_t>(num_symbols & 0xFF));
    output.push_back(static_cast<uint8_t>((num_symbols >> 8) & 0xFF));

    // Write symbol frequency table (symbol: 1 byte, freq: 4 bytes)
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            output.push_back(static_cast<uint8_t>(i));
            uint32_t f = static_cast<uint32_t>(freqs[i]);
            for (int k = 0; k < 4; ++k) {
                output.push_back(static_cast<uint8_t>((f >> (k * 8)) & 0xFF));
            }
        }
    }

    // 4. Build Tree & Codes
    std::unique_ptr<HuffmanNode> root(build_tree_from_frequencies(freqs));
    std::array<Code, 256> codes{};
    build_codes(root.get(), 0, 0, codes);

    // 5. Encode payload bits
    BitWriter writer;
    for (uint8_t b : input) {
        const Code& c = codes[b];
        writer.write_bits(c.val, c.length);
    }

    std::vector<uint8_t> bit_payload = writer.extract_bytes();
    output.insert(output.end(), bit_payload.begin(), bit_payload.end());

    return output;
}

std::vector<uint8_t> Huffman::decompress(const std::vector<uint8_t>& input) {
    if (input.size() < 10) {
        throw std::runtime_error("Corrupted Huffman stream: header too short");
    }

    size_t idx = 0;

    // 1. Read original size (8 bytes)
    uint64_t orig_size = 0;
    for (int i = 0; i < 8; ++i) {
        orig_size |= (static_cast<uint64_t>(input[idx++]) << (i * 8));
    }

    if (orig_size == 0) {
        return std::vector<uint8_t>();
    }

    // 2. Read num_symbols (2 bytes)
    uint16_t num_symbols = input[idx++];
    num_symbols |= (static_cast<uint16_t>(input[idx++]) << 8);

    if (num_symbols == 0) {
        return std::vector<uint8_t>();
    }

    // 3. Read symbol frequency table
    std::array<uint64_t, 256> freqs{};
    for (uint16_t i = 0; i < num_symbols; ++i) {
        if (idx + 5 > input.size()) {
            throw std::runtime_error("Corrupted Huffman stream: table truncated");
        }
        uint8_t sym = input[idx++];
        uint32_t f = 0;
        for (int k = 0; k < 4; ++k) {
            f |= (static_cast<uint32_t>(input[idx++]) << (k * 8));
        }
        freqs[sym] = f;
    }

    // 4. Reconstruct Huffman Tree
    std::unique_ptr<HuffmanNode> root(build_tree_from_frequencies(freqs));
    if (!root) {
        throw std::runtime_error("Failed to reconstruct Huffman tree");
    }

    // 5. Decode Bitstream
    BitReader reader(input.data() + idx, input.size() - idx);
    std::vector<uint8_t> output;
    output.reserve(orig_size);

    const HuffmanNode* curr = root.get();
    while (output.size() < orig_size) {
        uint8_t bit = 0;
        if (!reader.read_bit(bit)) {
            throw std::runtime_error("Unexpected EOF while decoding Huffman stream");
        }

        curr = (bit == 0) ? curr->left : curr->right;
        if (!curr) {
            throw std::runtime_error("Invalid bit path in Huffman tree");
        }

        if (curr->is_leaf) {
            output.push_back(curr->symbol);
            curr = root.get();
        }
    }

    return output;
}

} // namespace sxzip
