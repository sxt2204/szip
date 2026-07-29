#ifndef SZIP_RLE_HPP
#define SZIP_RLE_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace szip {

class Rle {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace szip

#endif // SZIP_RLE_HPP
