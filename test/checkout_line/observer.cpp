#include "observer.h"

Observer::Observer():
adevs::Atomic<Customer>(),
_oss()
{
    _oss << "# Col 1: Time customer enters the line" << std::endl;
    _oss << "# Col 2: Time required for customer checkout" << std::endl;
    _oss << "# Col 3: Time customer leaves the store" << std::endl;
    _oss << "# Col 4: Time spent waiting in line" << std::endl;
}

adevs::Time
Observer::ta()
{
    // The observer has no autonomous behavior, so time-advance is always infinity.
    return adevs_inf<adevs::Time>();
}

void
Observer::delta_int()
{
    // The observer has no autonomous behavior, so do nothing.
}

void
Observer::delta_ext(adevs::Time e, std::vector<Customer>& x)
{
    std::vector<Customer>::const_iterator it;
    for (it = x.begin(); it != x.end(); it++)
    {
        const Customer c = (*it);
        int waiting_time = (c.tleave - c.tenter) - c.twait;
        _oss << c.tenter << " " << c.twait << " " << c.tleave << " " << waiting_time << std::endl;
    }
}

void
Observer::delta_conf(std::vector<Customer>& x)
{
    // The observer has no autonomous behavior so do nothing
}

void
Observer::output_func(std::vector<Customer>& y)
{
    // The observer produces no output so do nothing
}


std::string
Observer::getResults()
{
    return _oss.str();
}