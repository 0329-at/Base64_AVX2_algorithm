#pragma once
#include <immintrin.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>

#if defined(__GNUC__) || defined(__clang__)
    #define B64_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define B64_FORCE_INLINE __forceinline
#else
    #define B64_FORCE_INLINE inline
#endif

namespace base64_avx2 {

namespace detail {

constexpr std::array<char, 64> enc_table = [] {
    std::array<char, 64> t{};
    constexpr const char* s =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    for (int i = 0; i < 64; ++i) t[i] = s[i];
    return t;
}();

B64_FORCE_INLINE int decode_char(unsigned char c) noexcept {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 0;   
}

} // namespace detail

constexpr inline bool validate(std::string_view input) noexcept {
    const std::size_t n = input.size();

    if (n == 0) {
        return true;             
    }

    if ((n & 3) != 0) {
        return false;
    }

    std::size_t data_len = n;
    if (input[n - 1] == '=') {
        --data_len;
        if (input[n - 2] == '=') {
            --data_len;
            if (n >= 3 && input[n - 3] == '=') return false;
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
        const __m256i vPlus  = _mm256_set1_epi8('+');
        const __m256i vSlash = _mm256_set1_epi8('/');

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

    for (; i < data_len; ++i) {
        const unsigned char c = src[i];
        const bool ok =
            (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '+' || c == '/';
        if (!ok) return false;
    }

    return true;
}

constexpr inline std::string encode(std::string_view input) {
    const std::size_t n = input.size();
    if (n == 0) return {};

    std::string out((n + 2) / 3 * 4, '\0');
    const std::uint8_t* __restrict src = (const std::uint8_t*)input.data();
    std::uint8_t* __restrict dst       = (std::uint8_t*)out.data();

    const __m256i m24  = _mm256_set_epi32(0, 0, -1, -1, -1, -1, -1, -1);
    const __m256i perm = _mm256_set_epi32(6, 5, 4, 3, 3, 2, 1, 0);
    const __m256i shuf = _mm256_setr_epi8(
        2,1,0,(char)0x80, 5,4,3,(char)0x80,
        8,7,6,(char)0x80, 11,10,9,(char)0x80,
        2,1,0,(char)0x80, 5,4,3,(char)0x80,
        8,7,6,(char)0x80, 11,10,9,(char)0x80);
    const __m256i vA  = _mm256_set1_epi8('A');
    const __m256i v25 = _mm256_set1_epi8(25);
    const __m256i v51 = _mm256_set1_epi8(51);
    const __m256i v62 = _mm256_set1_epi8(62);
    const __m256i v63 = _mm256_set1_epi8(63);
    const __m256i v6  = _mm256_set1_epi8(6);
    const __m256i v75 = _mm256_set1_epi8(75);
    const __m256i v15 = _mm256_set1_epi8(15);
    const __m256i v12 = _mm256_set1_epi8(12);
    const __m256i v3F = _mm256_set1_epi32(0x3F);

    auto enc_block = [&](const std::uint8_t* s, std::uint8_t* d) {
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

        __m256i ch  = _mm256_add_epi8(r, vA);
        __m256i m26 = _mm256_cmpgt_epi8(r, v25);
        ch = _mm256_add_epi8(ch, _mm256_and_si256(m26, v6));
        __m256i m52 = _mm256_cmpgt_epi8(r, v51);
        ch = _mm256_sub_epi8(ch, _mm256_and_si256(m52, v75));
        __m256i m62 = _mm256_cmpeq_epi8(r, v62);
        ch = _mm256_sub_epi8(ch, _mm256_and_si256(m62, v15));
        __m256i m63 = _mm256_cmpeq_epi8(r, v63);
        ch = _mm256_sub_epi8(ch, _mm256_and_si256(m63, v12));

        _mm256_storeu_si256((__m256i*)d, ch);
    };

    std::size_t pos = 0, dpos = 0;

    if (n >= 24) {
        while (pos + 48 <= n) {
            _mm_prefetch((const char*)(src + pos + 512), _MM_HINT_T0);
            enc_block(src + pos,      dst + dpos);
            enc_block(src + pos + 24, dst + dpos + 32);
            pos += 48; dpos += 64;
        }
        while (pos + 24 <= n) {
            enc_block(src + pos, dst + dpos);
            pos += 24; dpos += 32;
        }

        const std::size_t rem = n - pos;
        if (rem > 0) {
            alignas(32) std::uint8_t tmp[24] = {};
            std::memcpy(tmp, src + pos, rem);
            alignas(32) std::uint8_t tmp_out[32];
            enc_block(tmp, tmp_out);
            const std::size_t out_len = (rem + 2) / 3 * 4;
            if (rem % 3 == 1) {
                tmp_out[out_len - 2] = '=';
                tmp_out[out_len - 1] = '=';
            } else if (rem % 3 == 2) {
                tmp_out[out_len - 1] = '=';
            }
            std::memcpy(dst + dpos, tmp_out, out_len);
        }
    } else {
        while (pos < n) {
            const std::size_t rem = n - pos;
            std::uint32_t t = 0;
            if (rem >= 1) t |= (std::uint32_t)src[pos]     << 16;
            if (rem >= 2) t |= (std::uint32_t)src[pos + 1] <<  8;
            if (rem >= 3) t |= (std::uint32_t)src[pos + 2];

            dst[dpos + 0] = detail::enc_table[(t >> 18) & 0x3F];
            dst[dpos + 1] = detail::enc_table[(t >> 12) & 0x3F];
            dst[dpos + 2] = (rem >= 2) ? detail::enc_table[(t >> 6) & 0x3F] : '=';
            dst[dpos + 3] = (rem >= 3) ? detail::enc_table[t & 0x3F]        : '=';
            dpos += 4;
            pos  += (rem >= 3) ? 3 : rem;
        }
    }
    return out;
}

constexpr inline std::string decode(std::string_view input) {
    const std::size_t n = input.size();
    if (n == 0) {
        return {};
    }

    int pad = 0;
    if (input[n - 1] == '=') {
        ++pad;
        if (n >= 2 && input[n - 2] == '=') ++pad;
    }
    const std::size_t out_len = n / 4 * 3 - (std::size_t)pad;

    std::string out(out_len + 8, '\0');
    const std::uint8_t* __restrict src = (const std::uint8_t*)input.data();
    std::uint8_t* __restrict dst       = (std::uint8_t*)out.data();

    const __m256i vA   = _mm256_set1_epi8('A');
    const __m256i va_1 = _mm256_set1_epi8('a' - 1);
    const __m256i vz_1 = _mm256_set1_epi8('z' + 1);
    const __m256i v0_1 = _mm256_set1_epi8('0' - 1);
    const __m256i v9_1 = _mm256_set1_epi8('9' + 1);
    const __m256i vPlus  = _mm256_set1_epi8('+');
    const __m256i vSlash = _mm256_set1_epi8('/');
    const __m256i v6  = _mm256_set1_epi8(6);
    const __m256i v69 = _mm256_set1_epi8(69);
    const __m256i v84 = _mm256_set1_epi8(84);
    const __m256i v81 = _mm256_set1_epi8(81);
    const __m256i v3F = _mm256_set1_epi32(0x3F);
    const __m256i shuf = _mm256_setr_epi8(
        2,1,0, 6,5,4, 10,9,8, 14,13,12, 0,0,0,0,
        2,1,0, 6,5,4, 10,9,8, 14,13,12, 0,0,0,0);

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
            _mm256_cmpeq_epi8(c, vPlus), v84));
        v = _mm256_add_epi8(v, _mm256_and_si256(
            _mm256_cmpeq_epi8(c, vSlash), v81));

        __m256i c0 = _mm256_and_si256(v, v3F);
        __m256i c1 = _mm256_and_si256(_mm256_srli_epi32(v,  8), v3F);
        __m256i c2 = _mm256_and_si256(_mm256_srli_epi32(v, 16), v3F);
        __m256i c3 = _mm256_and_si256(_mm256_srli_epi32(v, 24), v3F);
        __m256i m  = _mm256_or_si256(
            _mm256_or_si256(_mm256_slli_epi32(c0, 18), _mm256_slli_epi32(c1, 12)),
            _mm256_or_si256(_mm256_slli_epi32(c2,  6), c3));
        __m256i packed = _mm256_shuffle_epi8(m, shuf);

        _mm_storeu_si128((__m128i*)(dst + dpos),
                         _mm256_castsi256_si128(packed));
        _mm_storeu_si128((__m128i*)(dst + dpos + 12),
                         _mm256_extracti128_si256(packed, 1));

        pos  += 32;
        dpos += 24;
    }

    while (pos < n) {
        std::uint32_t quad = 0;
        int valid = 0;
        for (int j = 0; j < 4; ++j) {
            const unsigned char ch = src[pos + j];
            if (ch == '=') continue;
            quad = (quad << 6) | (std::uint32_t)detail::decode_char(ch);
            ++valid;
        }
        quad <<= (4 - valid) * 6;
        if (valid >= 2) dst[dpos++] = (char)((quad >> 16) & 0xFF);
        if (valid >= 3) dst[dpos++] = (char)((quad >>  8) & 0xFF);
        if (valid >= 4) dst[dpos++] = (char)( quad        & 0xFF);
        pos += 4;
    }

    out.resize(out_len);
    return out;
}

constexpr inline std::string decode_checked(std::string_view input) {
    if (!validate(input))
        throw std::invalid_argument("base64: invalid input");
    return decode(input);
}

} // namespace base64_avx2