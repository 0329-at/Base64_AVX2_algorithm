#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace ref {

inline constexpr std::string_view kStdChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
inline constexpr std::string_view kUrlChars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

enum class Alphabet { Standard, UrlSafe };

inline const std::string_view& chars(Alphabet a) {
    return a == Alphabet::Standard ? kStdChars : kUrlChars;
}

inline std::string encode(std::string_view in, Alphabet a = Alphabet::Standard) {
    const auto& tbl = chars(a);
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    const std::size_t n = in.size();
    for (std::size_t i = 0; i < n; i += 3) {
        std::uint32_t t = 0;
        int bytes = 0;
        for (int j = 0; j < 3 && i + (std::size_t)j < n; ++j) {
            t = (t << 8) | (unsigned char)in[i + j];
            ++bytes;
        }
        t <<= (3 - bytes) * 8;
        for (int s = 18; s >= 0; s -= 6) out += tbl[(t >> s) & 0x3F];
        if (bytes == 1) { out[out.size() - 1] = '='; out[out.size() - 2] = '='; }
        else if (bytes == 2) { out[out.size() - 1] = '='; }
    }
    return out;
}

inline bool decode(std::string_view in, std::string& out, Alphabet a = Alphabet::Standard) {
    out.clear();
    if (in.empty()) return true;
    if ((in.size() % 4) != 0) return false;

    const char c62 = (a == Alphabet::Standard) ? '+' : '-';
    const char c63 = (a == Alphabet::Standard) ? '/' : '_';

    auto val = [&](unsigned char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == (unsigned char)c62) return 62;
        if (c == (unsigned char)c63) return 63;
        return -1;
    };

    std::size_t data_len = in.size();
    if (in[in.size() - 1] == '=') {
        --data_len;
        if (in.size() >= 2 && in[in.size() - 2] == '=') {
            --data_len;
            if (in.size() >= 3 && in[in.size() - 3] == '=') return false;
        }
    }
    for (std::size_t i = 0; i < data_len; ++i)
        if (val((unsigned char)in[i]) < 0) return false;

    out.reserve(in.size() / 4 * 3 + 2);
    for (std::size_t i = 0; i < in.size(); i += 4) {
        std::uint32_t quad = 0;
        int valid = 0;
        for (int j = 0; j < 4; ++j) {
            unsigned char c = (unsigned char)in[i + j];
            if (c == '=') continue;
            int d = val(c);
            if (d < 0) return false;
            quad = (quad << 6) | (std::uint32_t)d;
            ++valid;
        }
        quad <<= (4 - valid) * 6;
        if (valid >= 2) out += (char)((quad >> 16) & 0xFF);
        if (valid >= 3) out += (char)((quad >>  8) & 0xFF);
        if (valid >= 4) out += (char)( quad        & 0xFF);
    }
    return true;
}

} // namespace ref