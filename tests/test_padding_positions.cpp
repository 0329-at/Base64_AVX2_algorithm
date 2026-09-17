#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>

using base64_avx2::Base64;

namespace {
class PaddingPosition : public ::testing::TestWithParam<std::size_t> {};

TEST_P(PaddingPosition, MatchesRefAndRoundtrips) {
    const std::size_t len = GetParam();
    std::string data(len, '\0');
    for (std::size_t i = 0; i < len; ++i) data[i] = (char)(i * 7 + 1);

    Base64 b64;
    const std::string enc = b64.encode(data);
    ASSERT_EQ(enc, ref::encode(data)) << "len=" << len;
    ASSERT_TRUE(b64.validate(enc)) << "len=" << len;
    EXPECT_EQ(b64.decode(enc), data) << "len=" << len;
}

INSTANTIATE_TEST_SUITE_P(
    SimdBoundaries, PaddingPosition,
    ::testing::Values(
        1u, 2u, 3u, 4u, 5u,
        22u, 23u, 24u, 25u,
        43u, 44u, 45u, 46u, 47u,
        64u, 65u, 66u, 67u,
        88u, 89u, 90u, 91u,
        120u, 121u, 122u, 123u
    )
);
} // namespace