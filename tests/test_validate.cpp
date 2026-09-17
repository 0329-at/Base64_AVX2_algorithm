#include "Base64_algorithm.hpp"
#include <gtest/gtest.h>

TEST(Validate, AcceptsValid) {
    EXPECT_TRUE(base64_avx2::validate(""));
    EXPECT_TRUE(base64_avx2::validate("Zm9v"));
    EXPECT_TRUE(base64_avx2::validate("Zg=="));
    EXPECT_TRUE(base64_avx2::validate("Zm8="));
    EXPECT_TRUE(base64_avx2::validate("+/+/"));
    EXPECT_TRUE(base64_avx2::validate("AA=="));
    EXPECT_TRUE(base64_avx2::validate("AAA="));
    EXPECT_TRUE(base64_avx2::validate("MTIzNDU2Nzg5MDEyMzQ1Njc4OTAxMjM0"));
}

TEST(Validate, RejectsBadLength) {
    EXPECT_FALSE(base64_avx2::validate("Z"));
    EXPECT_FALSE(base64_avx2::validate("Zg"));
    EXPECT_FALSE(base64_avx2::validate("Zg="));
    EXPECT_FALSE(base64_avx2::validate("Zm9vZ"));
}

TEST(Validate, RejectsBadChars) {
    EXPECT_FALSE(base64_avx2::validate("$AAA"));
    EXPECT_FALSE(base64_avx2::validate("A-AA"));
    EXPECT_FALSE(base64_avx2::validate("AA_A"));
    EXPECT_FALSE(base64_avx2::validate("AA A"));
    EXPECT_FALSE(base64_avx2::validate("AA\nA"));
    EXPECT_FALSE(base64_avx2::validate(std::string("AA\x7f" "A")));
    EXPECT_FALSE(base64_avx2::validate(std::string("AA\x80" "A")));
}

TEST(Validate, RejectsBadPadding) {
    EXPECT_FALSE(base64_avx2::validate("=AAA"));
    EXPECT_FALSE(base64_avx2::validate("A=AA"));
    EXPECT_FALSE(base64_avx2::validate("AA=A"));
    EXPECT_FALSE(base64_avx2::validate("A==="));
}

TEST(Validate, AcceptsLongInputs) {
    std::string s(64, 'A');
    EXPECT_TRUE(base64_avx2::validate(s));

    std::string s2(32, 'A');
    s2[30] = '=';
    s2[31] = '=';
    EXPECT_TRUE(base64_avx2::validate(s2));
}

TEST(Validate, RejectsBadCharInLongInput) {
    std::string s(64, 'A');
    s[40] = '$';
    EXPECT_FALSE(base64_avx2::validate(s));

    std::string t(32, 'A');
    t[31] = '-';
    EXPECT_FALSE(base64_avx2::validate(t));
}