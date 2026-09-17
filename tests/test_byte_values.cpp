#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>

TEST(AllByteValues, FullRange) {
    std::string data(256, '\0');
    for (int i = 0; i < 256; ++i) data[i] = (char)i;

    const std::string enc = base64_avx2::encode(data);
    EXPECT_EQ(enc, ref::encode(data));
    EXPECT_TRUE(base64_avx2::validate(enc));
    EXPECT_EQ(base64_avx2::decode(enc), data);
}

TEST(AllByteValues, EachSingleByte) {
    for (int i = 0; i < 256; ++i) {
        std::string s(1, (char)i);
        const std::string e = base64_avx2::encode(s);
        EXPECT_TRUE(base64_avx2::validate(e)) << "byte " << i;
        EXPECT_EQ(base64_avx2::decode(e), s) << "byte " << i;
    }
}