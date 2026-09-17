#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>

TEST(EncodeVsRef, AllLengthsUpTo512) {
    std::mt19937 rng(999);
    for (std::size_t len = 0; len <= 512; ++len) {
        const std::string data = test_util::random_string(rng, len);
        const std::string e_avx2 = base64_avx2::encode(data);
        const std::string e_ref  = ref::encode(data);
        ASSERT_EQ(e_avx2, e_ref)
            << "len=" << len
            << "\n  avx2: " << test_util::hex(e_avx2, 64)
            << "\n  ref : " << test_util::hex(e_ref,  64);
    }
}