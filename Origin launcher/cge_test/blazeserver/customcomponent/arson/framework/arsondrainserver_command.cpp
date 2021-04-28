
/*** Include Files *******************************************************************************/

// framework includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/arsondrainserver_stub.h"

namespace Blaze
{
namespace Arson
{
class ArsonDrainServerCommand : public ArsonDrainServerCommandStub
{
public:
    ArsonDrainServerCommand(Message* message, Blaze::Arson::ArsonDrainRequest *req, ArsonSlaveImpl* componentImpl)
        : ArsonDrainServerCommandStub(message, req),
        mComponent(componentImpl)
    {
    }

    ~ArsonDrainServerCommand() override { }

    // Not owned memory
    ArsonSlaveImpl* mComponent;

    ArsonDrainServerCommandStub::Errors execute() override
    {
        BLAZE_INFO_LOG(Log::USER, "ArsonSlave executing Draining command");
        
        Blaze::DrainRequest req;

        // Set the Drain Target type
        switch(mRequest.getTarget())
        {
            case Blaze::Arson::ArsonDrainRequest::DRAIN_TARGET_ALL:
                req.setTarget(Blaze::DrainRequest::DRAIN_TARGET_ALL);
                break;
            case Blaze::Arson::ArsonDrainRequest::DRAIN_TARGET_LIST:
                req.setTarget(Blaze::DrainRequest::DRAIN_TARGET_LIST);
                break;
            case Blaze::Arson::ArsonDrainRequest::DRAIN_TARGET_LOCAL:
                req.setTarget(Blaze::DrainRequest::DRAIN_TARGET_LOCAL);
                break;
            default:
                req.setTarget(Blaze::DrainRequest::DRAIN_TARGET_LOCAL);
                break;
        }

        // Set mInstanceIdList
        Blaze::Arson::ArsonDrainRequest::InstanceIdList::iterator InstanceIdListItr = mRequest.getInstanceIdList().begin();
        for( ; InstanceIdListItr != mRequest.getInstanceIdList().end(); ++InstanceIdListItr)
        {
            req.getInstanceIdList().push_back(*InstanceIdListItr);
        } 

        // Set mTimeout
        req.setTimeout(mRequest.getTimeout());

        // Set ThresholdByClientType
        mRequest.getLocalUserThresholdByClientType().copyInto(req.getLocalUserThresholdByClientType());
        
        BlazeRpcError err = gController->drain(req);

        if(Blaze::ERR_OK == err)
            return ERR_OK;
        else
            return ERR_SYSTEM;
    }
};

DEFINE_ARSONDRAINSERVER_CREATE()

} //Arson
} //Blaze
