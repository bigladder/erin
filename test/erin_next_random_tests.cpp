#include "erin_next/erin_next_random.h"
#include <gtest/gtest.h>

std::streamsize const width = 30;

TEST(ErinRandom, Fixed)
{
    erin::FixedRandom r{};
    r.FixedValue = 0.3;
    EXPECT_EQ(r(), 0.3);
    EXPECT_EQ(r(), 0.3);
    EXPECT_EQ(r(), 0.3);
}

TEST(ErinRandom, Series)
{
    erin::FixedSeries r{};
    EXPECT_EQ(r(), 0.0);
    r.Series.push_back(0.1);
    r.Series.push_back(0.2);
    r.Series.push_back(0.3);
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
    erin::Random r = erin::CreateRandomWithSeed(17);
    for (size_t i = 0; i < 1'000; ++i)
    {
        EXPECT_TRUE((r() >= 0.0) && (r() <= 1.0));
    }
}

TEST(ErinRandom, FromClock)
{
    erin::Random r = erin::CreateRandom();
    for (size_t i = 0; i < 1'000; ++i)
    {
        EXPECT_TRUE((r() >= 0.0) && (r() <= 1.0));
    }
}
