#ifndef CHECKOUT_LINE_OBSERVER_H
#define CHECKOUT_LINE_OBSERVER_H
#include "../../vendor/bdevs/include/adevs.h"
#include "customer.h"
#include <sstream>
#include <string>

/**
 * The Observer records performance statistics for the Clerk model
 * based on its observable output.
 */
class Observer : public adevs::Atomic<Customer>
{
    public:
        Observer();
        std::string get_results();
        void delta_int() override;
        void delta_ext(adevs::Time e, std::vector<Customer>& x) override;
        void delta_conf(std::vector<Customer>& x) override;
        adevs::Time ta() override;
        void output_func(std::vector<Customer>& y) override;
    
    private:
        std::ostringstream _oss;
};

#endif
