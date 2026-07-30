#ifndef SXZP_BWT_HPP
#define SXZP_BWT_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

namespace sxzip {

class Bwt {
public:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 100000; // 100KB blocks

    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace sxzip

#endif // SXZP_BWT_HPP
