#include "erin_next/erin_next_lookup_table.h"
#include <gtest/gtest.h>
#include <vector>

TEST(LookupTable, TestInterp)
{
    std::vector<double> xs{0.0, 1.0};
    std::vector<double> ys{0.0, 1.0};
    double x = 0.5;
    double expected = 0.5;
    double actual = erin::LookupTable_LookupInterp(xs, ys, x);
    EXPECT_NEAR(expected, actual, 1e-6);
}

TEST(LookupTable, TestInterp2)
{
    std::vector<double> xs{0.0, 100.0};
    std::vector<double> ys{0.0, 10.0};
    double x = 50.0;
    double expected = 5.0;
    double actual = erin::LookupTable_LookupInterp(xs, ys, x);
    EXPECT_NEAR(expected, actual, 1e-6);
}

TEST(LookupTable, TestLookupStairstep)
{
    std::vector<double> xs{0.0, 50.0, 100.0};
    std::vector<double> ys{0.0, 5.0, 10.0};
    double x = 75.0;
    double expected = 5.0;
    double actual = erin::LookupTable_LookupStairStep(xs, ys, x);
    EXPECT_NEAR(expected, actual, 1e-6);
}
