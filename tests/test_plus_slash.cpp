#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>

TEST(PlusSlash, ShortSequence) {
    std::string s;
    s += (char)0xFB; s += (char)0xFF; s += (char)0xFF;
    EXPECT_EQ(base64_avx2::encode(s), "+///");
    EXPECT_TRUE(base64_avx2::validate("+///"));
    EXPECT_EQ(base64_avx2::decode("+///"), s);
}

TEST(PlusSlash, LongSequenceThroughSimd) {
    std::string big;
    for (int i = 0; i < 30; ++i) {
        big += (char)0xFB; big += (char)0xFF; big += (char)0xFF;
    }
    const std::string be = base64_avx2::encode(big);
    EXPECT_EQ(be, ref::encode(big));
    EXPECT_TRUE(base64_avx2::validate(be));
    EXPECT_EQ(base64_avx2::decode(be), big);
}