
#ifndef NISLEADERBOARD_H
#define NISLEADERBOARD_H

#include "NisSample.h"

#include "BlazeSDK/statsapi/lbapi.h"

namespace NonInteractiveSamples
{

class NisLeaderboards
    : public NisSample
{
    public:
        NisLeaderboards(NisBase &nisBase);
        virtual ~NisLeaderboards();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void            GetLeaderboardCb(Blaze::BlazeError err, Blaze::JobId, Blaze::Stats::Leaderboard* lb);
        void            CreateGlobalViewCb(Blaze::BlazeError err, Blaze::JobId, Blaze::Stats::GlobalLeaderboardView* view);

        Blaze::Stats::LeaderboardAPI * mApi;
};

}

#endif // NISLEADERBOARD_H
