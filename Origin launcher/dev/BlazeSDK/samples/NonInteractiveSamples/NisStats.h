
#ifndef NISSTATS_H
#define NISSTATS_H

#include "NisSample.h"

#include "BlazeSDK/statsapi/statsapi.h"

namespace NonInteractiveSamples
{

class NisStats : 
    public NisSample
{
    public:
        NisStats(NisBase &nisBase);
        virtual ~NisStats();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            GetKeyScopeCb(Blaze::BlazeError err, Blaze::JobId id, const Blaze::Stats::KeyScopes* keyScopes);
        void            GetStatsGroupCb(Blaze::BlazeError err, Blaze::JobId, Blaze::Stats::StatsGroup* group);
        void            CreateViewCb(Blaze::BlazeError err, Blaze::JobId, Blaze::Stats::StatsView* view);
        void            DescribeStatValues(const Blaze::Stats::StatValues * statValues);

        Blaze::Stats::StatsAPI * mStatsApi;
};

}

#endif // NISSTATS_H
