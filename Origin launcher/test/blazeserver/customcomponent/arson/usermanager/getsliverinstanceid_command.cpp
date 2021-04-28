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
#include "arson/rpc/arsonslave/getsliverinstanceid_stub.h"

#include "framework/slivers/slivermanager.h"

namespace Blaze
{
namespace Arson
{

class GetSliverInstanceIdCommand : public GetSliverInstanceIdCommandStub
{
public:
    GetSliverInstanceIdCommand(Message* message, Blaze::Arson::GetSliverInstanceIdRequest* request, ArsonSlaveImpl* componentImpl)
        : GetSliverInstanceIdCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetSliverInstanceIdCommand() override
    {
    }

private:
    GetSliverInstanceIdCommandStub::Errors execute() override
    {
        mResponse.setInstanceId(gSliverManager->getSliverInstanceId((Blaze::SliverNamespace)mRequest.getSliverNamespace(), GetSliverIdFromSliverKey(mRequest.getUserSessionId())));
        return ERR_OK;
    }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;
};

DEFINE_GETSLIVERINSTANCEID_CREATE()

} //Arson
} //Blaze
