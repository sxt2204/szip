#ifndef SZIP_BWT_HPP
#define SZIP_BWT_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

namespace szip {

class Bwt {
public:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 100000; // 100KB blocks

    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace szip

#endif // SZIP_BWT_HPP
