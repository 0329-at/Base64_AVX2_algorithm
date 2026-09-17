#include "Base64_algorithm.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>

TEST(Roundtrip, AllLengthsUpTo1024) {
    std::mt19937 rng(12345);
    for (std::size_t len = 0; len <= 1024; ++len) {
        const std::string data = test_util::random_string(rng, len);
        const std::string enc = base64_avx2::encode(data);

        ASSERT_TRUE(base64_avx2::validate(enc))
            << "encode produced invalid base64 at len " << len;

        EXPECT_EQ(base64_avx2::decode_checked(enc), data)
            << "roundtrip mismatch at len " << len;

        EXPECT_EQ(base64_avx2::decode(enc), base64_avx2::decode_checked(enc))
            << "decode vs decode_checked at len " << len;
    }
}