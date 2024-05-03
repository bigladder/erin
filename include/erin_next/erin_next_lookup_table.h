/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE.txt file for additional terms and conditions. */
#ifndef ERIN_LOOKUP_TABLE_H
#define ERIN_LOOKUP_TABLE_H

#include <vector>
#include <assert.h>

namespace erin
{
    // Assumptions:
    // - xs must be sorted in ascending order with no duplicates
    // - ys.size() == xs.size()
    // edge extension:
    // if x is less than xs[0] or greater than xs[xs.size()-1], then we return
    // ys[0] or ys[ys.size()-1], respectively
    double
    LookupTable_LookupStairStep(
        std::vector<double> const& xs,
        std::vector<double> const& ys,
        double x
    );

    double
    LookupTable_LookupInterp(
        std::vector<double> const& xs,
        std::vector<double> const& ys,
        double x
    );
}

#endif
