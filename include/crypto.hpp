#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

namespace sxzip {
namespace crypto {

class StreamCipher {
private:
    uint8_t S[256];
    int i, j;

public:
    // Initialize the cipher state (RC4 variant) using password and salt
    StreamCipher(const std::string& password, uint64_t salt) {
        // Construct the full key: password + salt
        std::string full_key = password;
        for (int k = 0; k < 8; ++k) {
            full_key += static_cast<char>((salt >> (k * 8)) & 0xFF);
        }

        // Key-Scheduling Algorithm (KSA)
        for (int k = 0; k < 256; ++k) {
            S[k] = static_cast<uint8_t>(k);
        }

        int j_init = 0;
        for (int k = 0; k < 256; ++k) {
            j_init = (j_init + S[k] + full_key[k % full_key.length()]) % 256;
            std::swap(S[k], S[j_init]);
        }

        i = 0;
        j = 0;

        // Discard the first 1024 bytes (RC4-drop1024) to mitigate weak keys
        for (int k = 0; k < 1024; ++k) {
            next_byte();
        }
    }

    // Pseudo-Random Generation Algorithm (PRGA)
    uint8_t next_byte() {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        std::swap(S[i], S[j]);
        return S[(S[i] + S[j]) % 256];
    }

    // Encrypt or decrypt data in-place (XOR is symmetric)
    void process(std::vector<uint8_t>& data) {
        for (size_t k = 0; k < data.size(); ++k) {
            data[k] ^= next_byte();
        }
    }
};

} // namespace crypto
} // namespace sxzip
