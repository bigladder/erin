#ifndef CHECKOUT_LINE_GENERATOR_H
#define CHECKOUT_LINE_GENERATOR_H
#include "../../vendor/bdevs/include/adevs.h"
#include "customer.h"
#include <vector>
#include <list>

/**
 * This class produces Customers according to the provided schedule.
 */
class Generator : public adevs::Atomic<Customer>
{
    public:
        // Constructor
        Generator();
        // Internal transition function.
        void delta_int() override;
        // External transition function.
        void delta_ext(adevs::Time e, std::vector<Customer>& x) override;
        // Confluent transition function.
        void delta_conf(std::vector<Customer>& x) override;
        // Output function.
        void output_func(std::vector<Customer>& y) override;
        // Time advance function.
        adevs::Time ta() override;

    private:
        std::list<Customer> _arrivals;
        static const int arrive;
};

#endif