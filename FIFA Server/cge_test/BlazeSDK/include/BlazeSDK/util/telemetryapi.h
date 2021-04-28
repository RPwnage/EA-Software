/*************************************************************************************************/
/*!
\file telemetryapi.h


\attention
(c) Electronic Arts. All Rights Reserved.

\warning This code is currently in preview stage. It doesn't do much and may probably crash. <b>Use at your own risk. </b>
 With that being said, TelemetryAPI can currently setup/initialize receiving telemetry for a PC.

*************************************************************************************************/

#ifndef BLAZE_TELEMETRYAPI_H
#define BLAZE_TELEMETRYAPI_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/api.h"
#include "BlazeSDK/blazetypes.h"
#include "BlazeSDK/idler.h"

#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/component/utilcomponent.h"

#ifdef USE_TELEMETRY_SDK
// VolcanoSDK includes
// TelemetrySDK includes
#include "TelemetrySDK/telemetryapi.h"
#include "TelemetrySDK/telemetrypinapi.h"
#if TELEMETRY_VERSION_MAJOR >= 15
#include "PinTaxonomySDK/telemetrypinentity.h"
#endif

namespace Telemetry
{
#ifdef USE_BLAZE_TELEMERY_SERVER
    class TelemetryApiRefT;
#else
    struct TelemetryApiRefT;
#endif
}
using namespace Telemetry;
#else
// DirtySDK telemetry
#include <telemetryapi.h>
struct TelemetryApiRefT;
#endif

namespace Blaze
{

class Fire2Connection;

namespace Telemetry
{

/*! **********************************************************************************************/
/*! 
    \class TelemetryAPI

    \brief
    TelemetryAPI is a wrapper for DirtySDK's TelemetryApi.

    \details
    TelemetryAPI is a wrapper for DirtySDK's TelemetryApi.

**************************************************************************************************/
class BLAZESDK_API TelemetryAPI : public MultiAPI, private Idler
{
private:
    NON_COPYABLE(TelemetryAPI);

public:
    //! the buffer can be one of these types
    typedef enum BufferTypeE
    {
        BUFFERTYPE_FILLANDSTOP = 0,     //!< original implementation - fill and then stop taking events
        BUFFERTYPE_CIRCULAROVERWRITE    //!< wrap around and overwrite old events as necessary
    } BufferTypeE;

    /*! **********************************************************************************************/
    /*! \brief  Parameters for TelemetryAPI creation
    see TelemetryAPI::createAPI.
    **************************************************************************************************/
    struct TelemetryApiParams
    {
        //! \brief Default struct constructor - no members initialized.
        TelemetryApiParams()
        { }

        //! \brief Convenience constructor; initializes the struct's fields
        TelemetryApiParams(uint32_t numEvents, BufferTypeE bufferType)
            : mNumEvents(numEvents), mBufferType(bufferType), mIsVersion3(0), mBuffer3Size(0)
        { }

        //! \brief Convenience constructor for version 3 telemetry
        TelemetryApiParams(uint32_t numEvents, BufferTypeE bufferType, uint8_t isVersion3)
            : mNumEvents(numEvents), mBufferType(bufferType), mIsVersion3(isVersion3), mBuffer3Size(0)
        { }

        //! \brief Convenience constructor for version 3 telemetry
        TelemetryApiParams(uint32_t numEvents, BufferTypeE bufferType, uint8_t isVersion3, uint32_t buffer3Size)
            : mNumEvents(numEvents), mBufferType(bufferType), mIsVersion3(isVersion3), mBuffer3Size(buffer3Size)
        { }

        uint32_t mNumEvents; //!< number of events to use (0=use default)
        BufferTypeE mBufferType; //!< type of buffer to use
        uint8_t mIsVersion3;   //!< use version 3 API
        uint32_t mBuffer3Size; //!< size of telemetry 3 event buffer
    };

    /*! **********************************************************************************************/
    /*! \name API methods
    **************************************************************************************************/

    /*! **********************************************************************************************/
    /*! \brief Creates the TelemetryAPI

        \param hub       The hub to create the API on.
        \param params    Specify number of events to use, buffer type, etc.
        \param allocator Pointer to the optional allocator to be used for the component; will use default if not specified.
    **************************************************************************************************/
    static void createAPI(BlazeHub &hub, const TelemetryApiParams& params, EA::Allocator::ICoreAllocator* allocator = nullptr);

    /*! **********************************************************************************************/
    /*! \brief Logs any initialization parameters for this API to the log. Required override from API.
    **************************************************************************************************/
    virtual void logStartupParameters() const;

    /*! **********************************************************************************************/
    /*! \brief Get DirtySDK's telemetry module for use with DirtySDK's telemetry API directly

        \returns A pointer to DirtySDK's telemetry module
    **************************************************************************************************/
    TelemetryApiRefT* getTelemetryApiRefT() const
    {
        return mTelemetryRef;
    }

