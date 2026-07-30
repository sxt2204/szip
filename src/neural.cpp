#include "neural.hpp"
#include "range_coder.hpp"
#include <stdexcept>

namespace sxzip {

std::vector<uint8_t> Neural::compress(const std::vector<uint8_t>& input) {
    if (input.empty()) return {};

    RangeEncoder encoder;
    NeuralPredictor predictor;

    // Bit-level decoders don't inherently know when to stop decoding.
    // We prefix the payload with the uncompressed length (4 bytes).
    uint32_t size = static_cast<uint32_t>(input.size());
    std::vector<uint8_t> out;
    out.push_back(size & 0xFF);
    out.push_back((size >> 8) & 0xFF);
    out.push_back((size >> 16) & 0xFF);
    out.push_back((size >> 24) & 0xFF);

    for (uint8_t byte : input) {
        // Compress bit by bit from MSB to LSB
        for (int i = 7; i >= 0; --i) {
            int bit = (byte >> i) & 1;
            uint32_t p1 = predictor.predict();
            encoder.encode(bit, p1);
            predictor.update(bit);
        }
    }

    encoder.flush();
    auto encoded_bytes = encoder.extract_bytes();
    out.insert(out.end(), encoded_bytes.begin(), encoded_bytes.end());
    
    return out;
}

std::vector<uint8_t> Neural::decompress(const std::vector<uint8_t>& input) {
    if (input.size() < 4) return {};

    uint32_t size = input[0] | (input[1] << 8) | (input[2] << 16) | (input[3] << 24);
    
    std::vector<uint8_t> range_data(input.begin() + 4, input.end());
    RangeDecoder decoder(range_data);
    NeuralPredictor predictor;
    
    std::vector<uint8_t> out;
    out.reserve(size);

    for (uint32_t i = 0; i < size; ++i) {
        uint8_t byte = 0;
        // Decompress bit by bit from MSB to LSB
        for (int j = 7; j >= 0; --j) {
            uint32_t p1 = predictor.predict();
            int bit = decoder.decode(p1);
            predictor.update(bit);
            byte |= (bit << j);
        }
        out.push_back(byte);
    }

    return out;
}

} // namespace sxzip
