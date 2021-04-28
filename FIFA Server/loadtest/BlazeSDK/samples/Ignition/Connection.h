
#ifndef IGNITION_CONNECTION_H
#define IGNITION_CONNECTION_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"


namespace Ignition
{

INTRUSIVE_PTR(Connection);

class Connection
    : public BlazeHubUiBuilder,
      public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener,
      public BlazeHubUiDriver::BlazeHubConnectionListener,
      protected Blaze::ConnectionManager::ConnectionManagerListener // for onQosPingSiteLatencyRetrieved
{
    friend class BlazeHubUiDriver;

    public:
        Connection();
        virtual ~Connection();

        Pyro::UiNodeParameterStructPtr getLoginParameters() { return mLoginParameters; }

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    protected:
        virtual void onConnected();
        virtual void onDisconnected();

    private:
        Pyro::UiNodeParameterStructPtr mLoginParameters;

        PYRO_ACTION(Connection, SetClientPlatformOverride);
        PYRO_ACTION(Connection, Connect);
        PYRO_ACTION(Connection, ConnectAndLogin);
        PYRO_ACTION(Connection, Disconnect);
        PYRO_ACTION(Connection, DirtyDisconnect);
        PYRO_ACTION(Connection, SetNetworkQosDataDebug);
        PYRO_ACTION(Connection, SetQosPingSiteLatencyDebug);
        PYRO_ACTION(Connection, GetPermissions);
        PYRO_ACTION(Connection, SetClientState);
        PYRO_ACTION(Connection, GetDrainStatus);
        PYRO_ACTION(Connection, SetNewServiceNameAndEnvironment);

        void connectionMessagesCb(Blaze::BlazeError blazeError, const Blaze::Redirector::DisplayMessageList *displayMessageList);

        void onQosPingSiteLatencyRetrieved(bool success);
};

}

#endif
