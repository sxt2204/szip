#ifndef SXZP_HEADER_HPP
#define SXZP_HEADER_HPP

#include "algorithm_base.hpp"
#include "delta.hpp"
#include "lz77.hpp"
#include "bwt.hpp"
#include "mtf.hpp"
#include "rle.hpp"
#include "huffman.hpp"
#include "neural.hpp"
#include <vector>
#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <future>
#include <thread>

namespace sxzip {

struct BlockInfo {
    size_t block_index = 0;
    std::vector<AlgorithmType> pipeline;
    uint32_t uncompressed_size = 0;
    uint32_t compressed_size = 0;
};

struct SzipFileInfo {
    uint8_t version = 0;
    std::vector<BlockInfo> blocks;
    size_t total_uncompressed_size = 0;
    size_t total_compressed_size = 0;
};

class SxzipEngine {
public:
    static constexpr uint8_t MAGIC[4] = {'S', 'Z', 'I', 'P'};
    static constexpr uint8_t VERSION = 0x05;
    static constexpr size_t DEFAULT_BLOCK_SIZE = 16777216; // 16MB default maximum block size

    static std::vector<AlgorithmType> parse_pipeline_str(const std::string& str) {
        std::vector<AlgorithmType> result;
        std::string token;
        for (char ch : str) {
            if (ch == ',' || ch == '+') {
                if (!token.empty()) {
                    AlgorithmType algo = string_to_algorithm(token);
                    if (algo == AlgorithmType::NONE) {
                        throw std::runtime_error("Unknown algorithm in pipeline: " + token);
                    }
                    result.push_back(algo);
                    token.clear();
                }
            } else if (!std::isspace(ch)) {
                token += static_cast<char>(std::tolower(ch));
            }
        }
        if (!token.empty()) {
            AlgorithmType algo = string_to_algorithm(token);
            if (algo == AlgorithmType::NONE) {
                throw std::runtime_error("Unknown algorithm in pipeline: " + token);
            }
            result.push_back(algo);
        }

        if (result.empty()) {
            result = {AlgorithmType::LZ77, AlgorithmType::NEURAL};
        }
        return result;
    }

    struct ProbeResult {
        std::vector<AlgorithmType> pipeline;
        size_t max_freq; // Acts as an inverse entropy score
    };

    static ProbeResult auto_tune_block(const std::vector<uint8_t>& block) {
        if (block.empty()) return {{AlgorithmType::LZ77, AlgorithmType::MTF, AlgorithmType::NEURAL}, 0};

        size_t scan_size = std::min(block.size(), static_cast<size_t>(4096));
        size_t run_count = 0;
        std::vector<size_t> freq(256, 0);
        
        // Micro-probe: Scan for repeats and byte distribution
        for (size_t i = 0; i < scan_size; ++i) {
            if (i > 0 && block[i] == block[i - 1]) run_count++;
            freq[block[i]]++;
        }
        
        // Entropy check & universal DELTA4 probe
        size_t non_zero = 0;
        size_t max_freq = 0;
        for (size_t f : freq) {
            if (f > 0) non_zero++;
            if (f > max_freq) max_freq = f;
        }

        size_t d4_max_freq = 0;
        if (scan_size >= 4) {
            std::vector<size_t> d4_freq(256, 0);
            for (size_t i = 4; i < scan_size; ++i) {
                uint8_t d = static_cast<uint8_t>(block[i] - block[i - 4]);
                d4_freq[d]++;
            }
            for (size_t f : d4_freq) {
                if (f > d4_max_freq) d4_max_freq = f;
            }
        }

        // Normalize frequencies to a 4096-byte scale so partial blocks don't artificially drop entropy
        size_t norm_max_freq = (max_freq * 4096) / scan_size;
        size_t norm_d4_max_freq = (d4_max_freq * 4096) / scan_size;

        // If data is highly random (uniform distribution) and DELTA4 doesn't rescue it
        if (scan_size >= 4096 && non_zero > 245 && norm_max_freq < 48 && norm_d4_max_freq < 60) {
            return {{AlgorithmType::STORE}, norm_max_freq};
        }

        // If more than 10% of adjacent bytes are identical, RLE will yield massive gains!
        if (run_count > scan_size / 10) {
            return {{AlgorithmType::RLE, AlgorithmType::LZ77, AlgorithmType::MTF, AlgorithmType::NEURAL}, norm_max_freq};
        }

        if (norm_d4_max_freq > norm_max_freq * 1.3 || (norm_max_freq < 60 && norm_d4_max_freq >= 60)) {
            return {{AlgorithmType::DELTA4, AlgorithmType::LZ77, AlgorithmType::MTF, AlgorithmType::NEURAL}, norm_max_freq};
        }

        // Fast path for standard text / data
        return {{AlgorithmType::LZ77, AlgorithmType::MTF, AlgorithmType::NEURAL}, norm_max_freq};
    }

