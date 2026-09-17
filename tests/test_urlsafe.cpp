#include "Base64_algorithm.hpp"
#include "ref_base64.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>

using base64_avx2::Base64;
using base64_avx2::Mode;

TEST(UrlSafe, EncodeDoesNotContainPlusSlash) {
    std::mt19937 rng(42);
    Base64 b64{Mode::UrlSafe};
    for (std::size_t len = 0; len <= 300; ++len) {
        std::string data = test_util::random_string(rng, len);
        std::string enc = b64.encode(data);
        EXPECT_EQ(enc.find('+'), std::string::npos) << "len=" << len;
        EXPECT_EQ(enc.find('/'), std::string::npos) << "len=" << len;
    }
}

TEST(UrlSafe, MatchesRefImplementation) {
    std::mt19937 rng(7);
    Base64 b64{Mode::UrlSafe};
    for (std::size_t len = 0; len <= 500; ++len) {
        std::string data = test_util::random_string(rng, len);
        EXPECT_EQ(b64.encode(data),
                  ref::encode(data, ref::Alphabet::UrlSafe)) << "len=" << len;
    }
}

TEST(UrlSafe, RoundTrip) {
    std::mt19937 rng(11);
    Base64 b64{Mode::UrlSafe};
    for (std::size_t len = 0; len <= 400; ++len) {
        std::string data = test_util::random_string(rng, len);
        std::string enc = b64.encode(data);
        ASSERT_TRUE(b64.validate(enc)) << "len=" << len;
        EXPECT_EQ(b64.decode(enc), data) << "len=" << len;
    }
}

TEST(UrlSafe, ValidateRejectsStandardChars) {
    Base64 url{Mode::UrlSafe};
    // URL-safe 里 '+' '/' 不是合法字符
    EXPECT_FALSE(url.validate("+/+/"));
    EXPECT_FALSE(url.validate("ab+/"));
    // 但 '-' '_' 是合法的
    EXPECT_TRUE(url.validate("-_--"));
    EXPECT_TRUE(url.validate("ab-_"));
}

TEST(UrlSafe, StandardModeRejectsUrlSafeChars) {
    Base64 std{Mode::Standard};
    EXPECT_FALSE(std.validate("-_--"));
    EXPECT_FALSE(std.validate("ab-_"));
    EXPECT_TRUE(std.validate("+/+/"));
    EXPECT_TRUE(std.validate("ab+/"));
}

TEST(UrlSafe, DecodeThrowsOnWrongAlphabet) {
    Base64 url{Mode::UrlSafe};
    EXPECT_THROW((void)url.decode("+/+/"), base64_avx2::DecodeError);

    Base64 std{Mode::Standard};
    EXPECT_THROW((void)std.decode("-_--"), base64_avx2::DecodeError);
}

TEST(UrlSafe, SetSwitchesMode) {
    Base64 b64;
    EXPECT_EQ(b64.mode(), Mode::Standard);

    b64.set(Mode::UrlSafe);
    EXPECT_EQ(b64.mode(), Mode::UrlSafe);

    std::string enc = b64.encode("\xFB\xEF\xBE");
    EXPECT_EQ(enc.find('+'), std::string::npos);
    EXPECT_EQ(enc.find('/'), std::string::npos);

    b64.set(Mode::Standard);
    EXPECT_EQ(b64.mode(), Mode::Standard);
}