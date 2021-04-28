/*************************************************************************************************/
/*!
    \file   getachievements_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getachievements_stub.h"
#include "component/achievements/achievementsslaveimpl.h"

namespace Blaze
{
namespace Arson
{
class GetAchievementsCommand : public GetAchievementsCommandStub
{
public:
    GetAchievementsCommand(
        Message* message, Blaze::Achievements::GetAchievementsRequest* request, ArsonSlaveImpl* componentImpl)
        : GetAchievementsCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetAchievementsCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetAchievementsCommandStub::Errors execute() override
    {
        Achievements::AchievementsSlave * achievementsComponent = (Achievements::AchievementsSlave *)gController->getComponent(Achievements::AchievementsSlave::COMPONENT_ID);
        
        if(nullptr != achievementsComponent)
        {
            if(Blaze::ERR_OK == achievementsComponent->getAchievements(mRequest, mResponse))
                return ERR_OK;
        }
        
        return ERR_SYSTEM;
    }
};

GetAchievementsCommandStub* GetAchievementsCommandStub::create(Message *msg, Blaze::Achievements::GetAchievementsRequest* request, ArsonSlave *component)
{
    return BLAZE_NEW GetAchievementsCommand(msg, request, static_cast<ArsonSlaveImpl*>(component));
}

} //Arson
} //Blaze
