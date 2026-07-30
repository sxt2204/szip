#ifndef SXZP_LZ77_HPP
#define SXZP_LZ77_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

namespace sxzip {

class Lz77 {
public:
    static constexpr size_t MIN_MATCH_LEN   = 3;

    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace sxzip

#endif // SXZP_LZ77_HPP
