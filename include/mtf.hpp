#ifndef SXZP_MTF_HPP
#define SXZP_MTF_HPP

#include <vector>
#include <cstdint>

namespace sxzip {

class Mtf {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace sxzip

#endif // SXZP_MTF_HPP
