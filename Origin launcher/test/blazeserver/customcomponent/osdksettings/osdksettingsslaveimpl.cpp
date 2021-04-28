/*************************************************************************************************/
/*!
    \file   osdksettingsslaveimpl.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/test/blazeserver/customcomponent/osdksettings/osdksettingsslaveimpl.cpp#1 $
    $Change: 1636281 $
    $DateTime: 2020/12/03 11:59:08 $

    \brief

    This is the implementation of the osdksettings component on the slave.

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OSDKSettingsSlaveImpl

    OSDKSettings Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdksettingsslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

// osdksettings includes
#include "osdksettings/tdf/osdksettingstypes.h"

namespace Blaze
{
namespace OSDKSettings
{

/*static*/ OSDKSettingsSlave* OSDKSettingsSlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "OSDKSettingsSlaveImpl") OSDKSettingsSlaveImpl();
}

/*** Public Methods ******************************************************************************/
OSDKSettingsSlaveImpl::OSDKSettingsSlaveImpl()
{
}

OSDKSettingsSlaveImpl::~OSDKSettingsSlaveImpl()
{
}

} // OSDKSettings
} // Blaze
     
