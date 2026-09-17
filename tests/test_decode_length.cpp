#include "Base64_algorithm.hpp"
#include <gtest/gtest.h>

namespace {
struct LengthCase {
    const char* in;
    std::size_t expect;
};

class DecodeLength : public ::testing::TestWithParam<LengthCase> {};

TEST_P(DecodeLength, OutputSizeMatches) {
    const auto& c = GetParam();
    ASSERT_TRUE(base64_avx2::validate(c.in));
    EXPECT_EQ(base64_avx2::decode(c.in).size(), c.expect);
}

INSTANTIATE_TEST_SUITE_P(
    Various, DecodeLength,
    ::testing::Values(
        LengthCase{"Zg==", 1},
        LengthCase{"Zm8=", 2},
        LengthCase{"Zm9v", 3},
        LengthCase{"Zm9vYg==", 4},
        LengthCase{"Zm9vYmE=", 5},
        LengthCase{"Zm9vYmFy", 6},
        LengthCase{"MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0", 24},
        LengthCase{"MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4", 48}
    )
);
} // namespace