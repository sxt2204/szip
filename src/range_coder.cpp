#include "range_coder.hpp"

namespace sxzip {

// --- RangeEncoder ---



void RangeEncoder::flush() {
    for (int i = 0; i < 5; ++i) {
        shift_low();
    }
}

void RangeEncoder::shift_low() {
    if (static_cast<uint32_t>(low_) < 0xFF000000 || static_cast<uint32_t>(low_ >> 32) != 0) {
        uint8_t temp = static_cast<uint8_t>(cache_);
        do {
            out_.push_back(temp + static_cast<uint8_t>(low_ >> 32));
            temp = 0xFF;
        } while (--ff_num_ != 0);
        cache_ = static_cast<uint32_t>(low_) >> 24;
    }
    ff_num_++;
    low_ = static_cast<uint32_t>(low_) << 8;
}

// --- RangeDecoder ---

RangeDecoder::RangeDecoder(const std::vector<uint8_t>& in) : in_(in) {
    code_ = 0;
    for (int i = 0; i < 5; ++i) {
        code_ = (code_ << 8) | read_byte();
    }
}



// --- NeuralPredictor ---

NeuralPredictor::NeuralPredictor() : c2_(4194304, 2048) {
    // Initialize all probabilities to 2048 (50% for scale of 4096)
    for (int i = 0; i < 256; ++i) c0_[i] = 2048;
    for (int i = 0; i < 65536; ++i) c1_[i] = 2048;
}



} // namespace sxzip
