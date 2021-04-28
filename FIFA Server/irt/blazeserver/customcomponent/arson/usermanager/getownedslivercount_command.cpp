/*************************************************************************************************/
/*!
    \file   checkconnectivity_command.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getownedslivercount_stub.h"

#include "framework/slivers/slivermanager.h"

namespace Blaze
{
namespace Arson
{

class GetOwnedSliverCountCommand : public GetOwnedSliverCountCommandStub
{
public:
    GetOwnedSliverCountCommand(Message* message, Blaze::Arson::GetOwnedSliverCountRequest* request, ArsonSlaveImpl* componentImpl)
        : GetOwnedSliverCountCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetOwnedSliverCountCommand() override
    {
    }

private:
    GetOwnedSliverCountCommandStub::Errors execute() override
    {
        mResponse.setCount(gSliverManager->getOwnedSliverCount((Blaze::SliverNamespace)mRequest.getSliverNamespace(), (InstanceId)mRequest.getInstanceId()));
        return ERR_OK;
    }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETOWNEDSLIVERCOUNT_CREATE()

} //Arson
} //Blaze
