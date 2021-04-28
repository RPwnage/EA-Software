/*************************************************************************************************/
/*!
    \file   grantachievement_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/grantachievement_stub.h"

#include "gamereporting/util/achievementsutil.h"

namespace Blaze
{
namespace Arson
{
class GrantAchievementCommand : public GrantAchievementCommandStub
{
public:
    GrantAchievementCommand(
        Message* message, Blaze::Achievements::GrantAchievementRequest* request, ArsonSlaveImpl* componentImpl)
        : GrantAchievementCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GrantAchievementCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GrantAchievementCommandStub::Errors execute() override
    {
        Achievements::AchievementsSlave * achievementsComponent = (Achievements::AchievementsSlave *)gController->getComponent(Achievements::AchievementsSlave::COMPONENT_ID);
        
        if(nullptr != achievementsComponent)
        {
            if(Blaze::ERR_OK == achievementsComponent->grantAchievement(mRequest, mResponse))
                return ERR_OK;
        }
        
        return ERR_SYSTEM;
    }    
};

GrantAchievementCommandStub* GrantAchievementCommandStub::create(Message *msg, Blaze::Achievements::GrantAchievementRequest* request, ArsonSlave *component)
{
    return BLAZE_NEW GrantAchievementCommand(msg, request, static_cast<ArsonSlaveImpl*>(component));
}

} //Arson
} //Blaze
