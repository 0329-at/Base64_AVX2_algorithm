#include "Base64_algorithm.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

using base64_avx2::Base64;
using base64_avx2::DecodeError;

TEST(InvalidChars, DecodeThrowsOnBadChar) {
    Base64 b64;
    EXPECT_THROW(b64.decode("$AAA"), DecodeError);
    EXPECT_THROW(b64.decode("A-AA"), DecodeError);
    EXPECT_THROW(b64.decode("AA_A"), DecodeError);
}

TEST(InvalidChars, DecodeThrowsOnBadLength) {
    Base64 b64;
    EXPECT_THROW(b64.decode("Z"),   DecodeError);
    EXPECT_THROW(b64.decode("Zg"),  DecodeError);
    EXPECT_THROW(b64.decode("Zg="), DecodeError);
}

TEST(InvalidChars, DecodeThrowsOnBadPadding) {
    Base64 b64;
    EXPECT_THROW(b64.decode("=AAA"), DecodeError);
    EXPECT_THROW(b64.decode("A=AA"), DecodeError);
    EXPECT_THROW(b64.decode("AA=A"), DecodeError);
    EXPECT_THROW(b64.decode("A==="), DecodeError);
}

TEST(InvalidChars, DecodeThrowsInSimdRange) {
    Base64 b64;
    std::string s1(64, 'A');
    s1[20] = '$';
    EXPECT_THROW(b64.decode(s1), DecodeError);
    EXPECT_FALSE(b64.validate(s1));

    std::string s2(64, 'A');
    s2[40] = '#';
    EXPECT_THROW(b64.decode(s2), DecodeError);

    std::string s3(40, 'A');
    s3[35] = '@';
    EXPECT_THROW(b64.decode(s3), DecodeError);
}

TEST(InvalidChars, ExceptionCarriesPosition) {
    Base64 b64;
    try {
        (void)b64.decode("AAAA$AAA");
        FAIL() << "should throw";
    } catch (const DecodeError& e) {
        EXPECT_EQ(e.position(), 4u);
        EXPECT_NE(std::string(e.what()).find("'$'"), std::string::npos);
        EXPECT_NE(std::string(e.what()).find("position 4"), std::string::npos);
    }
}

TEST(InvalidChars, ExceptionCarriesBadLength) {
    Base64 b64;
    try {
        (void)b64.decode("Zg=");
        FAIL() << "should throw";
    } catch (const DecodeError& e) {
        EXPECT_NE(std::string(e.what()).find("multiple of 4"), std::string::npos);
    }
}

TEST(InvalidChars, ExceptionCarriesBadPadding) {
    Base64 b64;
    try {
        (void)b64.decode("A=AA");
        FAIL() << "should throw";
    } catch (const DecodeError& e) {
        EXPECT_EQ(e.position(), 1u);
        EXPECT_NE(std::string(e.what()).find("padding"), std::string::npos);
    }
}

TEST(InvalidChars, ExceptionIsInvalidArgument) {
    Base64 b64;
    // DecodeError 继承 std::invalid_argument，旧代码 catch 依然有效
    EXPECT_THROW(b64.decode("$AAA"), std::invalid_argument);
}

TEST(InvalidChars, EmptyInputDoesNotThrow) {
    Base64 b64;
    EXPECT_NO_THROW((void)b64.decode(""));
    EXPECT_TRUE(b64.validate(""));
}