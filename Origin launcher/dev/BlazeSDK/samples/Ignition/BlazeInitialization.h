
#ifndef BLAZEINITIALIZATION_H
#define BLAZEINITIALIZATION_H

#include "Ignition/Ignition.h"
#include "Ignition/Messaging.h"

#include "EASTL/set.h"

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "Transmission.h"

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/shared/framework/locales.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/statsapi/lbapi.h"
#include "BlazeSDK/associationlists/associationlistapi.h"
#include "BlazeSDK/component/authenticationcomponent.h"

#define REPORT_ERROR(BlazeError, ErrorType) \
    if ((BlazeError != Blaze::ERR_OK) && (getUiDriver() != nullptr)) { \
        getUiDriver()->addDiagnostic_va( \
            __FILE__, EA_CURRENT_FUNCTION, __LINE__, 0, \
            Pyro::ErrorLevel::ERR, ErrorType, "Error: %s (%d)", \
            (gBlazeHub != nullptr ? gBlazeHub->getErrorName(BlazeError) : ""), BlazeError); \
        getUiDriver()->showMessage_va( \
            Pyro::ErrorLevel::ERR, ErrorType, "Error: %s (%d)", \
            (gBlazeHub != nullptr ? gBlazeHub->getErrorName(BlazeError) : ""), BlazeError); \
    }

#define REPORT_BLAZE_ERROR(BlazeError) \
    REPORT_ERROR(BlazeError, "BlazeSDK Error")

namespace Ignition
{

class BlazeInitialization
    : public BlazeHubUiBuilder,
      protected Blaze::BlazeNetworkAdapter::NetworkMeshAdapterUserListener
{
    friend class BlazeHubUiDriver;

    public:
        BlazeInitialization();
        virtual ~BlazeInitialization();

    protected:
        virtual void onInitialized( const Blaze::Mesh *mesh, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );
        virtual void onUninitialize( const Blaze::Mesh *mesh, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error );

    private:
        PYRO_ACTION(BlazeInitialization, StartupBlaze);
        PYRO_ACTION(BlazeInitialization, ShutdownBlaze);

        static void BlazeSdkLogFunction(const char8_t *pText, void *data);
        static int32_t DirtySockLogFunction(void *pParm, const char8_t *pText);
};


}

#endif
