/*************************************************************************************************/
/*!
\file   fetchcontent_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class FetchContentCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "gpscontentcontroller/rpc/gpscontentcontrollerslave/fetchcontent_stub.h"

// global includes

#include "framework/connection/selector.h"
#include "framework/petitionablecontent/petitionablecontent.h"

// gpscontentcontroller includes
#include "gpscontentcontrollerslaveimpl.h"
#include "gpscontentcontroller/tdf/gpscontentcontrollertypes.h"

namespace Blaze
{
namespace GpsContentController
{

class FetchContentCommand : public FetchContentCommandStub
{
public:
    FetchContentCommand(Message* message, FetchContentRequest* request, GpsContentControllerSlaveImpl* componentImpl)
        : FetchContentCommandStub(message, request), mComponent(componentImpl)
    { }

    ~FetchContentCommand() override { }

/* Private methods *******************************************************************************/
private:
    FetchContentCommandStub::Errors execute() override
    {
        TRACE_LOG("[FetchContentCommand].execute() Fetch content id: " << mRequest.getContentId().toString().c_str());

        // only user authorized as GPS Administrator can access this RPC
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GPS_ADMINISTRATOR))
        {
            WARN_LOG("[FetchContentCommand].execute: User " << gCurrentUserSession->getBlazeId() << " attempted to fetch content "
                << mRequest.getContentId().toString().c_str() << ", no permission!" );
            return static_cast<Errors>(Blaze::ERR_AUTHORIZATION_REQUIRED);
        }

        eastl::string url;

        Collections::AttributeMap& attrMap = mResponse.getAttributeMap();
        BlazeRpcError result = 
            gPetitionableContentManager->fetchContent(mRequest.getContentId(), attrMap, url);
        if (result != Blaze::ERR_OK && result != Blaze::ERR_SYSTEM)
            result = Blaze::GPSCONTENTCONTROLLER_ERR_CONTENT_NOT_FOUND;

        mResponse.setExternalURL(url.c_str());

        if (result == Blaze::ERR_OK)
        {
            INFO_LOG("[FetchContentCommand].execute: User " << gCurrentUserSession->getBlazeId() << " fetched content " 
                << mRequest.getContentId().toString().c_str() << ", had permission." );
        }

        return commandErrorFromBlazeError(result);
    }

private:
    // Not owned memory.
    GpsContentControllerSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_FETCHCONTENT_CREATE()

} // GpsContentController
} // Blaze
