#ifndef SZIP_MTF_HPP
#define SZIP_MTF_HPP

#include <vector>
#include <cstdint>

namespace szip {

class Mtf {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace szip

#endif // SZIP_MTF_HPP
