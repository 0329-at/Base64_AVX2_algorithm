#include "Base64_algorithm.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>
#include <stdexcept>

using base64_avx2::Base64;
using base64_avx2::DecodeError;

TEST(ApiConsistency, ValidateAndDecodeAgree) {
    std::mt19937 rng(0x12345678);
    Base64 b64;

    // 随机合法输入
    for (int iter = 0; iter < 300; ++iter) {
        std::uniform_int_distribution<int> len_d(0, 300);
        const int len = len_d(rng);
        const std::string data = test_util::random_string(rng, (std::size_t)len);
        const std::string enc = b64.encode(data);

        ASSERT_TRUE(b64.validate(enc));
        EXPECT_NO_THROW((void)b64.decode(enc));
    }

    // 随机非法输入
    const char bad[] = {'$', '-', '_', ' ', '\n', '@', '#'};
    for (int iter = 0; iter < 300; ++iter) {
        std::uniform_int_distribution<int> len_d(1, 100);
        const int len = len_d(rng);
        std::string data = test_util::random_string(rng, (std::size_t)len);
        std::string enc = b64.encode(data);   // ← 去掉 const
        if (enc.empty()) continue;

        std::uniform_int_distribution<std::size_t> pos_d(0, enc.size() - 1);
        const std::size_t p = pos_d(rng);
        enc[p] = bad[rng() % (sizeof(bad))];  // ← 现在可以改了

        const bool valid = b64.validate(enc);
        if (!valid) {
            EXPECT_THROW((void)b64.decode(enc), DecodeError)
                << "iter " << iter << " pos " << p;
        }
    }
}

// decode 对 encode 的输出始终有效（各种长度）
TEST(ApiConsistency, DecodeAcceptsEncodeOutput) {
    std::mt19937 rng(0);
    Base64 b64;
    for (std::size_t len : {0u, 1u, 2u, 3u, 24u, 32u, 33u, 48u, 100u, 1024u}) {
        const std::string data = test_util::random_string(rng, len);
        const std::string enc = b64.encode(data);
        ASSERT_TRUE(b64.validate(enc)) << "len=" << len;
        EXPECT_EQ(b64.decode(enc), data) << "len=" << len;
    }
}

// 空输入：合法、不抛、返回空
TEST(ApiConsistency, EmptyInputIsValidAndSafe) {
    Base64 b64;
    EXPECT_TRUE(b64.validate(""));
    EXPECT_NO_THROW((void)b64.decode(""));
    EXPECT_EQ(b64.decode(""), "");
    EXPECT_EQ(b64.encode(""), "");
}

// DecodeError 是 std::invalid_argument 的子类
TEST(ApiConsistency, DecodeErrorIsInvalidArgument) {
    Base64 b64;
    try {
        (void)b64.decode("$AAA");
        FAIL() << "should throw";
    } catch (const std::invalid_argument& e) {
        // 用基类引用捕获，仍然能拿到 what()
        EXPECT_FALSE(std::string(e.what()).empty());
    }
}

// 抛出后对象仍然可用
TEST(ApiConsistency, ObjectUsableAfterException) {
    Base64 b64;

    EXPECT_THROW((void)b64.decode("$AAA"), DecodeError);

    // 同一个对象继续用
    const std::string enc = b64.encode("hello");
    EXPECT_EQ(b64.decode(enc), "hello");

    EXPECT_THROW((void)b64.decode("Zg="), DecodeError);
    EXPECT_EQ(b64.decode(b64.encode("world")), "world");
}

// 异常信息携带出错位置
TEST(ApiConsistency, ExceptionPositionIsConsistent) {
    Base64 b64;

    struct Case {
        const char* in;
        std::size_t pos;
    };
    const Case cases[] = {
        {"AAAA$AAA", 4},
        {"A$AA",     1},
        {"AAA$",     3},
    };
    for (const auto& c : cases) {
        try {
            (void)b64.decode(c.in);
            FAIL() << "should throw for " << c.in;
        } catch (const DecodeError& e) {
            EXPECT_EQ(e.position(), c.pos) << "input: " << c.in;
        }
    }
}