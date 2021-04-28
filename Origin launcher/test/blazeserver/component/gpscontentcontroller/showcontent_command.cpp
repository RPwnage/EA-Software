/*************************************************************************************************/
/*!
\file   showcontent_command.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class ShowContentCommand

Pokes the slave.

\note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "gpscontentcontroller/rpc/gpscontentcontrollerslave/showcontent_stub.h"

// global includes

#include "framework/petitionablecontent/petitionablecontent.h"

// gpscontentcontroller includes
#include "gpscontentcontrollerslaveimpl.h"
#include "gpscontentcontroller/tdf/gpscontentcontrollertypes.h"

namespace Blaze
{
namespace GpsContentController
{

class ShowContentCommand : public ShowContentCommandStub
{
public:
    ShowContentCommand(Message* message, ShowContentRequest* request, GpsContentControllerSlaveImpl* componentImpl)
        : ShowContentCommandStub(message, request), mComponent(componentImpl)
    { }

    ~ShowContentCommand() override { }

    /* Private methods *******************************************************************************/
private:

    ShowContentCommandStub::Errors execute() override
    {
        TRACE_LOG("[ShowContentCommand].execute() Show content id: " << mRequest.getContentId().toString().c_str());

        // only user authorized as GPS Administrator can access this RPC
        if (!UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_GPS_ADMINISTRATOR))
        {
            WARN_LOG("[ShowContentCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                << " attempted to " << (mRequest.getShow() ? "show" : "hide" )<< " content " << mRequest.getContentId().toString().c_str() << ", no permission!" );
            return static_cast<Errors>(Blaze::ERR_AUTHORIZATION_REQUIRED);
        }

        BlazeRpcError result = 
            gPetitionableContentManager->showContent(mRequest.getContentId(), mRequest.getShow());
        if (result != Blaze::ERR_OK && result != Blaze::ERR_SYSTEM)
            result = Blaze::GPSCONTENTCONTROLLER_ERR_CONTENT_NOT_FOUND;

        if (result == Blaze::ERR_OK)
        {
            INFO_LOG("[ShowContentCommand].execute: User " << gCurrentUserSession->getBlazeId() 
                <<  (mRequest.getShow() ? " showed" : " hided") << " content " << mRequest.getContentId().toString().c_str() << ", had permission." );
        }

        return commandErrorFromBlazeError(result);
    }  /*lint !e1961 - Don't make the function to const because its parent class is generated from template file */

private:
    // Not owned memory.
    GpsContentControllerSlaveImpl* mComponent;

};
// static factory method impl
DEFINE_SHOWCONTENT_CREATE()

} // GpsContentController
} // Blaze
