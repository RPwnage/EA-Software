

/*************************************************************************************************/
/*!
    \file   gpscontentcontrollerslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GPSCONTENTCONTROLLER_SLAVEIMPL_H
#define BLAZE_GPSCONTENTCONTROLLER_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "gpscontentcontroller/rpc/gpscontentcontrollerslave_stub.h"
#include "framework/replication/replicationcallback.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/protocol/outboundhttpresult.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace GpsContentController
{

class GpsContentControllerSlaveImpl : public GpsContentControllerSlaveStub
{
public:
    GpsContentControllerSlaveImpl() {}
    ~GpsContentControllerSlaveImpl() override {}

    BlazeRpcError DEFINE_ASYNC_RET(filePetitionHttpRequest(const FilePetitionRequest &req, FilePetitionResponse &resp) const);

private:
    // configuration parameters
    eastl::string mRequestorId;

    bool onConfigure() override;
    bool onReconfigure() override;
    bool onValidateConfig(GpsContentControllerConfig& config, const GpsContentControllerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
    const char8_t* getGPSPlatformStringFromClientPlatformType(const ClientPlatformType platformType) const;
};

} // GpsContentController
} // Blaze

#endif // BLAZE_EXAMPLE_SLAVEIMPL_H

