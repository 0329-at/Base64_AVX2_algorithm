#pragma once
#include <cstdint>
#include <random>
#include <string>

namespace test_util {

inline std::string hex(const std::string& s, std::size_t max_len = 32) {
    static const char* h = "0123456789abcdef";
    std::string r;
    const std::size_t L = (s.size() < max_len) ? s.size() : max_len;
    for (std::size_t i = 0; i < L; ++i) {
        unsigned char c = (unsigned char)s[i];
        r += h[c >> 4];
        r += h[c & 0xF];
    }
    if (s.size() > max_len) r += "...";
    return r;
}

inline std::string random_string(std::mt19937& rng, std::size_t len) {
    std::string s(len, '\0');
    for (auto& b : s) b = (char)(rng() & 0xFF);
    return s;
}

} // namespace test_util