    /*! **********************************************************************************************/
    /*! \brief Initialize the passed TelemetryApiRefT pointer based on the response from Blaze

        \param pRef     A pointer to a TelemetryApiRefT object
        \param response The response from the getTelemetryServer RPC in the util component
        \param error    The error returned by the getTelemetryServer RPC in the util component
        \param pUser    The current local user; nullptr if initializing an anonymous telemetry handle

        \returns true if initialization succeeded, false if there was an error returned by Blaze
    **************************************************************************************************/
    static bool initAPI(TelemetryApiRefT *pRef, 
        const Util::GetTelemetryServerResponse* response, 
        BlazeError error, 
        Blaze::UserManager::LocalUser *pUser);

    /*! **********************************************************************************************/
    /*! \brief Returns the Pin Telemetry host config

    \returns the pointer to the pin telemetry host config
    **************************************************************************************************/
    PinHostConfigT* getTelemetryPinHostConfig() const
    {
        return &mPinConfig;
    }

protected:

    /*! ************************************************************************************************/
    /*! \brief Notification that a local user has been authenticated against the blaze server.  
        The provided user index indicates which local user authenticated.  If multiple local users
        authenticate, this will trigger once per user.
    
        \param[in] userIndex - the user index of the local user that authenticated.
    ***************************************************************************************************/
    virtual void onAuthenticated(uint32_t userIndex);

    /*! ************************************************************************************************/
    /*! \brief Notification that a local user has lost authentication (logged out) of the blaze server
        The provided user index indicates which local user  lost authentication.  If multiple local users
        loose authentication, this will trigger once per user.

        \param[in] userIndex - the user index of the local user that authenticated.
    ***************************************************************************************************/
    virtual void onDeAuthenticated(uint32_t userIndex);

    /*! **********************************************************************************************/
    /*! \brief Create DirtySDK's telemetry module
    **************************************************************************************************/
    void create();

    /*! **********************************************************************************************/
    /*! \brief Initialize/configure DirtySDK's telemetry module
    **************************************************************************************************/
    void configure();

    /*! **********************************************************************************************/
    /*! \brief Destroy DirtySDK's telemetry module
    **************************************************************************************************/
    void destroy();

    static bool connect(TelemetryApiRefT *pRef);
    static void disconnect(TelemetryApiRefT *pRef);

    /*! **********************************************************************************************/
    /*! \brief internal callback for getTelemetryServer RPC (see initialize)
    **************************************************************************************************/
    void onGetTelemetryServer(const Util::GetTelemetryServerResponse* response, BlazeError error, JobId id);

    /*! **********************************************************************************************/
    /*! \brief Idler interface
    **************************************************************************************************/
    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);
    void cancelIdle(bool timedout);
    void doFinishIdle();

private:
    /*! **********************************************************************************************/
    /*! \brief Constructor.

        Private as all construction should happen via the factory method.
    **************************************************************************************************/
    TelemetryAPI(BlazeHub& blazeHub, const TelemetryApiParams& params, uint32_t userIndex);
    virtual ~TelemetryAPI();
    
    /*! **********************************************************************************************/
    /*! \brief Internal function used to queue a blaze sdk telemetry event for when a disconnect happens
                This function adds additional information to the supplied event, including last ping time,
                last idle time, and QOS data.

        \param event      The event to queue 
    **************************************************************************************************/
    void queueBlazeSDKTelemetry(TelemetryApiEvent3T& event);
    
    /*! **********************************************************************************************/
    /*! \brief Determine if the client should log telemetry events when the client disconnects
                from the blaze server or game server/peer

        \returns true if disconnect telemetry logging is enabled, false otherwise.
    **************************************************************************************************/
    bool isDisconnectTelemetryEnabled() const { return mEnableDisconnectTelemetry; }

private:
    BlazeHub& mBlazeHub;
    static ::Telemetry::PinHostConfigT mPinConfig; //!< pin telemetry config
    
    //these classes make use of queueBlazeSDKTelemetry
    friend class Blaze::GameManager::GameManagerAPI;
    friend class Blaze::Fire2Connection;
    
    TelemetryApiParams mCreateRefParams;
    TelemetryApiRefT* mTelemetryRef; //!< DirtySDK's telemetry module state
#if TELEMETRY_VERSION_MAJOR >= 15
    PinPlayerEntity *mPinEntity;     //!< PinTaxonomySDK's entity
#endif
    Blaze::UserManager::UserManager* mUserManager;
    Blaze::Util::UtilComponent* mUtilComponent;
    bool mEnableDisconnectTelemetry;

    JobId mCancelIdleJobId;
};

}// Telemetry

}// Blaze

#endif
