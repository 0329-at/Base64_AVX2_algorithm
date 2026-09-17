#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include <gtest/gtest.h>

namespace {
struct KnownVector { const char* in; const char* out; };

class KnownVectors : public ::testing::TestWithParam<KnownVector> {};

TEST_P(KnownVectors, EncodeMatchesRfc) {
    base64_avx2::Base64 b64;
    EXPECT_EQ(b64.encode(GetParam().in), GetParam().out);
}

TEST_P(KnownVectors, ValidateAcceptsRfc) {
    base64_avx2::Base64 b64;
    EXPECT_TRUE(b64.validate(GetParam().out));
}

TEST_P(KnownVectors, DecodeMatchesRfc) {
    base64_avx2::Base64 b64;
    EXPECT_EQ(b64.decode(GetParam().out), GetParam().in);
}

INSTANTIATE_TEST_SUITE_P(
    Rfc4648, KnownVectors,
    ::testing::Values(
        KnownVector{"",       ""        },
        KnownVector{"f",      "Zg=="    },
        KnownVector{"fo",     "Zm8="    },
        KnownVector{"foo",    "Zm9v"    },
        KnownVector{"foob",   "Zm9vYg=="},
        KnownVector{"fooba",  "Zm9vYmE="},
        KnownVector{"foobar", "Zm9vYmFy"}
    )
);
} // namespace