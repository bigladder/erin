// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <gtest/gtest.h>

#include "erin/random.h"

TEST(ErinRandom, Fixed)
{
    erin::FixedRandom r {};
    r.fixed_value = 0.3;
    EXPECT_EQ(r(), 0.3);
    EXPECT_EQ(r(), 0.3);
    EXPECT_EQ(r(), 0.3);
}

TEST(ErinRandom, Series)
{
    erin::FixedSeries r {};
    EXPECT_EQ(r(), 0.0);
    r.series.push_back(0.1);
    r.series.push_back(0.2);
    r.series.push_back(0.3);
    EXPECT_EQ(r(), 0.1);
    EXPECT_EQ(r(), 0.2);
    EXPECT_EQ(r(), 0.3);
    EXPECT_EQ(r(), 0.1);
    EXPECT_EQ(r(), 0.2);
    EXPECT_EQ(r(), 0.3);
    EXPECT_EQ(r(), 0.1);
}

TEST(ErinRandom, WithSeed)
{
    erin::Random r = erin::create_random_with_seed(17);
    for (size_t i = 0; i < 1'000; ++i)
    {
        EXPECT_TRUE((r() >= 0.0) && (r() <= 1.0));
    }
}

TEST(ErinRandom, FromClock)
{
    erin::Random r = erin::create_random();
    for (size_t i = 0; i < 1'000; ++i)
    {
        EXPECT_TRUE((r() >= 0.0) && (r() <= 1.0));
    }
}
