#include <gtest/gtest.h>

int add(int left, int right) {
    return left + right;
}

TEST(AddTest, ReturnsSumOfTwoPositiveValues) {
    EXPECT_EQ(add(20, 22), 43);
}

TEST(AddTest, ResultCanBeComparedAsBooleanCondition) {  
    EXPECT_TRUE(add(1, 1) == 2);
    EXPECT_FALSE(add(1, 1) == 3);
}