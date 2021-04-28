
#ifndef NISBASE_H
#define NISBASE_H

#include "PyroSDK/pyrosdk.h"

#include "BlazeSDK/blazehub.h"

#include "NisCensus.h"
#include "NisEntitlements.h"
#include "NisAssociationList.h"
#include "NisGameBrowsing.h"
#include "NisGameReporting.h"
#include "NisGeoIp.h"
#include "NisJoinGame.h"
#include "NisLeaderboardList.h"
#include "NisLeaderboards.h"
#include "NisMessaging.h"
#include "NisQos.h"
#include "NisServerConfig.h"
#include "NisSmallUserStorage.h"
#include "NisStats.h"
#include "NisTelemetry.h"



namespace NonInteractiveSamples
{

INTRUSIVE_PTR(NisBase);

class NisBase
    : public Pyro::UiBuilder,
      public Blaze::BlazeStateEventHandler    
{
    public:
        NisBase();
        virtual ~NisBase();

        Blaze::BlazeHub *getBlazeHub() { return mBlazeHub; }

        void killRunningNis();
        virtual void onIdle();

    protected:
        virtual void onStarting();
        virtual void onStopping();

        /*! ************************************************************************************************/
        /*! \brief The BlazeSDK has made a connection to the blaze server.  This is user independent, as
            all local users share the same connection.
        ***************************************************************************************************/
        virtual void onConnected();

        /*! ************************************************************************************************/
        /*! \brief Notification that a local user has been authenticated against the blaze server.  
            The provided user index indicates which local user authenticated.  If multiple local users
            authenticate, this will trigger once per user.
        
            \param[in] userIndex - the user index of the local user that authenticated.
        ***************************************************************************************************/
        virtual void onAuthenticated(uint32_t userIndex);

    private:
        void actionConnectAndLogin(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);
        void actionRunNisSample(Pyro::UiNodeAction *action, Pyro::UiNodeParameterStructPtr parameters);

        void connectionMessagesCb(Blaze::BlazeError blazeError, const Blaze::Redirector::DisplayMessageList *displayMessageList);

        static void BlazeSdkLogFunction(const char8_t *pText, void *data);

    private:
        Blaze::BlazeHub *mBlazeHub;
        Blaze::BlazeHub::InitParameters mInitParameters;

        Blaze::BlazeNetworkAdapter::ConnApiAdapter *mNetAdapter;

        char8_t mEmail[256];
        char8_t mPassword[256];
        char8_t mPersonaName[256];

        NisSample *mRunningNis;
        bool mKillRunningNis;

        Pyro::UiNodeAction *mActionConnectAndLogin;

        NisCensus *mNisCensus;
        NisAssociationList *mNisAssociationList;
        NisGameBrowsing *mNisGameBrowsing;
        NisEntitlements *mNisEntitlements;
        NisGameReporting *mNisGameReporting;
        NisGeoIp *mNisGeoIp;
        NisJoinGame *mNisJoinGame;
        NisLeaderboardList *mNisLeaderboardList;
        NisLeaderboards *mNisLeaderboards;
        NisMessaging *mNisMessaging;
        NisQos *mNisQos;
        NisServerConfig *mNisServerConfig;
        NisSmallUserStorage *mNisSmallUserStorage;
        NisStats *mNisStats;
        NisTelemetry *mNisTelemetry;
};

void initializePlatform();
void tickPlatform();
void finalizePlatform();

}

#endif
