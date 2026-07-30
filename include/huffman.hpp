#ifndef SXZP_HUFFMAN_HPP
#define SXZP_HUFFMAN_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>

namespace sxzip {

class Huffman {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace sxzip

#endif // SXZP_HUFFMAN_HPP
