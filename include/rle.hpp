#ifndef SXZP_RLE_HPP
#define SXZP_RLE_HPP

#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

namespace sxzip {

class Rle {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace sxzip

#endif // SXZP_RLE_HPP
