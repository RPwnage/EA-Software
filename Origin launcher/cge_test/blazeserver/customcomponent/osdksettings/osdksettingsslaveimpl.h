/*************************************************************************************************/
/*!
    \file   osdksettingsslaveimpl.h

    $Header: //gosdev/games/FIFA/2022/GenX/cge_test/blazeserver/customcomponent/osdksettings/osdksettingsslaveimpl.h#1 $
    $Change: 1653004 $
    $DateTime: 2021/03/02 12:48:45 $

    \brief

    This is the osdksettings component's implementation on the slave.

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

#ifndef BLAZE_OSDKSETTINGS_SLAVEIMPL_H
#define BLAZE_OSDKSETTINGS_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "osdksettings/rpc/osdksettingsslave_stub.h"
#include "framework/replication/replicationcallback.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

// forward declarations
class ConfigMap;

namespace OSDKSettings
{

// forward declarations
class OSDKSettingsConfig;
class SettingString;

class OSDKSettingsSlaveImpl : public OSDKSettingsSlaveStub
{
public:
    OSDKSettingsSlaveImpl();
    ~OSDKSettingsSlaveImpl();

    const OSDKSettingsConfigTdf& getSettingsConfig() { return getConfig(); }
};

} // OSDKSettings
} // Blaze

#endif // BLAZE_OSDKSETTINGS_SLAVEIMPL_H

