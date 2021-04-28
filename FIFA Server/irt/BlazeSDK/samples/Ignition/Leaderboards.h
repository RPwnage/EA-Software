
#ifndef LEADERBOARDS_H
#define LEADERBOARDS_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/statsapi/lbapi.h"

namespace Ignition
{

class Leaderboards :
    public BlazeHubUiBuilder,
    public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener
{
    public:
        Leaderboards();
        virtual ~Leaderboards();

        void GetLeaderboardDataTopRanked(const char8_t *leaderboardName);
        void GetLeaderboardDataCenteredAroundCurrentPlayer(const char8_t *leaderboardName);

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    private:
        PYRO_ACTION(Leaderboards, ViewLeaderboards);
        PYRO_ACTION(Leaderboards, GetLeaderboardDataTopRanked);
        PYRO_ACTION(Leaderboards, GetLeaderboardDataCenteredAroundCurrentPlayer);

        void traverseLeaderboardFolderTree(const Blaze::Stats::LeaderboardTreeFolder *folder, Pyro::UiNodeTableRowContainer &rows);

        void populateSampleScopeNameValueMap(const char8_t *leaderboardName, Blaze::Stats::ScopeNameValueMap *map);

        void GetLeaderboardTreeCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::LeaderboardTree *leaderboardTree);
        void GetLeaderboardCb_GlobalView(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::Leaderboard *leaderboard);
        void GetLeaderboardCb_CenteredView(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::Leaderboard *leaderboard);
        void CreateGlobalViewCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::GlobalLeaderboardView *globalLeaderboardView);

};

}

#endif
