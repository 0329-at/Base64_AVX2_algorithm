#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>

using base64_avx2::Base64;

TEST(PlusSlash, ShortSequence) {
    Base64 b64;
    std::string s;
    s += (char)0xFB; s += (char)0xFF; s += (char)0xFF;
    EXPECT_EQ(b64.encode(s), "+///");
    EXPECT_TRUE(b64.validate("+///"));
    EXPECT_EQ(b64.decode("+///"), s);
}

TEST(PlusSlash, LongSequenceThroughSimd) {
    Base64 b64;
    std::string big;
    for (int i = 0; i < 30; ++i) {
        big += (char)0xFB; big += (char)0xFF; big += (char)0xFF;
    }
    const std::string be = b64.encode(big);
    EXPECT_EQ(be, ref::encode(big));
    EXPECT_TRUE(b64.validate(be));
    EXPECT_EQ(b64.decode(be), big);
}