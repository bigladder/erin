#include "clerk.h"
#include <iostream>

Clerk::Clerk():
    adevs::Atomic<Customer>(), // Initialize the parent Atomic model
    _t(0), // Set the clock to zero
    _t_spent(0), // No time spent on a customer so far
    _verbose(false)
{}

void
Clerk::delta_ext(adevs::Time e, std::vector<Customer>& x)
{
    if (_verbose)
    {
        // Print a notice
        std::cout << "Clerk: Computed the external transition function at t = " << (_t + e.real) << std::endl;
    }
    // Update the clock
    _t += e.real;
    // Update the time spent on the customer at the front of the line
    if (!_line.empty())
    {
        _t_spent += e.real;
    }
    // Add new customers to the back of the line.
    std::vector<Customer>::const_iterator it;
    for (it = x.begin(); it != x.end(); it++)
    {
        // Copy the incoming customer and place them at the back of the line.
        _line.push_back(Customer(*it));
        _line.back().tenter = _t;
    }
    // Summarize the model state
    if (_verbose)
    {
        std::cout << "Clerk: There are " << _line.size() << " customers waiting." << std::endl;
        std::cout << "Clerk: The next customer will leave at t = " << (_t + ta().real) << "." << std::endl;
    }
}

void
Clerk::delta_int()
{
    if (_verbose)
    {
        // Print a noitice of the internal transition
        std::cout << "Clerk: Computed the internal transition function at t = " << (_t + ta().real) << std::endl;
    }
    // Update the clock
    _t += ta().real;
    // Reset the spent time
    _t_spent = 0;
    // Remove the departing customer from the front of the line;
    _line.pop_front();
    if (_verbose)
    {
        // Summarize the model state
        std::cout << "Clerk: There are " << _line.size() << " customers waiting." << std::endl;
        std::cout << "Clerk: The next customer will leave at t = " << (_t + ta().real) << "." << std::endl;
    }
}

void
Clerk::delta_conf(std::vector<Customer>& x)
{
    delta_int();
    delta_ext(adevs::Time(0, 0), x);
}

void
Clerk::output_func(std::vector<Customer>& y)
{
    // Get the departing customer
    Customer leaving = _line.front();
    // Set the departure time
    leaving.tleave = _t + ta().real;
    // Eject the customer
    y.push_back(leaving);
    if (_verbose)
    {
        // Print a notice of the departure
        std::cout << "Clerk: Computed the output function at t = " << (_t + ta().real) << std::endl;
        std::cout << "Clerk: A customer just departed!" << std::endl;
    }
}

adevs::Time
Clerk::ta()
{
    if (_line.empty())
    {
        return adevs_inf<adevs::Time>();
    }
    return adevs::Time(_line.front().twait - _t_spent, 0);
}