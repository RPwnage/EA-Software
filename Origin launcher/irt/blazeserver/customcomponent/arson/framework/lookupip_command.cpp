/*************************************************************************************************/
/*!
    \file   lookupip_command.cpp

    @deprecated [geoip] BlazeGeoIp::lookupIp has been replaced with UserSessionManager::getGeoIpData
        -- which is already available via Blaze::GetUserGeoIpDataCommand

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

// global includes
#include "framework/blaze.h"
#include "framework/usersessions/usersessionmanager.h"

// arson includes
#include "arson/tdf/arson.h"
#include "arsonslaveimpl.h"
#include "arson/rpc/arsonslave/lookupip_stub.h"

namespace Blaze
{
namespace Arson
{
class LookUpIpCommand : public LookUpIpCommandStub
{
public:
    LookUpIpCommand(Message* message, LookUpIpRequest* request, ArsonSlaveImpl* componentImpl)
        : LookUpIpCommandStub(message, request),
        mComponent(componentImpl)
    {
    }

    ~LookUpIpCommand() override { }

private:
    // Not owned memory
    ArsonSlaveImpl* mComponent;

    LookUpIpCommandStub::Errors execute() override
    {
        GeoLocationData geoData;
        InetAddress ipAddr(mRequest.getUserIp(), 0, InetAddress::HOST);

        // Call to internal command 
        if (!gUserSessionManager->getGeoIpData(ipAddr, geoData))
        {
            return commandErrorFromBlazeError(Blaze::ERR_SYSTEM);
        }

        // Response set GeoLocation
        mResponse.setGeoLoc(geoData.getCity());

        return commandErrorFromBlazeError(Blaze::ERR_OK);
    }
};

DEFINE_LOOKUPIP_CREATE()


} //Arson
} //Blaze
