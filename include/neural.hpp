#ifndef SXZP_NEURAL_HPP
#define SXZP_NEURAL_HPP

#include "algorithm_base.hpp"
#include <vector>
#include <cstdint>

namespace sxzip {

class Neural {
public:
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);
};

} // namespace sxzip

#endif // SXZP_NEURAL_HPP
