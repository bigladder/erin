#include "generator.h"

Generator::Generator():
adevs::Atomic<Customer>()
{
    // Store the arrivals in a list
    int next_arrival_time = 0;
    int last_arrival_time = 0;
    // Hard-code the input data for ease of use...
    std::vector<int> next_arrival_times{1, 2, 3, 5, 7, 8, 10, 11};
    std::vector<int> twaits{1, 4, 4, 2, 10, 20, 2, 1};
    assert(twaits.size() == next_arrival_times.size());
    auto twaits_size = twaits.size();
    for (decltype(twaits_size) idx=0; idx < twaits_size; idx++) {
        auto c = Customer();
        next_arrival_time = next_arrival_times.at(idx);
        c.twait = twaits.at(idx);
        c.tenter = next_arrival_time - last_arrival_time;
        _arrivals.emplace_back(c);
        last_arrival_time = next_arrival_time;
    }
}

adevs::Time
Generator::ta()
{
    // If there are not more customers, the next event is infinity
    if (_arrivals.empty())
    {
        return adevs_inf<adevs::Time>();
    }
    // Otherwise, wait until the next arrival
    return adevs::Time(_arrivals.front().tenter, 0);
}

void
Generator::delta_int()
{
    // Remove the first customer.
    _arrivals.pop_front();
}

void
Generator::delta_ext(adevs::Time e, std::vector<Customer>& x)
{
    // The generator is input free, and so it ignores external events.
}

void
Generator::delta_conf(std::vector<Customer>& x)
{
    // The generator is input free, and so it ignores any input.
    delta_int();
}

void
Generator::output_func(std::vector<Customer>& y)
{
    y.emplace_back(Customer(_arrivals.front()));
}
