#ifndef SZIP_HUFFMAN_HPP
#define SZIP_HUFFMAN_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>

namespace szip {

class Huffman {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace szip

#endif // SZIP_HUFFMAN_HPP
