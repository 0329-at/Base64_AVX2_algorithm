#include "Base64_algorithm.hpp"
#include <gtest/gtest.h>

using base64_avx2::Base64;

TEST(Validate, AcceptsValid) {
    Base64 b64;
    EXPECT_TRUE(b64.validate(""));
    EXPECT_TRUE(b64.validate("Zm9v"));
    EXPECT_TRUE(b64.validate("Zg=="));
    EXPECT_TRUE(b64.validate("Zm8="));
    EXPECT_TRUE(b64.validate("+/+/"));
    EXPECT_TRUE(b64.validate("AA=="));
    EXPECT_TRUE(b64.validate("AAA="));
    EXPECT_TRUE(b64.validate("MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0"));
}

TEST(Validate, RejectsBadLength) {
    Base64 b64;
    EXPECT_FALSE(b64.validate("Z"));
    EXPECT_FALSE(b64.validate("Zg"));
    EXPECT_FALSE(b64.validate("Zg="));
    EXPECT_FALSE(b64.validate("Zm9vZ"));
}

TEST(Validate, RejectsBadChars) {
    Base64 b64;
    EXPECT_FALSE(b64.validate("$AAA"));
    EXPECT_FALSE(b64.validate("A-AA"));
    EXPECT_FALSE(b64.validate("AA_A"));
    EXPECT_FALSE(b64.validate("AA A"));
    EXPECT_FALSE(b64.validate("AA\nA"));
    EXPECT_FALSE(b64.validate(std::string("AA\x7f" "A")));
    EXPECT_FALSE(b64.validate(std::string("AA\x80" "A")));
}

TEST(Validate, RejectsBadPadding) {
    Base64 b64;
    EXPECT_FALSE(b64.validate("=AAA"));
    EXPECT_FALSE(b64.validate("A=AA"));
    EXPECT_FALSE(b64.validate("AA=A"));
    EXPECT_FALSE(b64.validate("A==="));
}

TEST(Validate, AcceptsLongInputs) {
    Base64 b64;
    EXPECT_TRUE(b64.validate(std::string(64, 'A')));
    std::string s(32, 'A');
    s[30] = '='; s[31] = '=';
    EXPECT_TRUE(b64.validate(s));
}

TEST(Validate, RejectsBadCharInLongInput) {
    Base64 b64;
    std::string s(64, 'A');
    s[40] = '$';
    EXPECT_FALSE(b64.validate(s));

    std::string t(32, 'A');
    t[31] = '-';
    EXPECT_FALSE(b64.validate(t));
}