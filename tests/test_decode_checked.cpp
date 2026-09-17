#include "Base64_algorithm.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>

TEST(DecodeChecked, EquivalentToValidateAndDecode) {
    std::mt19937 rng(0x12345678);
    for (int iter = 0; iter < 500; ++iter) {
        std::uniform_int_distribution<int> len_d(0, 300);
        const int len = len_d(rng);
        const std::string data =
            test_util::random_string(rng, (std::size_t)len);
        const std::string enc = base64_avx2::encode(data);

        const std::string a = base64_avx2::decode_checked(enc);

        std::string b;
        if (base64_avx2::validate(enc)) b = base64_avx2::decode(enc);
        else b = "<<invalid>>";

        EXPECT_EQ(a, b) << "iter " << iter;
    }
}

TEST(Decode, AcceptsOutputOfEncodeForArbitraryData) {
    std::mt19937 rng(0);
    for (std::size_t len : {0u, 1u, 2u, 3u, 24u, 32u, 33u, 48u, 100u, 1024u}) {
        const std::string data = test_util::random_string(rng, len);
        const std::string enc = base64_avx2::encode(data);
        ASSERT_TRUE(base64_avx2::validate(enc));
        EXPECT_EQ(base64_avx2::decode(enc), data) << "len=" << len;
    }
}