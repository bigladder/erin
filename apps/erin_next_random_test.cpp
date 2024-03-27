#include "erin_next/erin_next_random.h"
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <assert.h>

std::streamsize const width = 30;

void
TestFixedRandom(void)
{
    std::cout << std::left << "[" << std::setw(width) << "TestFixedRandom";
    erin::FixedRandom r{};
    r.FixedValue = 0.3;
    assert(r() == 0.3);
    assert(r() == 0.3);
    assert(r() == 0.3);
    std::cout << " ... PASSED]" << std::endl;
}

void
TestFixedSeries(void)
{
    std::cout << std::left << "[" << std::setw(width) << "TestFixedSeries";
    erin::FixedSeries r{};
    assert(r() == 0.0);
    r.Series.push_back(0.1);
    r.Series.push_back(0.2);
    r.Series.push_back(0.3);
    assert(r() == 0.1);
    assert(r() == 0.2);
    assert(r() == 0.3);
    assert(r() == 0.1);
    assert(r() == 0.2);
    assert(r() == 0.3);
    assert(r() == 0.1);
    std::cout << " ... PASSED]" << std::endl;
}

void
TestRandomWithSeed(void)
{
    std::cout << std::left << "[" << std::setw(width) << "TestRandomWithSeed";
    erin::Random r = erin::CreateRandomWithSeed(17);
    for (size_t i = 0; i < 1'000; ++i)
    {
        assert(r() >= 0.0 && r() <= 1.0);
    }
    std::cout << " ... PASSED]" << std::endl;
}

void
TestRandomSeededFromClock(void)
{
    std::cout << std::left << "[" << std::setw(width)
              << "TestRandomSeededFromClock";
    erin::Random r = erin::CreateRandom();
    for (size_t i = 0; i < 1'000; ++i)
    {
        assert(r() >= 0.0 && r() <= 1.0);
    }
    std::cout << " ... PASSED]" << std::endl;
}

int
main(int argc, char* argv[])
{
    std::cout << "Testing Random Functionality:" << std::endl;
    TestFixedRandom();
    TestFixedSeries();
    TestRandomWithSeed();
    TestRandomSeededFromClock();
    return EXIT_SUCCESS;
}
