#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>

using base64_avx2::Base64;

TEST(AllByteValues, FullRange) {
    Base64 b64;
    std::string data(256, '\0');
    for (int i = 0; i < 256; ++i) data[i] = (char)i;

    const std::string enc = b64.encode(data);
    EXPECT_EQ(enc, ref::encode(data));
    ASSERT_TRUE(b64.validate(enc));
    EXPECT_EQ(b64.decode(enc), data);
}

TEST(AllByteValues, EachSingleByte) {
    Base64 b64;
    for (int i = 0; i < 256; ++i) {
        std::string s(1, (char)i);
        const std::string e = b64.encode(s);
        ASSERT_TRUE(b64.validate(e)) << "byte " << i;
        EXPECT_EQ(b64.decode(e), s) << "byte " << i;
    }
}