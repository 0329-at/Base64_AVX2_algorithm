#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>

using base64_avx2::Base64;

TEST(EncodeVsRef, StandardAllLengthsUpTo512) {
    std::mt19937 rng(999);
    Base64 b64;
    for (std::size_t len = 0; len <= 512; ++len) {
        const std::string data = test_util::random_string(rng, len);
        const std::string e_avx = b64.encode(data);
        const std::string e_ref = ref::encode(data, ref::Alphabet::Standard);
        ASSERT_EQ(e_avx, e_ref)
            << "len=" << len
            << "\n  avx: " << test_util::hex(e_avx, 64)
            << "\n  ref: " << test_util::hex(e_ref, 64);
    }
}