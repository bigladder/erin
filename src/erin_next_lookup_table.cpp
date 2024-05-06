#include "erin_next/erin_next_lookup_table.h"

namespace erin
{
    double
    LookupTable_LookupStairStep(
        std::vector<double> const& xs,
        std::vector<double> const& ys,
        double x
    )
    {
        assert(xs.size() == ys.size());
        assert(xs.size() > 0);
        size_t numEntries = xs.size();
        size_t maxIdx = numEntries - 1;
        if (x <= xs[0])
        {
            return ys[0];
        }
        if (x >= xs[maxIdx])
        {
            return ys[maxIdx];
        }
        for (size_t i = 1; i < numEntries; ++i)
        {
            if (x >= xs[i - 1] && x < xs[i])
            {
                return ys[i - 1];
            }
        }
        return ys[maxIdx];
    }

    double
    LookupTable_LookupInterp(
        std::vector<double> const& xs,
        std::vector<double> const& ys,
        double x
    )
    {
        assert(xs.size() == ys.size());
        assert(xs.size() > 0);
        size_t numEntries = xs.size();
        size_t maxIdx = numEntries - 1;
        if (x <= xs[0])
        {
            return ys[0];
        }
        if (x >= xs[maxIdx])
        {
            return ys[maxIdx];
        }
        for (size_t i = 1; i < numEntries; ++i)
        {
            if (x >= xs[i - 1] && x < xs[i])
            {
                // y_delta / dy = x_delta / dx
                // y_delta = (x_delta / dx) * dy
                // y = y0 + y_delta
                double dx = xs[i] - xs[i - 1];
                double dy = ys[i] - ys[i - 1];
                double x_delta = x - xs[i - 1];
                double y_delta = (x_delta / dx) * dy;
                return ys[i - 1] + y_delta;
            }
        }
        return ys[maxIdx];
    }
}
