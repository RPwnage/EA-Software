
#ifndef CENSUS_H
#define CENSUS_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

#include <BlazeSDK/censusdata/censusdataapi.h>
#include <BlazeSDK/gamemanager/gamemanagerapi.h>

namespace Ignition
{

class Census
    : public BlazeHubUiBuilder,
      public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener,
      protected Blaze::CensusData::CensusDataAPIListener
{
    public:
        Census();
        virtual ~Census();

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    protected:
        void onNotifyCensusData();

    private:
        PYRO_ACTION(Census, StartListeningToCensus);
        PYRO_ACTION(Census, StopListeningToCensus);
        PYRO_ACTION(Census, GetLatestCensusData);

        void SubscribeToCensusDataCb(Blaze::BlazeError blazeError);
        void UnsubscribeToCensusDataCb(Blaze::BlazeError blazeError);
        void GetLatestCensusDataCb(const Blaze::CensusData::NotifyServerCensusData *resultValue, Blaze::BlazeError blazeError, Blaze::JobId jobId);

        void updateCensusWindow(const Blaze::GameManager::GameManagerCensusData* gmCensusData, const Blaze::Clubs::ClubsCensusData* clubsCensusData,
                                const Blaze::UserManagerCensusData* userSessionCensusData);
};

}

#endif
