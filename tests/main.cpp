#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // тут можно добавить глобальную настройку:
    // ::testing::GTEST_FLAG(filter) = "BelderFixture.*";
    // ::testing::GTEST_FLAG(break_on_failure) = true;
    // ::testing::GTEST_FLAG(throw_on_failure) = true;

    return RUN_ALL_TESTS();
}