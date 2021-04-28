
#ifndef STATS_H
#define STATS_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include "BlazeSDK/statsapi/statsapi.h"

namespace Ignition
{

class Stats :
    public BlazeHubUiBuilder,
    public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener
{
    public:
        Stats();
        virtual ~Stats();

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

        void runRequestKeyScopes();

        static void setupCreateStatsViewParameters(Pyro::UiNodeActionPtr actionCreateStatsView, Blaze::Stats::StatsGroup *statsGroup);

    private:
        Blaze::Stats::StatsGroup *mSelectedStatsGroup;
        Blaze::Stats::StatsView *mCurrentStatsView;

        void refreshStatsGroup();

        PYRO_ACTION(Stats, RequestStatsGroupList);
        PYRO_ACTION(Stats, CreateStatsView);

        PYRO_ACTION(Stats, GetStatsGroup);

        void GetKeyScopeCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::Stats::KeyScopes *keyScropes);
        void GetStatsGroupListCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, const Blaze::Stats::StatGroupList *statGroupList);
        void GetStatsGroupCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::StatsGroup *statsGroup);
        void CreateViewCb(Blaze::BlazeError blazeError, Blaze::JobId jobId, Blaze::Stats::StatsView *statsView);
};

}

#endif
