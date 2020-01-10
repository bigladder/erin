#ifndef CHECKOUT_LINE_CUSTOMER_H
#define CHECKOUT_LINE_CUSTOMER_H

struct Customer {
    // Time needed for the clerk to process the customer
    int twait;
    // Time that the customer entered and left the queue
    int tenter, tleave;
};

#endif
