#ifndef SXZP_RANGE_CODER_HPP
#define SXZP_RANGE_CODER_HPP

#include <cstdint>
#include <vector>

namespace sxzip {

class RangeEncoder {
public:
    RangeEncoder() = default;
    
    inline void encode(int bit, uint32_t p1) {
        uint32_t r = (range_ >> 12) * p1;
        if (bit == 1) {
            range_ = r;
        } else {
            low_ += r;
            range_ -= r;
        }
        
        while (range_ < 0x01000000) {
            range_ <<= 8;
            shift_low();
        }
    }
    
    // Flush the remaining range state to output
    void flush();
    
    std::vector<uint8_t> extract_bytes() { return std::move(out_); }

private:
    uint64_t low_ = 0;
    uint32_t range_ = 0xFFFFFFFF;
    uint32_t cache_ = 0;
    uint32_t ff_num_ = 1;
    std::vector<uint8_t> out_;

    void shift_low();
};

class RangeDecoder {
public:
    explicit RangeDecoder(const std::vector<uint8_t>& in);
    
    inline int decode(uint32_t p1) {
        uint32_t r = (range_ >> 12) * p1;
        int bit = (code_ < r) ? 1 : 0;
        
        if (bit == 1) {
            range_ = r;
        } else {
            code_ -= r;
            range_ -= r;
        }
        
        while (range_ < 0x01000000) {
            range_ <<= 8;
            code_ = (code_ << 8) | read_byte();
        }
        return bit;
    }

private:
    const std::vector<uint8_t>& in_;
    size_t ptr_ = 0;
    uint32_t range_ = 0xFFFFFFFF;
    uint32_t code_ = 0;

    inline uint8_t read_byte() {
        if (ptr_ < in_.size()) {
            return in_[ptr_++];
        }
        return 0; // Padding 0s if we reach end
    }
};

class NeuralPredictor {
public:
    NeuralPredictor();
    
    inline uint32_t predict() const {
        uint32_t p0 = c0_[cxt0_];
        uint32_t p1 = c1_[cxt1_];
        
        // 22-bit Hash for c2
        uint32_t h2 = (cxt2_ * 0x1E35A7BD) >> 10;
        uint32_t p2 = c2_[h2 & 0x3FFFFF]; // 4 million slots (22-bit)
        
        // Context Mixing: (p0 + 3*p1 + 4*p2) / 8
        uint32_t p = (p0 + p1 * 3 + p2 * 4) >> 3;
        
        if (p < 1) p = 1;
        if (p > 4095) p = 4095;
        
        return p;
    }
    
    inline void update(int bit) {
        int32_t target = bit << 12; // Branchless: 1 -> 4096, 0 -> 0
        
        c0_[cxt0_] += (target - c0_[cxt0_]) >> 5;
        c1_[cxt1_] += (target - c1_[cxt1_]) >> 5;
        
        uint32_t h2 = (cxt2_ * 0x1E35A7BD) >> 10;
        c2_[h2 & 0x3FFFFF] += (target - c2_[h2 & 0x3FFFFF]) >> 6; // Slower learning rate for higher order

        cxt0_ = (cxt0_ << 1) | bit;
        cxt1_ = (cxt1_ & 0xFF00) | cxt0_;
        cxt2_ = (cxt2_ & 0xFFFF00) | cxt0_;

        if (cxt0_ >= 256) {
            uint8_t finished_byte = static_cast<uint8_t>(cxt0_ & 0xFF);
            uint8_t prev_byte = static_cast<uint8_t>(cxt1_ >> 8);
            
            cxt0_ = 1;
            cxt1_ = (static_cast<uint32_t>(finished_byte) << 8) | 1;
            cxt2_ = (static_cast<uint32_t>(prev_byte) << 16) | (static_cast<uint32_t>(finished_byte) << 8) | 1;
        }
    }

private:
    uint16_t c0_[256];      // Order-0 bit context
    uint16_t c1_[65536];    // Order-1 byte context
    std::vector<uint16_t> c2_; // Order-2 byte context (22-bit hashed)
    uint32_t cxt0_ = 1;     // Partial byte context
    uint32_t cxt1_ = 1;     // (Last byte << 8) | Partial byte context
    uint32_t cxt2_ = 1;     // (Last 2 bytes << 8) | Partial byte context
};

} // namespace sxzip

#endif // SXZP_RANGE_CODER_HPP
