
#ifndef NISLEADERBOARDLIST_H
#define NISLEADERBOARDLIST_H

#include "NisSample.h"

#include "BlazeSDK/statsapi/lbapi.h"

#include <EASTL/map.h>
#include <EASTL/string.h>

namespace NonInteractiveSamples
{

class NisLeaderboardList
    : public NisSample
{
    public:
        NisLeaderboardList(NisBase &nisBase);
        virtual ~NisLeaderboardList();

    protected:
        virtual void onCreateApis();
        virtual void onCleanup();

        virtual void onRun();

    private:
        void GetLeaderboardFolderCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::Stats::LeaderboardFolder *leaderboardFolder);

        void GetLeaderboardFolderListCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::Stats::LeaderboardFolderList *leaderboardFolderList);
        void GetLeaderboardListCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::Stats::LeaderboardList *leaderboardList);

        Blaze::Stats::LeaderboardAPI * mApi;
};

}

#endif // NISLEADERBOARDLIST_H