    static std::vector<uint8_t> compress_single_block(const std::vector<uint8_t>& input, const std::vector<AlgorithmType>& pipeline) {
        std::vector<uint8_t> current = input;
        for (AlgorithmType algo : pipeline) {
            switch (algo) {
                case AlgorithmType::DELTA:   current = Delta::compress(current); break;
                case AlgorithmType::LZ77:    current = Lz77::compress(current); break;
                case AlgorithmType::BWT:     current = Bwt::compress(current); break;
                case AlgorithmType::MTF:     current = Mtf::compress(current); break;
                case AlgorithmType::RLE:     current = Rle::compress(current); break;
                case AlgorithmType::HUFFMAN: current = Huffman::compress(current); break;
                case AlgorithmType::NEURAL:  current = Neural::compress(current); break;
                case AlgorithmType::STORE:   break;
                case AlgorithmType::DELTA4:  current = Delta4::compress(current); break;
                default: break;
            }
        }
        return current;
    }

    static std::vector<uint8_t> decompress_single_block(const std::vector<uint8_t>& payload, const std::vector<AlgorithmType>& pipeline) {
        std::vector<uint8_t> current = payload;
        for (auto it = pipeline.rbegin(); it != pipeline.rend(); ++it) {
            switch (*it) {
                case AlgorithmType::DELTA:   current = Delta::decompress(current); break;
                case AlgorithmType::LZ77:    current = Lz77::decompress(current); break;
                case AlgorithmType::BWT:     current = Bwt::decompress(current); break;
                case AlgorithmType::MTF:     current = Mtf::decompress(current); break;
                case AlgorithmType::RLE:     current = Rle::decompress(current); break;
                case AlgorithmType::HUFFMAN: current = Huffman::decompress(current); break;
                case AlgorithmType::NEURAL:  current = Neural::decompress(current); break;
                case AlgorithmType::STORE:   break;
                case AlgorithmType::DELTA4:  current = Delta4::decompress(current); break;
                default: break;
            }
        }
        return current;
    }

