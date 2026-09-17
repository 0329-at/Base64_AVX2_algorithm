#include "Base64_algorithm.hpp"
#include "test_utils.hpp"
#include <gtest/gtest.h>
#include <random>

TEST(Mutation, RandomSingleCharReplacements) {
    std::mt19937 rng(0xCAFEBABE);
    const char bad[] = {'$', '-', '_', ' ', '\n', '@', '#', '^', '&', '*'};

    int checked = 0;
    for (int iter = 0; iter < 500; ++iter) {
        std::uniform_int_distribution<int> len_d(1, 100);
        const int len = len_d(rng);
        const std::string data =
            test_util::random_string(rng, (std::size_t)len);

        std::string enc = base64_avx2::encode(data);
        if (enc.empty()) continue;

        std::uniform_int_distribution<std::size_t> pos_d(0, enc.size() - 1);
        const std::size_t p = pos_d(rng);
        const char orig = enc[p];
        const char repl = bad[rng() % (sizeof(bad))];
        if (repl == orig) continue;
        enc[p] = repl;

        EXPECT_FALSE(base64_avx2::validate(enc))
            << "iter " << iter << " pos " << p << " char " << repl;
        ++checked;
    }
    EXPECT_GT(checked, 0);
}