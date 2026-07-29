#ifndef SZIP_LZ77_HPP
#define SZIP_LZ77_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

namespace szip {

class Lz77 {
public:
    static constexpr size_t MIN_MATCH_LEN   = 3;

    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace szip

#endif // SZIP_LZ77_HPP