    static std::vector<uint8_t> compress(const std::vector<uint8_t>& input,
                                         const std::vector<AlgorithmType>& user_pipeline = {},
                                         bool adaptive = true,
                                         size_t block_size = DEFAULT_BLOCK_SIZE,
                                         unsigned int threads = 0,
                                         size_t entropy_threshold = 200,
                                         bool silent_progress = false) {
        std::vector<uint8_t> output;

        if (block_size == 0) {
            throw std::runtime_error("Invalid block size: 0");
        }

        // 1. Write Header: MAGIC (4B) + VERSION (1B 0x03)
        output.push_back(MAGIC[0]);
        output.push_back(MAGIC[1]);
        output.push_back(MAGIC[2]);
        output.push_back(MAGIC[3]);
        output.push_back(VERSION);

        // Partition input into blocks
        size_t n = input.size();
        size_t offset = 0;
        std::vector<std::vector<uint8_t>> blocks;

        if (n == 0) {
            blocks.push_back({});
        } else {
            if (adaptive) {
                // Bottom-Up Irregular Micro-Block Merge (Content-Defined Chunking)
                size_t MICRO_BLOCK_SIZE = 4096;
                std::vector<AlgorithmType> current_pipeline;
                size_t current_entropy = 0;
                size_t current_variance = 0;
                size_t current_block_start = 0;

                while (offset < n) {
                    size_t micro_sz = std::min(MICRO_BLOCK_SIZE, n - offset);
                    std::vector<uint8_t> micro_block(input.begin() + offset, input.begin() + offset + micro_sz);
                    ProbeResult probe = auto_tune_block(micro_block);

                    if (offset == current_block_start) {
                        current_pipeline = probe.pipeline;
                        current_entropy = probe.max_freq;
                        current_variance = 50; // Initial guess
                    } else {
                        bool pipeline_changed = (probe.pipeline != current_pipeline);
                        int diff = std::abs(static_cast<int>(probe.max_freq) - static_cast<int>(current_entropy));
                        
                        size_t effective_threshold = entropy_threshold;
                        if (entropy_threshold == static_cast<size_t>(-1)) {
                            // Auto-Tuning (-E): Threshold scales with the data's inherent variance
                            effective_threshold = std::max(static_cast<size_t>(30), current_variance * 3);
                        }
                        
                        bool entropy_shifted = (diff > static_cast<int>(effective_threshold));
                        
                        if (pipeline_changed || entropy_shifted || (offset - current_block_start + micro_sz > block_size)) {
                            // Pipeline intent changed, entropy mutated violently, or block size exceeded -> flush irregular block
                            blocks.push_back(std::vector<uint8_t>(input.begin() + current_block_start, input.begin() + offset));
                            current_block_start = offset;
                            current_pipeline = probe.pipeline;
                            current_entropy = probe.max_freq;
                            current_variance = 50;
                        } else {
                            // Smoothly roll the entropy average and variance to adapt to gradual changes
                            current_entropy = (current_entropy * 7 + probe.max_freq) / 8;
                            current_variance = (current_variance * 7 + diff) / 8;
                        }
                    }
                    offset += micro_sz;
                }
                if (current_block_start < n) {
                    blocks.push_back(std::vector<uint8_t>(input.begin() + current_block_start, input.end()));
                }
            } else {
                // Fallback fixed slicing for forced pipelines
                while (offset < n) {
                    size_t sz = std::min(block_size, n - offset);
                    blocks.push_back(std::vector<uint8_t>(input.begin() + offset, input.begin() + offset + sz));
                    offset += sz;
                }
            }
        }

        uint32_t num_blocks = static_cast<uint32_t>(blocks.size());
        // Write num_blocks (4 bytes)
        for (int i = 0; i < 4; ++i) {
            output.push_back(static_cast<uint8_t>((num_blocks >> (i * 8)) & 0xFF));
        }

        // Process blocks in parallel batches
        unsigned int max_threads = threads > 0 ? threads : std::thread::hardware_concurrency();
        if (max_threads == 0) max_threads = 4;

        struct BlockResult {
            std::vector<AlgorithmType> pipeline;
            std::vector<uint8_t> payload;
            uint32_t uncompressed_sz;
        };

        size_t total_blocks = blocks.size();
        for (size_t i = 0; i < total_blocks; i += max_threads) {
            size_t end_idx = std::min(i + max_threads, total_blocks);
            std::vector<std::future<BlockResult>> futures;

            // Launch tasks
            for (size_t j = i; j < end_idx; ++j) {
                futures.push_back(std::async(std::launch::async, [&blocks, j, &user_pipeline, adaptive]() {
                    std::vector<AlgorithmType> pipeline = user_pipeline;
                    if (adaptive || pipeline.empty()) {
                        pipeline = auto_tune_block(blocks[j]).pipeline;
                    }
                    auto payload = compress_single_block(blocks[j], pipeline);
                    
                    // Fallback to STORE if compression inflated the data
                    if (payload.size() >= blocks[j].size()) {
                        pipeline = {AlgorithmType::STORE};
                        payload = blocks[j];
                    }
                    
                    return BlockResult{pipeline, std::move(payload), static_cast<uint32_t>(blocks[j].size())};
                }));
            }

            // Collect results in order
            for (auto& fut : futures) {
                BlockResult res = fut.get();
                
                // Block header
                uint8_t count = static_cast<uint8_t>(res.pipeline.size());
                output.push_back(count);
                for (AlgorithmType algo : res.pipeline) {
                    output.push_back(static_cast<uint8_t>(algo));
                }

                uint32_t uncompressed_sz = res.uncompressed_sz;
                for (int k = 0; k < 4; ++k) {
                    output.push_back(static_cast<uint8_t>((uncompressed_sz >> (k * 8)) & 0xFF));
                }

                uint32_t compressed_sz = static_cast<uint32_t>(res.payload.size());
                for (int k = 0; k < 4; ++k) {
                    output.push_back(static_cast<uint8_t>((compressed_sz >> (k * 8)) & 0xFF));
                }

                output.insert(output.end(), res.payload.begin(), res.payload.end());
            }

            // Print progress bar
            if (!silent_progress) {
                int progress = static_cast<int>((static_cast<float>(end_idx) / total_blocks) * 100);
                std::cerr << "\r[sxzip] Compressing... [";
                for (int p = 0; p < 40; ++p) {
                    if (p < (progress * 40 / 100)) std::cerr << "=";
                    else if (p == (progress * 40 / 100)) std::cerr << ">";
                    else std::cerr << " ";
                }
                std::cerr << "] " << progress << "% (" << end_idx << "/" << total_blocks << " blocks)" << std::flush;
            }
        }
        if (!silent_progress) std::cerr << std::endl;

        return output;
    }

    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& input) {
        if (input.size() < 6) {
            throw std::runtime_error("Invalid .sxz file: Header too small");
        }

        // Validate Magic
        if (input[0] != MAGIC[0] || input[1] != MAGIC[1] || input[2] != MAGIC[2] || input[3] != MAGIC[3]) {
            throw std::runtime_error("Invalid .sxz file: Magic header mismatch (expected SXZP)");
        }

        uint8_t ver = input[4];
        if (ver == 0x01) { // Fallback v0.1 format
            uint8_t mode = input[5];
            std::vector<AlgorithmType> pipeline = {AlgorithmType::RLE, AlgorithmType::HUFFMAN};
            if (mode == 0x01) pipeline = {AlgorithmType::HUFFMAN};
            else if (mode == 0x02) pipeline = {AlgorithmType::RLE};
            std::vector<uint8_t> payload(input.begin() + 6, input.end());
            return decompress_single_block(payload, pipeline);
        } else if (ver == 0x02) { // Fallback v0.2 format
            uint8_t count = input[5];
            std::vector<AlgorithmType> pipeline;
            for (uint8_t i = 0; i < count; ++i) {
                pipeline.push_back(static_cast<AlgorithmType>(input[6 + i]));
            }
            std::vector<uint8_t> payload(input.begin() + 6 + count, input.end());
            return decompress_single_block(payload, pipeline);
        } else if (ver >= 0x03 && ver <= 0x05) { // v0.3 to v0.5 Block-Based Adaptive format
            if (input.size() < 9) {
                throw std::runtime_error("Invalid .sxz file: Truncated v0.3 header");
            }

            uint32_t num_blocks = 0;
            for (int i = 0; i < 4; ++i) {
                num_blocks |= (static_cast<uint32_t>(input[5 + i]) << (i * 8));
            }

            size_t idx = 9;
            std::vector<uint8_t> output;

            for (uint32_t b = 0; b < num_blocks; ++b) {
                if (idx >= input.size()) {
                    throw std::runtime_error("Corrupted .sxz file: Truncated block header");
                }

                uint8_t count = input[idx++];
                std::vector<AlgorithmType> pipeline;
                for (uint8_t i = 0; i < count; ++i) {
                    if (idx >= input.size()) throw std::runtime_error("Corrupted .sxz file: Truncated algorithm list");
                    pipeline.push_back(static_cast<AlgorithmType>(input[idx++]));
                }

                if (idx + 8 > input.size()) {
                    throw std::runtime_error("Corrupted .sxz file: Truncated block metadata");
                }

                uint32_t uncompressed_sz = 0;
                for (int i = 0; i < 4; ++i) {
                    uncompressed_sz |= (static_cast<uint32_t>(input[idx++]) << (i * 8));
                }
                (void)uncompressed_sz;

                uint32_t compressed_sz = 0;
                for (int i = 0; i < 4; ++i) {
                    compressed_sz |= (static_cast<uint32_t>(input[idx++]) << (i * 8));
                }

                if (idx + compressed_sz > input.size()) {
                    throw std::runtime_error("Corrupted .sxz file: Truncated block payload");
                }

                std::vector<uint8_t> payload(input.begin() + idx, input.begin() + idx + compressed_sz);
                idx += compressed_sz;

                auto decomp_block = decompress_single_block(payload, pipeline);
                output.insert(output.end(), decomp_block.begin(), decomp_block.end());
            }

            return output;
        }

        throw std::runtime_error("Unsupported SXZP format version: " + std::to_string(ver));
    }

