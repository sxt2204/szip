#include "sxzip_header.hpp"
#include "tui.hpp"
#include <iostream>
#include <fstream> // IWYU pragma: keep
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <cstdlib>

namespace {

std::vector<uint8_t> read_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for reading: " + filepath);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (size > 0 && !file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file contents: " + filepath);
    }

    return buffer;
}

void write_file(const std::string& filepath, const std::vector<uint8_t>& data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + filepath);
    }

    if (!data.empty() && !file.write(reinterpret_cast<const char*>(data.data()), data.size())) {
        throw std::runtime_error("Failed to write data to file: " + filepath);
    }
}

void print_usage(const char* prog_name) {
    std::cout << "Sxzip (sxzip) v0.3 - Block-Based Adaptive Data Compressor\n"
              << "Usage:\n"
              << "  Compress:    " << prog_name << " -c <input> <output.sxz> [options]\n"
              << "  Decompress:  " << prog_name << " -d <input.sxz> <output>\n"
              << "  Info:        " << prog_name << " -i <input.sxz>\n"
              << "  Evaluate:    " << prog_name << " -Ev <input>\n"
              << "  Interactive: " << prog_name << " (run without arguments)\n\n"
              << "Options:\n"
              << "  -a, --adaptive     Enable block-level adaptive algorithm selection (default)\n"
              << "  -b, --block-size   Maximum block size limit in KB (default: 16384KB = 16MB)\n"
              << "  -p, --pipeline     Force specific algorithm pipeline across all blocks\n"
              << "  -e, --entropy      Entropy-boundary sensitivity for block chunking (default: 200)\n"
              << "  -E, --auto-entropy Enable dynamic entropy auto-tuning based on variance\n"
              << "  -Ea, --brute-force Brute-force search for the optimal entropy threshold\n"
              << "  -t, --threads      Number of threads to use (default: 0 = auto)\n\n"
              << "Pipeline string format:\n"
              << "  e.g., 'LZ77,HUFFMAN' or 'BWT,MTF,RLE,NEURAL'\n";
}

std::string format_size(size_t bytes) {
    double size = static_cast<double>(bytes);
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int u = 0;
    while (size >= 1024.0 && u < 5) {
        size /= 1024.0;
        u++;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[u];
    return ss.str();
}

std::string pipeline_to_string(const std::vector<sxzip::AlgorithmType>& pipeline) {
    std::string result;
    for (size_t i = 0; i < pipeline.size(); ++i) {
        if (i > 0) result += " -> ";
        result += sxzip::algorithm_to_string(pipeline[i]);
    }
    return result;
}

} // anonymous namespace

