#pragma once
#include <immintrin.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#if defined(__GNUC__) || defined(__clang__)
    #define BASE64_AVX2_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define BASE64_AVX2_FORCE_INLINE __forceinline
#else
    #define BASE64_AVX2_FORCE_INLINE inline
#endif

#define BASE64_AVX2_VERSION_MAJOR 0
#define BASE64_AVX2_VERSION_MINOR 3
#define BASE64_AVX2_VERSION_PATCH 0

namespace base64_avx2 {

enum class Mode {
    Standard,   // A-Z a-z 0-9 + /
    UrlSafe,    // A-Z a-z 0-9 - _
};

class DecodeError : public std::invalid_argument {
public:
    DecodeError(std::string msg, std::size_t position)
        : std::invalid_argument(std::move(msg))
        , position_(position) {}

    constexpr std::size_t position() const noexcept { return position_; }

private:
    std::size_t position_;
};

class Base64 {
public:
    constexpr Base64() noexcept = default;
    explicit constexpr Base64(Mode m) noexcept : mode_(m) {}

    void set(Mode m) noexcept { mode_ = m; }
    Mode mode() const noexcept { return mode_; }

    constexpr std::string encode(std::string_view input) const {
        const std::size_t n = input.size();
        if (n == 0) return {};

        std::string out((n + 2) / 3 * 4, '\0');
        const std::uint8_t* __restrict src = (const std::uint8_t*)input.data();
        std::uint8_t* __restrict dst       = (std::uint8_t*)out.data();

        const char* table = (mode_ == Mode::Standard) ? kStdChars : kUrlChars;

        std::size_t pos = 0, dpos = 0;

        if (n >= 24) {
            const __m256i m24  = _mm256_set_epi32(0, 0, -1, -1, -1, -1, -1, -1);
            const __m256i perm = _mm256_set_epi32(6, 5, 4, 3, 3, 2, 1, 0);
            const __m256i shuf = _mm256_setr_epi8(
                2,1,0,(char)0x80, 5,4,3,(char)0x80,
                8,7,6,(char)0x80, 11,10,9,(char)0x80,
                2,1,0,(char)0x80, 5,4,3,(char)0x80,
                8,7,6,(char)0x80, 11,10,9,(char)0x80);
            const __m256i v3F = _mm256_set1_epi32(0x3F);

            const __m256i v51 = _mm256_set1_epi8(51);
            const __m256i v26 = _mm256_set1_epi8(26);
            const __m256i v13 = _mm256_set1_epi8(13);
            const int off62 = (int)(unsigned char)c62(mode_) - 62;
            const int off63 = (int)(unsigned char)c63(mode_) - 63;
            // 偏移表（按 6-bit 值的简化值索引）：
            //   简化值 0  → +71  输入 26..51 → 'a'..'z'
            //   简化值 1..10 → -4  输入 52..61 → '0'..'9'
            //   简化值 11 → off62  输入 62 → '+' 或 '-'
            //   简化值 12 → off63  输入 63 → '/' 或 '_'
            //   简化值 13 → +65  输入 0..25 → 'A'..'Z'
            const __m256i offsets = _mm256_setr_epi8(
                71, -4, -4, -4, -4, -4, -4, -4,
                -4, -4, -4, (char)off62, (char)off63, 65, 0, 0,
                71, -4, -4, -4, -4, -4, -4, -4,
                -4, -4, -4, (char)off62, (char)off63, 65, 0, 0);

            auto encode_block = [&](const std::uint8_t* s, std::uint8_t* d) {
                __m256i v = _mm256_maskload_epi32((const int*)s, m24);
                __m256i w = _mm256_permutevar8x32_epi32(v, perm);
                __m256i sh = _mm256_shuffle_epi8(w, shuf);

                __m256i c3 = _mm256_and_si256(sh, v3F);
                __m256i c2 = _mm256_and_si256(_mm256_srli_epi32(sh,  6), v3F);
                __m256i c1 = _mm256_and_si256(_mm256_srli_epi32(sh, 12), v3F);
                __m256i c0 = _mm256_srli_epi32(sh, 18);

                __m256i r = _mm256_or_si256(
                    _mm256_or_si256(_mm256_slli_epi32(c3, 24), _mm256_slli_epi32(c2, 16)),
                    _mm256_or_si256(_mm256_slli_epi32(c1,  8), c0));

                __m256i result = _mm256_subs_epu8(r, v51);          // r<51 → 0
                __m256i less = _mm256_cmpgt_epi8(v26, r);           // r<26 → 0xFF
                result = _mm256_or_si256(result,
                          _mm256_and_si256(less, v13));             // r<26 → 13
                result = _mm256_shuffle_epi8(offsets, result);
                __m256i ch = _mm256_add_epi8(result, r);

                _mm256_storeu_si256((__m256i*)d, ch);
            };

            while (pos + 48 <= n) {
                _mm_prefetch((const char*)(src + pos + 512), _MM_HINT_T0);
                encode_block(src + pos,      dst + dpos);
                encode_block(src + pos + 24, dst + dpos + 32);
                pos  += 48;
                dpos += 64;
            }
            while (pos + 24 <= n) {
                encode_block(src + pos, dst + dpos);
                pos  += 24;
                dpos += 32;
            }
            const std::size_t rem = n - pos;
            if (rem > 0) {
                alignas(32) std::uint8_t tmp[24] = {};
                std::memcpy(tmp, src + pos, rem);
                alignas(32) std::uint8_t tmp_out[32];
                encode_block(tmp, tmp_out);
                const std::size_t tail_len = (rem + 2) / 3 * 4;
                if (rem % 3 == 1) {
                    tmp_out[tail_len - 2] = '=';
                    tmp_out[tail_len - 1] = '=';
                } else if (rem % 3 == 2) {
                    tmp_out[tail_len - 1] = '=';
                }
                std::memcpy(dst + dpos, tmp_out, tail_len);
            }
        } else {
            while (pos < n) {
                const std::size_t rem = n - pos;
                std::uint32_t t = 0;
                if (rem >= 1) t |= (std::uint32_t)src[pos]     << 16;
                if (rem >= 2) t |= (std::uint32_t)src[pos + 1] <<  8;
                if (rem >= 3) t |= (std::uint32_t)src[pos + 2];
                dst[dpos + 0] = (std::uint8_t)table[(t >> 18) & 0x3F];
                dst[dpos + 1] = (std::uint8_t)table[(t >> 12) & 0x3F];
                dst[dpos + 2] = (rem >= 2) ? (std::uint8_t)table[(t >> 6) & 0x3F] : '=';
                dst[dpos + 3] = (rem >= 3) ? (std::uint8_t)table[t & 0x3F]        : '=';
                dpos += 4;
                pos  += (rem >= 3) ? 3 : rem;
            }
        }
        return out;
    }

