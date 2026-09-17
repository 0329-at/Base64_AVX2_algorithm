#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>

namespace {
struct KnownVector {
    const char* in;
    const char* out;
};

class KnownVectors : public ::testing::TestWithParam<KnownVector> {};

TEST_P(KnownVectors, EncodeMatchesRfc) {
    const auto& v = GetParam();
    EXPECT_EQ(base64_avx2::encode(v.in), v.out);
}

TEST_P(KnownVectors, DecodeMatchesRfc) {
    const auto& v = GetParam();
    EXPECT_EQ(base64_avx2::decode_checked(v.out), v.in);
}

TEST_P(KnownVectors, ValidateAcceptsRfcVector) {
    const auto& v = GetParam();
    EXPECT_TRUE(base64_avx2::validate(v.out));
}

INSTANTIATE_TEST_SUITE_P(
    Rfc4648, KnownVectors,
    ::testing::Values(
        KnownVector{"",       ""       },
        KnownVector{"f",      "Zg=="   },
        KnownVector{"fo",     "Zm8="   },
        KnownVector{"foo",    "Zm9v"   },
        KnownVector{"foob",   "Zm9vYg==" },
        KnownVector{"fooba",  "Zm9vYmE=" },
        KnownVector{"foobar", "Zm9vYmFy" }
    )
);
} // namespace