int sxzip_cli(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::string flag = argv[1];

    try {
        bool is_eval = (flag == "-Ev" || flag == "--eval");
        if (flag == "-c" || flag == "--compress" || is_eval) {
            if (!is_eval && argc < 4) {
                print_usage(argv[0]);
                return 1;
            }
            if (is_eval && argc < 3) {
                print_usage(argv[0]);
                return 1;
            }
            std::string input_path = argv[2];
            std::string output_path = is_eval ? "" : argv[3];

            bool adaptive = true;
            size_t block_size = sxzip::SxzipEngine::DEFAULT_BLOCK_SIZE;
            unsigned int threads = 0;
            size_t entropy_threshold = is_eval ? static_cast<size_t>(-3) : 200;
            std::vector<sxzip::AlgorithmType> forced_pipeline;

            int arg_start = is_eval ? 3 : 4;
            for (int i = arg_start; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "--adaptive" || arg == "-a") {
                    adaptive = true;
                } else if (arg == "--pipeline" || arg == "-p") {
                    if (i + 1 < argc) {
                        forced_pipeline = sxzip::SxzipEngine::parse_pipeline_str(argv[++i]);
                        adaptive = false;
                    }
                } else if (arg == "--block-size" || arg == "-b") {
                    if (i + 1 < argc) {
                        unsigned long long kb = std::stoull(argv[++i]);
                        if (kb == 0) {
                            throw std::runtime_error("Block size must be greater than 0 KB");
                        }
                        if (kb > (std::numeric_limits<size_t>::max() / 1024)) {
                            block_size = std::numeric_limits<size_t>::max();
                        } else {
                            block_size = static_cast<size_t>(kb * 1024);
                        }
                    }
                } else if (arg == "--threads" || arg == "-t") {
                    if (i + 1 < argc) {
                        threads = std::stoul(argv[++i]);
                    }
                } else if (arg == "--entropy" || arg == "-e") {
                    if (i + 1 < argc) {
                        entropy_threshold = std::stoull(argv[++i]);
                    }
                } else if (arg == "--auto-entropy" || arg == "-E") {
                    entropy_threshold = static_cast<size_t>(-1); // SIZE_MAX
                } else if (arg == "--brute-force" || arg == "-Ea") {
                    entropy_threshold = static_cast<size_t>(-2); // SIZE_MAX - 1
                }
            }

            std::string actual_input_path = input_path;
            bool is_temp_tar = false;

            bool silent_eval = is_eval;
            
            if (std::filesystem::is_directory(input_path)) {
                if (!silent_eval) std::cout << "[sxzip] Directory detected! Packing with tar first...\n";
                actual_input_path = "szip_temp_dir.tar";
                std::string cmd = "tar -cf " + actual_input_path + " " + input_path;
                if (std::system(cmd.c_str()) != 0) {
                    throw std::runtime_error("Failed to pack directory with tar.");
                }
                is_temp_tar = true;
            }

            if (!silent_eval) std::cout << "[sxzip] Reading " << actual_input_path << "...\n";
            auto input_data = read_file(actual_input_path);
            
            if (is_temp_tar) {
                std::filesystem::remove(actual_input_path);
            }

            auto start_time = std::chrono::high_resolution_clock::now();

            if (!silent_eval) {
                if (adaptive) {
                    std::string sens_str = std::to_string(entropy_threshold);
                    if (entropy_threshold == static_cast<size_t>(-1)) sens_str = "AUTO (-E)";
                    else if (entropy_threshold == static_cast<size_t>(-2)) sens_str = "BRUTE-FORCE (-Ea)";
                    
                    std::cout << "[sxzip Adaptive Engine] Compressing with Block-Based Auto-Tuning (Block Size: "
                              << format_size(block_size) << ", Entropy Sensitivity: " << sens_str << ")...\n";
                } else {
                    std::cout << "[sxzip Pipeline Engine] Forcing pipeline chain: [" << pipeline_to_string(forced_pipeline) << "]\n";
                }
                if (threads > 0) {
                    std::cout << "[sxzip] Using " << threads << " threads for compression.\n";
                }
            }

            std::vector<uint8_t> compressed_data;
            if (entropy_threshold == static_cast<size_t>(-2) || silent_eval) {
                if (!silent_eval) std::cout << "[sxzip Brute-Force] Sweeping entropy thresholds to find global optimum...\n";
                std::vector<size_t> candidates = {0, 10, 30, 50, 100, 200, 500, 1000, 2000, 4096};
                size_t best_size = std::numeric_limits<size_t>::max();
                size_t best_thresh = 0;
                std::vector<uint8_t> best_data;

                std::vector<std::pair<size_t, size_t>> results;
                for (size_t cand : candidates) {
                    bool inner_silent = true; // Always silence intermediate progress bars
                    auto temp_data = sxzip::SxzipEngine::compress(input_data, forced_pipeline, adaptive, block_size, threads, cand, inner_silent);
                    if (!silent_eval) std::cout << "  -> Threshold " << std::setw(4) << cand << " yields " << temp_data.size() << " bytes\n";
                    results.push_back({cand, temp_data.size()});
                    if (temp_data.size() < best_size) {
                        best_size = temp_data.size();
                        best_data = std::move(temp_data);
                        best_thresh = cand;
                    }
                }
                
                if (silent_eval) {
                    bool first = true;
                    for (const auto& r : results) {
                        if (r.second <= best_size * 1.01) { // within 1% of the best size
                            if (!first) std::cout << " ";
                            std::cout << r.first;
                            first = false;
                        }
                    }
                    std::cout << std::endl;
                    return 0;
                }
                
                std::cout << "[sxzip Brute-Force] Optimal threshold found: " << best_thresh << " (" << best_size << " bytes)\n";
                compressed_data = std::move(best_data);
            } else {
                compressed_data = sxzip::SxzipEngine::compress(input_data, forced_pipeline, adaptive, block_size, threads, entropy_threshold, silent_eval);
            }
            
            auto end_time = std::chrono::high_resolution_clock::now();

            write_file(output_path, compressed_data);

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            double ratio = input_data.empty() ? 0.0 : (1.0 - (double)compressed_data.size() / input_data.size()) * 100.0;

            std::cout << "[sxzip] Block Compression complete!\n"
                      << "----------------------------------------\n"
                      << "  Original Size:   " << format_size(input_data.size()) << " (" << input_data.size() << " bytes)\n"
                      << "  Compressed Size: " << format_size(compressed_data.size()) << " (" << compressed_data.size() << " bytes)\n"
                      << "  Space Saved:     " << std::fixed << std::setprecision(2) << ratio << "%\n"
                      << "  Time Elapsed:    " << duration << " ms\n"
                      << "----------------------------------------\n";

        } else if (flag == "-d" || flag == "--decompress") {
            if (argc < 4) {
                print_usage(argv[0]);
                return 1;
            }
            std::string input_path = argv[2];
            std::string output_path = argv[3];

            std::cout << "[sxzip] Reading compressed file " << input_path << "...\n";
            auto compressed_data = read_file(input_path);

            auto start_time = std::chrono::high_resolution_clock::now();
            auto decompressed_data = sxzip::SxzipEngine::decompress(compressed_data);
            auto end_time = std::chrono::high_resolution_clock::now();

            write_file(output_path, decompressed_data);

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

            std::cout << "[sxzip] Decompression complete!\n"
                      << "----------------------------------------\n"
                      << "  Output Size:     " << format_size(decompressed_data.size()) << " (" << decompressed_data.size() << " bytes)\n"
                      << "  Time Elapsed:    " << duration << " ms\n"
                      << "----------------------------------------\n";
                      
            // Auto-unpack if it is a tarball
            if (decompressed_data.size() > 265) {
                std::string magic(decompressed_data.begin() + 257, decompressed_data.begin() + 262);
                if (magic == "ustar") {
                    std::cout << "[sxzip] Auto-detect: Tar archive payload. Unpacking into current directory...\n";
                    std::string cmd = "tar -xf " + output_path;
                    if (std::system(cmd.c_str()) == 0) {
                        std::filesystem::remove(output_path); // Clean up intermediate tar file
                        std::cout << "[sxzip] Unpacking complete! Temporary tarball removed.\n";
                    } else {
                        std::cerr << "[sxzip] Warning: Failed to extract tar archive.\n";
                    }
                }
            }

        } else if (flag == "-i" || flag == "--info") {
            std::string input_path = argv[2];
            auto compressed_data = read_file(input_path);
            auto info = sxzip::SxzipEngine::get_info(compressed_data);

            std::cout << "[sxzip File Information]\n"
                      << "----------------------------------------\n"
                      << "  Format Magic:    SXZP\n"
                      << "  Version:         v" << (int)info.version << "\n"
                      << "  Total Blocks:    " << info.blocks.size() << "\n"
                      << "  Compressed Size: " << format_size(info.total_compressed_size) << "\n";

            if (info.version == 0x03) {
                std::cout << "----------------------------------------\n"
                          << "  [Block Breakdown]\n";
                for (const auto& b : info.blocks) {
                    std::cout << "   Block #" << b.block_index + 1 << ": "
                              << format_size(b.uncompressed_size) << " -> " << format_size(b.compressed_size)
                              << " | Chain: [" << pipeline_to_string(b.pipeline) << "]\n";
                }
            }
            std::cout << "----------------------------------------\n";
        } else {
            std::cerr << "Unknown option: " << flag << "\n";
            print_usage(argv[0]);
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

namespace sxzip {
namespace tui {

int execute_cli(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    for (size_t i = 0; i < args.size(); ++i) {
        argv.push_back(const_cast<char*>(args[i].c_str()));
    }
    // We catch exceptions to prevent the TUI from crashing due to CLI errors
    try {
        return sxzip_cli(static_cast<int>(argv.size()), argv.data());
    } catch (const std::exception& e) {
        std::cerr << "Error executing command: " << e.what() << std::endl;
        return 1;
    }
}

} // namespace tui
} // namespace sxzip

int main(int argc, char* argv[]) {
    if (argc == 1) {
        return sxzip::tui::run();
    } else {
        return sxzip_cli(argc, argv);
    }
}
