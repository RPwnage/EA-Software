/*************************************************************************************************/
/*!
    \file   getpsubyclienttype_command.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/component/componentmanager.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/getpsubyclienttype_stub.h"

namespace Blaze
{
namespace Arson
{
class GetPSUByClientTypeCommand : public GetPSUByClientTypeCommandStub
{
public:
    GetPSUByClientTypeCommand(
        Message* message, Blaze::Arson::GetMetricsByGeoIPDataRequest* request, ArsonSlaveImpl* componentImpl)
        : GetPSUByClientTypeCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~GetPSUByClientTypeCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    GetPSUByClientTypeCommandStub::Errors execute() override
    {
        Blaze::GetMetricsByGeoIPDataRequest getMetricsByGeoIPDataRequest;
        getMetricsByGeoIPDataRequest.setIncludeISP(mRequest.getIncludeISP());
        Blaze::GetPSUResponse getPSUResponse;

        BlazeRpcError err = gController->getPSUByClientType(getMetricsByGeoIPDataRequest, getPSUResponse);

        getPSUResponse.getPSUMap().copyInto(mResponse.getPSUMap());

        //return back with the resulting rpc error code
        return commandErrorFromBlazeError(err);
    }
};

DEFINE_GETPSUBYCLIENTTYPE_CREATE()
} //Arson
} //Blaze
