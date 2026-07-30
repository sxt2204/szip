#include "bit_stream.hpp"

namespace sxzip {

void BitWriter::write_bit(uint8_t bit) {
    flushed_ = false;
    current_byte_ = (current_byte_ << 1) | (bit & 1);
    bit_pos_++;
    total_bits_++;
    if (bit_pos_ == 8) {
        buffer_.push_back(current_byte_);
        current_byte_ = 0;
        bit_pos_ = 0;
    }
}

void BitWriter::write_bits(uint64_t val, size_t num_bits) {
    if (num_bits == 0) return;
    for (size_t i = 0; i < num_bits; ++i) {
        uint8_t bit = (val >> (num_bits - 1 - i)) & 1;
        write_bit(bit);
    }
}

void BitWriter::flush() {
    if (flushed_) return;
    if (bit_pos_ > 0) {
        buffer_.push_back(current_byte_ << (8 - bit_pos_));
        current_byte_ = 0;
        bit_pos_ = 0;
    }
    flushed_ = true;
}

std::vector<uint8_t> BitWriter::extract_bytes() {
    flush();
    return std::move(buffer_);
}

BitReader::BitReader(const std::vector<uint8_t>& data)
    : data_(data.data()), size_(data.size()) {}

BitReader::BitReader(const uint8_t* data, size_t size)
    : data_(data), size_(size) {}

bool BitReader::read_bit(uint8_t& bit) {
    if (byte_idx_ >= size_) return false;
    bit = (data_[byte_idx_] >> (7 - bit_pos_)) & 1;
    bit_pos_++;
    if (bit_pos_ == 8) {
        bit_pos_ = 0;
        byte_idx_++;
    }
    return true;
}

bool BitReader::read_bits(size_t num_bits, uint64_t& val) {
    val = 0;
    for (size_t i = 0; i < num_bits; ++i) {
        uint8_t bit = 0;
        if (!read_bit(bit)) return false;
        val = (val << 1) | bit;
    }
    return true;
}

bool BitReader::has_more() const {
    return byte_idx_ < size_;
}

} // namespace sxzip
