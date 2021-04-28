/*************************************************************************************************/
/*!
    \file   osdkclubshelperslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved. 2010
*/
/*************************************************************************************************/

#ifndef BLAZE_OSDKCLUBSHELPER_SLAVEIMPL_H
#define BLAZE_OSDKCLUBSHELPER_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "osdkclubshelper/rpc/osdkclubshelperslave_stub.h"
#include "framework/replication/replicationcallback.h"
#include "osdkclubshelperconfiguration.h"
#include "osdkclubshelper/tdf/osdkclubshelpertypes_server.h"

#include "clubs/clubsslaveimpl.h"
#include "clubs/rpc/clubsslave/updateclubsettings_stub.h"  //for the updateclubsettings command hook defs.

#include "stats/statsslaveimpl.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace OSDKClubsHelper
{

class OSDKClubsHelperSlaveImpl : public OSDKClubsHelperSlaveStub
{
public:
    OSDKClubsHelperSlaveImpl();
    ~OSDKClubsHelperSlaveImpl();

private:

    virtual bool onConfigure();
    virtual bool onReconfigure();
    bool configureHelper();

    //Functor for hook callback for the updateClubSettings command.
    void updateClubSettingsRequestHookCallback(Blaze::Clubs::UpdateClubSettingsRequest& request, BlazeRpcError& blazeError, bool& executeRpc);
 
    bool isValidTeam(uint32_t teamId);
    bool isValidSetting(const OSDKClubsHelper::ValidSettingsContainer& settings, uint32_t value);

    const ConfigMap* mConfigMap;
    OSDKClubsHelperValidationSettings mValidationConfig;

};

} // OSDKClubsHelper
} // Blaze

#endif // BLAZE_OSDKCLUBSHELPER_SLAVEIMPL_H
