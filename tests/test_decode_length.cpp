#include "Base64_algorithm.hpp"
#include <gtest/gtest.h>

using base64_avx2::Base64;

namespace {
struct LengthCase { const char* in; std::size_t expect; };

class DecodeLength : public ::testing::TestWithParam<LengthCase> {};

TEST_P(DecodeLength, OutputSizeMatches) {
    Base64 b64;
    ASSERT_TRUE(b64.validate(GetParam().in));
    EXPECT_EQ(b64.decode(GetParam().in).size(), GetParam().expect);
}

INSTANTIATE_TEST_SUITE_P(
    Various, DecodeLength,
    ::testing::Values(
        LengthCase{"Zg==",     1},
        LengthCase{"Zm8=",     2},
        LengthCase{"Zm9v",     3},
        LengthCase{"Zm9vYg==", 4},
        LengthCase{"Zm9vYmE=", 5},
        LengthCase{"Zm9vYmFy", 6},
        LengthCase{"MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0", 24},
        LengthCase{"MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0NTY3ODkwMTIzNDU2Nzg5MDEyMzQ1Njc4", 48}
    )
);
} // namespace