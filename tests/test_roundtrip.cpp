#include "Base64_algorithm.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>

using base64_avx2::Base64;

TEST(Roundtrip, AllLengthsUpTo1024) {
    std::mt19937 rng(12345);
    Base64 b64;
    for (std::size_t len = 0; len <= 1024; ++len) {
        const std::string data = test_util::random_string(rng, len);
        const std::string enc = b64.encode(data);

        ASSERT_TRUE(b64.validate(enc)) << "len=" << len;
        EXPECT_EQ(b64.decode(enc), data) << "len=" << len;
    }
}