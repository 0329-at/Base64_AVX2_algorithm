#include "Base64_algorithm.hpp"
#include <gtest/gtest.h>
#include <stdexcept>

TEST(InvalidChars, DecodeCheckedThrows) {
    EXPECT_THROW((void)base64_avx2::decode_checked("$AAA"),
                 std::invalid_argument);
    EXPECT_THROW((void)base64_avx2::decode_checked("Zg="),
                 std::invalid_argument);
    EXPECT_NO_THROW((void)base64_avx2::decode_checked(""));
}

TEST(InvalidChars, InSimdRange) {
    std::string big(64, 'A');
    big[20] = '$';
    EXPECT_FALSE(base64_avx2::validate(big));
    EXPECT_THROW((void)base64_avx2::decode_checked(big),
                 std::invalid_argument);

    std::string big2(64, 'A');
    big2[40] = '#';
    EXPECT_FALSE(base64_avx2::validate(big2));

    std::string tail(40, 'A');
    tail[35] = '@';
    EXPECT_FALSE(base64_avx2::validate(tail));
}