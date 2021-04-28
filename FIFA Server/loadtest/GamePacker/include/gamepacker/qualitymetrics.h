/*************************************************************************************************/
/*!
    \file

    \attention    
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef GAME_PACKER_QUALITYMETRICS_H
#define GAME_PACKER_QUALITYMETRICS_H

namespace Packer
{

struct GameQualityMetrics
{
    uint64_t mImproved;
    uint64_t mUnchanged;
    uint64_t mDegraded;
    uint64_t mRejected;
};

struct FactorMetrics
{
    double min;
    double max;
    double sum;
    double mean;
    double m2;
    uint64_t count;

    FactorMetrics() : min(0), max(0), sum(0), mean(0), m2(0), count(0) {}
    void accumulateMetric(double value);
};

typedef eastl::vector<FactorMetrics> GameQualityFactorMetrics;

}
#endif

