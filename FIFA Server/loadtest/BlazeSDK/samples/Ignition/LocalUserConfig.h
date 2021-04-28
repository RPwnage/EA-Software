
#ifndef LOCAL_USER_CONFIG_H
#define LOCAL_USER_CONFIG_H

#include "Ignition/Ignition.h"
#include "Ignition/BlazeInitialization.h"

namespace Ignition
{

class LocalUserConfig
    : public BlazeHubUiBuilder,
      public BlazeHubUiDriver::PrimaryLocalUserAuthenticationListener,
      public Blaze::BlazeStateEventHandler
{
    public:
        LocalUserConfig();
        virtual ~LocalUserConfig();

        virtual void onConnected() {}
        virtual void onDisconnected(Blaze::BlazeError errorCode) {}
        virtual void onAuthenticated(uint32_t userIndex);
        virtual void onDeAuthenticated(uint32_t userIndex);

        virtual void onAuthenticatedPrimaryUser();
        virtual void onDeAuthenticatedPrimaryUser();

    private:

        void updateLUGroupCb(Blaze::BlazeError error, Blaze::JobId jobId, const Blaze::UpdateLocalUserGroupResponse * response);

        PYRO_ACTION(LocalUserConfig, UpdateLocalUserIndices);

        Blaze::ConnectionUserIndexList mSelectedIndices;

        typedef eastl::set<uint32_t> ConnUserIndexUpdateSet;
        ConnUserIndexUpdateSet mConnUserIndexUpdateSet;
};

}

#endif
