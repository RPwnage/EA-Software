
/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "util/utilslaveimpl.h"
// arson includes
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/schedulefibrecalltoutilsrpc_stub.h"

namespace Blaze
{
namespace Arson
{
class ScheduleFibreCallToUtilsRPCCommand : public ScheduleFibreCallToUtilsRPCCommandStub
{
public:
    ScheduleFibreCallToUtilsRPCCommand(Message* message, ArsonSlaveImpl* componentImpl)
        : ScheduleFibreCallToUtilsRPCCommandStub(message),
        mComponent(componentImpl)
    {
    }

    ~ScheduleFibreCallToUtilsRPCCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    ScheduleFibreCallToUtilsRPCCommandStub::Errors execute() override
    {
        Fiber::CreateParams params(Fiber::STACK_MEDIUM);
        gSelector->scheduleFiberCall(this, &ScheduleFibreCallToUtilsRPCCommand::utils_userSettingSave_fiber, "ScheduleFibreCallToUtilsRPCCommand::utils_userSettingSave_fiber", params);

        return ERR_OK;
    }   
    
    void utils_userSettingSave_fiber()
    {
        Blaze::Util::UserSettingsSaveRequest saveReq;
        Blaze::Util::UserSettingsLoadRequest loadReq;
        Blaze::Util::UserSettingsResponse loadRsp;
        Blaze::Util::DeleteUserSettingsRequest deleteReq;

        //saveReq.setKey("test_key");
        //saveReq.setData("test_data");
        //saveReq.setUserId(1000073565419); // nexus001

        Util::UtilSlaveImpl* utilComponent = nullptr;

        //Util::UtilSlave* utilComponent = nullptr;
        //utilComponent = static_cast<Util::UtilSlave*>(gController->getComponent(Util::UtilSlave::COMPONENT_ID));
        utilComponent = static_cast<Util::UtilSlaveImpl*>(gController->getComponent(Util::UtilSlaveImpl::COMPONENT_ID));
        //if(nullptr == utilComponent) TODO: Check for utilComponent
        
        //UserSession::pushSuperUserPrivilege();
        
        UserSession::SuperUserPermissionAutoPtr permission(true);

        if(UserSession::hasSuperUserPrivilege())
        {
            utilComponent->userSettingsSave(saveReq);
            utilComponent->userSettingsLoad(loadReq, loadRsp);
            utilComponent->deleteUserSettings(deleteReq);
        }
        

    }
    void utils_userSettingLoad_fiber()
    {
    }
    void utils_deleteUserSettings_fiber()
    {
    }
};



DEFINE_SCHEDULEFIBRECALLTOUTILSRPC_CREATE()

} //Arson
} //Blaze