    static SzipFileInfo get_info(const std::vector<uint8_t>& input) {
        if (input.size() < 6 ||
            input[0] != MAGIC[0] || input[1] != MAGIC[1] ||
            input[2] != MAGIC[2] || input[3] != MAGIC[3]) {
            throw std::runtime_error("Not a valid SXZP file");
        }

        SzipFileInfo info;
        info.version = input[4];
        info.total_compressed_size = input.size();

        if (info.version == 0x01 || info.version == 0x02) {
            BlockInfo binfo;
            binfo.block_index = 0;
            if (info.version == 0x01) {
                binfo.pipeline = {AlgorithmType::RLE, AlgorithmType::HUFFMAN};
            } else {
                uint8_t count = input[5];
                for (uint8_t i = 0; i < count; ++i) {
                    binfo.pipeline.push_back(static_cast<AlgorithmType>(input[6 + i]));
                }
            }
            binfo.compressed_size = static_cast<uint32_t>(input.size());
            info.blocks.push_back(binfo);
        } else if (info.version >= 0x03 && info.version <= 0x05) {
            uint32_t num_blocks = 0;
            for (int i = 0; i < 4; ++i) {
                num_blocks |= (static_cast<uint32_t>(input[5 + i]) << (i * 8));
            }

            size_t idx = 9;
            for (uint32_t b = 0; b < num_blocks; ++b) {
                BlockInfo binfo;
                binfo.block_index = b;
                uint8_t count = input[idx++];
                for (uint8_t i = 0; i < count; ++i) {
                    binfo.pipeline.push_back(static_cast<AlgorithmType>(input[idx++]));
                }

                binfo.uncompressed_size = 0;
                for (int i = 0; i < 4; ++i) {
                    binfo.uncompressed_size |= (static_cast<uint32_t>(input[idx++]) << (i * 8));
                }

                binfo.compressed_size = 0;
                for (int i = 0; i < 4; ++i) {
                    binfo.compressed_size |= (static_cast<uint32_t>(input[idx++]) << (i * 8));
                }

                idx += binfo.compressed_size;
                info.total_uncompressed_size += binfo.uncompressed_size;
                info.blocks.push_back(binfo);
            }
        }

        return info;
    }
};

} // namespace sxzip

#endif // SXZP_HEADER_HPP
