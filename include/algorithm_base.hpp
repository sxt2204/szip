#ifndef SXZP_ALGORITHM_BASE_HPP
#define SXZP_ALGORITHM_BASE_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace sxzip {

enum class AlgorithmType : uint8_t {
    NONE    = 0x00,
    DELTA   = 0x01,
    LZ77    = 0x02,
    BWT     = 0x03,
    MTF     = 0x04,
    RLE     = 0x05,
    HUFFMAN = 0x06,
    NEURAL  = 0x07,
    STORE   = 0x08,
    DELTA4  = 0x09
};

inline std::string algorithm_to_string(AlgorithmType type) {
    switch (type) {
        case AlgorithmType::DELTA:   return "delta";
        case AlgorithmType::LZ77:    return "lz77";
        case AlgorithmType::BWT:     return "bwt";
        case AlgorithmType::MTF:     return "mtf";
        case AlgorithmType::RLE:     return "rle";
        case AlgorithmType::HUFFMAN: return "huffman";
        case AlgorithmType::NEURAL:  return "neural";
        case AlgorithmType::STORE:   return "store";
        case AlgorithmType::DELTA4:  return "delta4";
        default:                     return "none";
    }
}

inline AlgorithmType string_to_algorithm(const std::string& str) {
    if (str == "delta")   return AlgorithmType::DELTA;
    if (str == "lz77")    return AlgorithmType::LZ77;
    if (str == "bwt")     return AlgorithmType::BWT;
    if (str == "mtf")     return AlgorithmType::MTF;
    if (str == "rle")     return AlgorithmType::RLE;
    if (str == "huffman") return AlgorithmType::HUFFMAN;
    if (str == "neural")  return AlgorithmType::NEURAL;
    if (str == "store")   return AlgorithmType::STORE;
    if (str == "delta4")  return AlgorithmType::DELTA4;
    return AlgorithmType::NONE;
}

} // namespace sxzip

#endif // SXZP_ALGORITHM_BASE_HPP
