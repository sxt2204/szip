#ifndef SZIP_NEURAL_HPP
#define SZIP_NEURAL_HPP

#include "algorithm_base.hpp"
#include <vector>
#include <cstdint>

namespace szip {

class Neural {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace szip

#endif // SZIP_NEURAL_HPP
