/*************************************************************************************************/
/*!
    \file   changekeyscopevalue_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ChangeKeyscopeValueCommand

    Change Keyscope Value.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statsslaveimpl.h"
#include "stats/rpc/statsslave/changekeyscopevalue_stub.h"

namespace Blaze
{
namespace Stats
{

class ChangeKeyscopeValueCommand : public ChangeKeyscopeValueCommandStub
{
public:
    ChangeKeyscopeValueCommand(Message *message, KeyScopeChangeRequest *request, StatsSlaveImpl* componentImpl) 
    : ChangeKeyscopeValueCommandStub(message, request),
      mComponent(componentImpl)
    {
    }

    ChangeKeyscopeValueCommandStub::Errors execute() override
    {
        BlazeId blazeId = mRequest.getEntityId();
        if (gCurrentUserSession == nullptr)
        {
            if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_UPDATE_STATS))
            {
                WARN_LOG("[UpdateStatsCommand].execute: Cannot change keyscopes without permission");
                return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
            }
        }
        else
        {
            bool authorized = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_UPDATE_STATS, true);
            if(gCurrentUserSession->getBlazeId() != blazeId && !authorized)
            {
                WARN_LOG("[ChangeKeyscopeValueCommand].execute: User [" << gCurrentUserSession->getBlazeId() 
                    << "] with no update stats permissions can't set another user [" << mRequest.getEntityId() <<"]'s keyscope value");
                return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);
            }
            if(!mComponent->isKeyScopeChangeable(mRequest.getKeyScopeName()) && !authorized)
            {
                WARN_LOG("[ChangeKeyscopeValueCommand].execute: User [" << gCurrentUserSession->getBlazeId()
                    << "] with no update stats permissions can't modify Keyscope [" << mRequest.getKeyScopeName()
                    << "] since it is not in the KeyScopeChangeMap.");
                return commandErrorFromBlazeError(Blaze::ERR_AUTHORIZATION_REQUIRED);            
            }
        }
        
        BlazeRpcError error = mComponent->changeKeyscopeValue(mRequest);
        return (commandErrorFromBlazeError(error));
    }

private:
    StatsSlaveImpl* mComponent;
};

DEFINE_CHANGEKEYSCOPEVALUE_CREATE()

} // Stats
} // Blaze
