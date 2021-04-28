/*************************************************************************************************/
/*!
    \file   fetchsettingsgroupsslave_command.cpp

    $Header: //gosdev/games/FIFA/2022/GenX/test/blazeserver/customcomponent/osdksettings/fetchsettingsgroupsslave_command.cpp#1 $
    $Change: 1636281 $
    $DateTime: 2020/12/03 11:59:08 $

    \brief

    This is the slave version of the FetchSettingsGroups command.

    \attention
        (c) Electronic Arts Inc. 2009
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FetchSettingsGroupsSlaveCommand

    Requests settings groups information from the slave.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "osdksettings/rpc/osdksettingsslave/fetchsettingsgroups_stub.h"

// global includes

// framework includes
#include "framework/connection/selector.h"

// osdksettings includes
#include "osdksettings/osdksettingsslaveimpl.h"
#include "osdksettings/tdf/osdksettingstypes.h"

namespace Blaze
{
namespace OSDKSettings
{

    class FetchSettingsGroupsSlaveCommand : public FetchSettingsGroupsCommandStub
    {
        /*** Public Methods ******************************************************************************/
    public:
        FetchSettingsGroupsSlaveCommand(Message* message, FetchSettingsGroupsRequest* request, OSDKSettingsSlaveImpl* componentImpl)
            : FetchSettingsGroupsCommandStub(message, request), mComponent(componentImpl)
        { }

        virtual ~FetchSettingsGroupsSlaveCommand() { }

        /*** Private methods *****************************************************************************/
    private:

        FetchSettingsGroupsCommandStub::Errors execute()
        {
            TRACE_LOG("[FetchSettingsGroupsSlaveCommand:" << this << "].execute()");

            const OSDKSettingsConfigTdf& config = mComponent->getSettingsConfig();

            // fill the response's group list with groups from the parser
            SettingGroupList &responseGroupListPtr = mResponse.getSettingGroupList();

            for (SettingGroupList::const_iterator it = config.getGroups().begin();
                it != config.getGroups().end(); ++it)
            {
                SettingGroup* reservedSpace = responseGroupListPtr.pull_back();
                (*it)->copyInto(*reservedSpace);
            }

            return ERR_OK;
        }

    private:
        // Not owned memory.
        OSDKSettingsSlaveImpl* mComponent;
    };


    // static factory method impl
    FetchSettingsGroupsCommandStub* FetchSettingsGroupsCommandStub::create(Message *msg, FetchSettingsGroupsRequest* request, OSDKSettingsSlave *component)
    {
        return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "FetchSettingsGroupsSlaveCommand") FetchSettingsGroupsSlaveCommand(msg, request, static_cast<OSDKSettingsSlaveImpl*>(component));
    }

} // OSDKSettings
} // Blaze
