#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>
#include <random>

using base64_avx2::Base64;

TEST(CrossValidate, TwoThousandRandom) {
    std::mt19937 rng(0xDEADBEEF);
    std::uniform_int_distribution<int> len_dist(0, 400);
    std::uniform_int_distribution<int> byte_dist(0, 255);
    Base64 b64;

    for (int iter = 0; iter < 2000; ++iter) {
        const int len = len_dist(rng);
        std::string data(len, '\0');
        for (auto& b : data) b = (char)byte_dist(rng);

        const std::string enc_avx = b64.encode(data);
        const std::string enc_ref = ref::encode(data);
        ASSERT_EQ(enc_avx, enc_ref) << "iter " << iter << " len " << len;

        ASSERT_TRUE(b64.validate(enc_ref)) << "iter " << iter;

        std::string dec_ref;
        ASSERT_TRUE(ref::decode(enc_ref, dec_ref)) << "iter " << iter;
        ASSERT_EQ(dec_ref, data) << "iter " << iter;

        ASSERT_EQ(b64.decode(enc_ref), dec_ref) << "iter " << iter;
    }
}