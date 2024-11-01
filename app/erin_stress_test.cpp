// Copyright (c) 2020 - 2024 Big Ladder Software, LLC.
// See the LICENSE.txt file for additional terms and conditions.
#include <assert.h>
#include <iostream>
#include <stdint.h>
#include <string>

#include "erin/all.h"

using namespace erin;

int main(int argc, char** argv)
{
    size_t num_components = 5'000;
    size_t num_hours = 8'760;
    if (argc == 3)
    {
        num_components = static_cast<size_t>(std::stoll(std::string {argv[1]}));
        num_hours = static_cast<size_t>(std::stoll(std::string {argv[2]}));
    }
    std::cout << "Running " << num_components << " components for " << num_hours << " hours"
              << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    Model m = {};
    m.RandFn = []() { return 0.4; };
    m.FinalTime = 8760.0 * 3600.0;
    std::vector<TimeAndAmount> times_and_loads = {};
    times_and_loads.reserve(num_hours + 1);
    for (size_t i = 0; i <= num_hours; ++i)
    {
        times_and_loads.push_back(TimeAndAmount {((double)i) * 3600.0, 1});
    }
    for (size_t i = 0; i < num_components; ++i)
    {
        auto src_id = Model_AddConstantSource(m, 100);
        auto load_id = Model_AddScheduleBasedLoad(m, times_and_loads);
        Model_AddConnection(m, src_id, 0, load_id, 0);
    }
    auto stop_constr = std::chrono::high_resolution_clock::now();
    auto duration_constr = std::chrono::duration_cast<std::chrono::microseconds>(stop_constr - start);
    std::cout << "Construction time: " << ((double)duration_constr.count() / 1000.0) << " ms"
              << std::endl;
    auto results = Simulate(m, false);
    assert(results.size() == num_hours + 1 && "Results is not of expected length");
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration_sim = std::chrono::duration_cast<std::chrono::microseconds>(stop - stop_constr);
    std::cout << "Sim time: " << ((double)duration_sim.count() / 1000.0) << " ms" << std::endl;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    std::cout << "Total time: " << ((double)duration.count() / 1000.0) << " ms" << std::endl;
    return EXIT_SUCCESS;
}
