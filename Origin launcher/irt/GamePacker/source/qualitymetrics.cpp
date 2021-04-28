#include "gamepacker/internal/packerinternal.h"
#include "gamepacker/qualitymetrics.h"

namespace Packer
{

void FactorMetrics::accumulateMetric(double value)
{
    // should deal with potential overflow, by maybe just clearing everything?
    if (count++ > 0)
    {
        if (min > value)
            min = value;
        if (max < value)
            max = value;
    }
    else
    {
        min = max = value;
    }
    sum += value;

    // iteratively calculate/update variance parameters (used to derive standard deviation aka sigma)
    // source: https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
    double delta = value - mean;
    mean += delta / count;
    double delta2 = value - mean;
    m2 += delta2*delta;
}

} // GamePacker
