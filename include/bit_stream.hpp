#ifndef SXZP_BIT_STREAM_HPP
#define SXZP_BIT_STREAM_HPP

#include <vector>
#include <cstdint>
#include <cstddef>

namespace sxzip {

class BitWriter {
public:
    BitWriter() = default;

    void write_bit(uint8_t bit);
    void write_bits(uint64_t val, size_t num_bits);
    void flush();

    const std::vector<uint8_t>& get_bytes() const { return buffer_; }
    std::vector<uint8_t> extract_bytes();
    size_t total_bits() const { return total_bits_; }

private:
    std::vector<uint8_t> buffer_;
    uint8_t current_byte_ = 0;
    uint8_t bit_pos_ = 0;
    size_t total_bits_ = 0;
    bool flushed_ = false;
};

class BitReader {
public:
    explicit BitReader(const std::vector<uint8_t>& data);
    BitReader(const uint8_t* data, size_t size);

    bool read_bit(uint8_t& bit);
    bool read_bits(size_t num_bits, uint64_t& val);
    bool has_more() const;

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0;
    size_t byte_idx_ = 0;
    uint8_t bit_pos_ = 0;
};

} // namespace sxzip

#endif // SXZP_BIT_STREAM_HPP