    constexpr bool validate(std::string_view input) const noexcept {
        const std::size_t n = input.size();
        if (n == 0) return true;
        if (n % 4 != 0) return false;

        int pad = 0;
        if (input[n - 1] == '=') {
            pad = 1;
            if (n >= 2 && input[n - 2] == '=') {
                pad = 2;
                if (n >= 3 && input[n - 3] == '=') return false;
            }
        }
        const std::size_t data_len = n - (std::size_t)pad;

        for (std::size_t i = 0; i < data_len; ++i) {
            if (input[i] == '=') return false;
        }

        const std::uint8_t* src = (const std::uint8_t*)input.data();
        std::size_t i = 0;
        if (data_len >= 32) {
            const __m256i vA_1 = _mm256_set1_epi8('A' - 1);
            const __m256i vZ_1 = _mm256_set1_epi8('Z' + 1);
            const __m256i va_1 = _mm256_set1_epi8('a' - 1);
            const __m256i vz_1 = _mm256_set1_epi8('z' + 1);
            const __m256i v0_1 = _mm256_set1_epi8('0' - 1);
            const __m256i v9_1 = _mm256_set1_epi8('9' + 1);
            const __m256i vPlus  = _mm256_set1_epi8(c62(mode_));
            const __m256i vSlash = _mm256_set1_epi8(c63(mode_));

            while (i + 32 <= data_len) {
                __m256i c = _mm256_loadu_si256((const __m256i*)(src + i));
                __m256i m_upper = _mm256_and_si256(
                    _mm256_cmpgt_epi8(c, vA_1), _mm256_cmpgt_epi8(vZ_1, c));
                __m256i m_lower = _mm256_and_si256(
                    _mm256_cmpgt_epi8(c, va_1), _mm256_cmpgt_epi8(vz_1, c));
                __m256i m_digit = _mm256_and_si256(
                    _mm256_cmpgt_epi8(c, v0_1), _mm256_cmpgt_epi8(v9_1, c));
                __m256i m_plus  = _mm256_cmpeq_epi8(c, vPlus);
                __m256i m_slash = _mm256_cmpeq_epi8(c, vSlash);
                __m256i valid = _mm256_or_si256(
                    _mm256_or_si256(m_upper, _mm256_or_si256(m_lower, m_digit)),
                    _mm256_or_si256(m_plus, m_slash));
                if (_mm256_movemask_epi8(valid) != -1) return false;
                i += 32;
            }
        }

        const char cp = c62(mode_);
        const char cs = c63(mode_);
        for (; i < data_len; ++i) {
            const unsigned char ch = src[i];
            const bool ok =
                (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') ||
                ch == (unsigned char)cp || ch == (unsigned char)cs;
            if (!ok) return false;
        }
        return true;
    }

    constexpr std::string decode(std::string_view input) const {
        const std::size_t n = input.size();
        if (n == 0) return {};

        // ---- 校验 ----
        check_or_throw(input);

        // ---- 计算输出长度 ----
        int pad = 0;
        if (input[n - 1] == '=') {
            ++pad;
            if (n >= 2 && input[n - 2] == '=') ++pad;
        }
        const std::size_t out_len = n / 4 * 3 - (std::size_t)pad;

        std::string out(out_len + 8, '\0');
        const std::uint8_t* __restrict src = (const std::uint8_t*)input.data();
        std::uint8_t* __restrict dst       = (std::uint8_t*)out.data();

        const char cp = c62(mode_);
        const char cs = c63(mode_);

        const __m256i vA   = _mm256_set1_epi8('A');
        const __m256i va_1 = _mm256_set1_epi8('a' - 1);
        const __m256i vz_1 = _mm256_set1_epi8('z' + 1);
        const __m256i v0_1 = _mm256_set1_epi8('0' - 1);
        const __m256i v9_1 = _mm256_set1_epi8('9' + 1);
        const __m256i vPlus  = _mm256_set1_epi8(cp);
        const __m256i vSlash = _mm256_set1_epi8(cs);
        const __m256i v6  = _mm256_set1_epi8(6);
        const __m256i v69 = _mm256_set1_epi8(69);
        const __m256i vFixPlus  = _mm256_set1_epi8((char)(127 - (int)(unsigned char)cp));
        const __m256i vFixSlash = _mm256_set1_epi8((char)(128 - (int)(unsigned char)cs));

        const __m256i madd1 = _mm256_set1_epi32(0x01400140);
        const __m256i madd2 = _mm256_set1_epi32(0x00011000);
        const __m256i pack_shuf = _mm256_setr_epi8(
            2,1,0, 6,5,4, 10,9,8, 14,13,12, -1,-1,-1,-1,
            2,1,0, 6,5,4, 10,9,8, 14,13,12, -1,-1,-1,-1);
        const __m256i pack_perm = _mm256_setr_epi32(0, 1, 2, 4, 5, 6, -1, -1);

        std::size_t pos = 0, dpos = 0;

        while (pos + 32 <= n) {
            _mm_prefetch((const char*)(src + pos + 512), _MM_HINT_T0);
            __m256i c = _mm256_loadu_si256((const __m256i*)(src + pos));

            __m256i v = _mm256_sub_epi8(c, vA);
            v = _mm256_sub_epi8(v, _mm256_and_si256(
                _mm256_and_si256(_mm256_cmpgt_epi8(c, va_1),
                                 _mm256_cmpgt_epi8(vz_1, c)), v6));
            v = _mm256_add_epi8(v, _mm256_and_si256(
                _mm256_and_si256(_mm256_cmpgt_epi8(c, v0_1),
                                 _mm256_cmpgt_epi8(v9_1, c)), v69));
            v = _mm256_add_epi8(v, _mm256_and_si256(
                _mm256_cmpeq_epi8(c, vPlus), vFixPlus));
            v = _mm256_add_epi8(v, _mm256_and_si256(
                _mm256_cmpeq_epi8(c, vSlash), vFixSlash));

            __m256i merged = _mm256_maddubs_epi16(v, madd1);
            __m256i packed = _mm256_madd_epi16(merged, madd2);
            packed = _mm256_shuffle_epi8(packed, pack_shuf);
            packed = _mm256_permutevar8x32_epi32(packed, pack_perm);

            _mm256_storeu_si256((__m256i*)(dst + dpos), packed);
            pos  += 32;
            dpos += 24;
        }

        // 标量尾部
        while (pos < n) {
            std::uint32_t quad = 0;
            int v = 0;
            for (int j = 0; j < 4; ++j) {
                const unsigned char ch = src[pos + j];
                if (ch == '=') continue;
                quad = (quad << 6) | (std::uint32_t)decode_char(ch);
                ++v;
            }
            quad <<= (4 - v) * 6;
            if (v >= 2) dst[dpos++] = (char)((quad >> 16) & 0xFF);
            if (v >= 3) dst[dpos++] = (char)((quad >>  8) & 0xFF);
            if (v >= 4) dst[dpos++] = (char)( quad        & 0xFF);
            pos += 4;
        }

        out.resize(out_len);
        return out;
    }

private:

    void constexpr check_or_throw(std::string_view input) const {
        const std::size_t n = input.size();
        if (n == 0) return;

        if (n % 4 != 0) {
            throw DecodeError(
                "base64: length not a multiple of 4 (got " + std::to_string(n) + ")",
                n);
        }

        int pad = 0;
        if (input[n - 1] == '=') {
            pad = 1;
            if (n >= 2 && input[n - 2] == '=') {
                pad = 2;
                if (n >= 3 && input[n - 3] == '=') {
                    throw DecodeError(
                        "base64: invalid padding at position " + std::to_string(n - 3),
                        n - 3);
                }
            }
        }
        const std::size_t data_len = n - (std::size_t)pad;

        for (std::size_t i = 0; i < data_len; ++i) {
            if (input[i] == '=') {
                throw DecodeError(
                    "base64: invalid padding at position " + std::to_string(i),
                    i);
            }
        }

        const std::uint8_t* src = (const std::uint8_t*)input.data();
        std::size_t i = 0;
        if (data_len >= 32) {
            const __m256i vA_1 = _mm256_set1_epi8('A' - 1);
            const __m256i vZ_1 = _mm256_set1_epi8('Z' + 1);
            const __m256i va_1 = _mm256_set1_epi8('a' - 1);
            const __m256i vz_1 = _mm256_set1_epi8('z' + 1);
            const __m256i v0_1 = _mm256_set1_epi8('0' - 1);
            const __m256i v9_1 = _mm256_set1_epi8('9' + 1);
            const __m256i vPlus  = _mm256_set1_epi8(c62(mode_));
            const __m256i vSlash = _mm256_set1_epi8(c63(mode_));

            while (i + 32 <= data_len) {
                __m256i c = _mm256_loadu_si256((const __m256i*)(src + i));
                __m256i m_upper = _mm256_and_si256(
                    _mm256_cmpgt_epi8(c, vA_1), _mm256_cmpgt_epi8(vZ_1, c));
                __m256i m_lower = _mm256_and_si256(
                    _mm256_cmpgt_epi8(c, va_1), _mm256_cmpgt_epi8(vz_1, c));
                __m256i m_digit = _mm256_and_si256(
                    _mm256_cmpgt_epi8(c, v0_1), _mm256_cmpgt_epi8(v9_1, c));
                __m256i m_plus  = _mm256_cmpeq_epi8(c, vPlus);
                __m256i m_slash = _mm256_cmpeq_epi8(c, vSlash);
                __m256i valid = _mm256_or_si256(
                    _mm256_or_si256(m_upper, _mm256_or_si256(m_lower, m_digit)),
                    _mm256_or_si256(m_plus, m_slash));
                if (_mm256_movemask_epi8(valid) != -1) break;
                i += 32;
            }
        }

        const char cp = c62(mode_);
        const char cs = c63(mode_);
        for (; i < data_len; ++i) {
            const unsigned char ch = src[i];
            const bool ok =
                (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') ||
                ch == (unsigned char)cp || ch == (unsigned char)cs;
            if (!ok) {
                throw DecodeError(
                    "base64: invalid character '" + char_repr((char)ch)
                        + "' at position " + std::to_string(i),
                    i);
            }
        }
    }

    static constexpr std::string char_repr(char c) {
        unsigned char uc = (unsigned char)c;
        if (uc >= 0x20 && uc < 0x7f && uc != '\\') return std::string(1, c);
        constexpr const char* hex = "0123456789abcdef";
        std::string r = "\\x";
        r += hex[uc >> 4];
        r += hex[uc & 0xF];
        return r;
    }

    static constexpr const char* kStdChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static constexpr const char* kUrlChars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

    static constexpr char c62(Mode m) noexcept { return m == Mode::Standard ? '+' : '-'; }
    static constexpr char c63(Mode m) noexcept { return m == Mode::Standard ? '/' : '_'; }

    constexpr int decode_char(unsigned char c) const noexcept {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (mode_ == Mode::Standard) {
            if (c == '+') return 62;
            if (c == '/') return 63;
        } else {
            if (c == '-') return 62;
            if (c == '_') return 63;
        }
        return 0;
    }

    Mode mode_ = Mode::Standard;
};

} // namespace base64_avx2