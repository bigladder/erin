#ifndef CHECKOUT_LINE_CLERK_H
#define CHECKOUT_LINE_CLERK_H
#include "../../vendor/bdevs/include/adevs.h"
#include "customer.h"
#include <vector>
#include <list>

class Clerk : public adevs::Atomic<Customer>
{
    public:
        Clerk();
        void delta_int() override;
        void delta_ext(adevs::Time e, std::vector<Customer>& x) override;
        void delta_conf(std::vector<Customer>& x) override;
        adevs::Time ta() override;
        void output_func(std::vector<Customer>& y) override;

    private:
        // The clerk's clock
        int _t;
        // Waiting customers
        std::list<Customer> _line;
        // Time spent so far on the customer at the front of the line
        int _t_spent;
        bool _verbose;
};

#endif
