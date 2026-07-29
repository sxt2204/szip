#ifndef SZIP_DELTA_HPP
#define SZIP_DELTA_HPP

#include <vector>
#include <cstdint>

namespace szip {

class Delta {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

class Delta4 {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace szip

#endif // SZIP_DELTA_HPP
