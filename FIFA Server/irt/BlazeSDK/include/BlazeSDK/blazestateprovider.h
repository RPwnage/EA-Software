/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_STATE_PROVIDER_H
#define BLAZE_STATE_PROVIDER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/component/framework/tdf/userdefines.h"



namespace Blaze
{
    /*! ****************************************************************************/
    /*! \class BlazeHubStateEventHandler

        \brief Interface for objects that want to be notified about Blaze SDK state
               changes.
    ********************************************************************************/
    
    class BLAZESDK_API BlazeStateEventHandler
    {
    public:
        /*! ****************************************************************************
            \name Base methods and members for all Blaze BlazeStateEventHandlers
         */

        virtual ~BlazeStateEventHandler() {}


        /*! ************************************************************************************************/
        /*! \brief The BlazeSDK has made a connection to the blaze server.  This is user independent, as
            all local users share the same connection.
        ***************************************************************************************************/
        virtual void onConnected() {};

        /*! ************************************************************************************************/
        /*! \brief The BlazeSDK has lost connection to the blaze server.  This is user independent, as
            all local users share the same connection.
            \param errorCode - the error describing why the disconnect occurred.
        ***************************************************************************************************/
        virtual void onDisconnected(BlazeError errorCode) {};
        
        /*! ************************************************************************************************/
        /*! \brief Notification that a local user has been authenticated against the blaze server.  
            The provided user index indicates which local user authenticated.  If multiple local users
            authenticate, this will trigger once per user.
        
            \param[in] userIndex - the user index of the local user that authenticated.
        ***************************************************************************************************/
        virtual void onAuthenticated(uint32_t userIndex) {};

        /*! ************************************************************************************************/
        /*! \brief Notification that a local user has lost authentication (logged out) of the blaze server
            The provided user index indicates which local user lost authentication. If multiple local users
            lose authentication, this will trigger once per user.

            \param[in] userIndex - the user index of the local user that authenticated.
        ***************************************************************************************************/
        virtual void onDeAuthenticated(uint32_t userIndex) {};

        /*! ************************************************************************************************/
        /*! \brief Notification that a local user has been forcefully lost authentication (logged out)
            The provided user index indicates which local user lost authentication.
            The provided forced logout type indicates the reason of the logout.
            If multiple local users lose authentication, this will trigger once per user.

            \param[in] userIndex - the user index of the local user that authenticated.
            \param[in] forcedLogoutType - the type of forced logout for the user
        ***************************************************************************************************/
        virtual void onForcedDeAuthenticated(uint32_t userIndex, UserSessionForcedLogoutType forcedLogoutType) {};

    };


    class BLAZESDK_API ConnectionManagerStateListener {
    public:
        virtual ~ConnectionManagerStateListener() {}

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notification from ConnectionManager that the connection
            to the blaze server has been established.  Implies netconn connection as well
            as first ping response.
        ***************************************************************************************************/
        virtual void onConnected() = 0;
        
        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notification from ConnectionManager that the connection
            to the blaze server has been lost.
            \param error - the error code reason why the disconnect occurred.
        ***************************************************************************************************/
        virtual void onDisconnected(BlazeError error) = 0;
    };

    class BLAZESDK_API UserManagerStateListener {
    public:
        virtual ~UserManagerStateListener() {}

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notifications that a local user has authenticated.
        
            \param[in] userIndex - the index of the local user who has authenticated
        ***************************************************************************************************/
        virtual void onLocalUserAuthenticated(uint32_t userIndex) = 0;

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notifications that a local user has deauthenticated.
        
            \param[in] userIndex - the index of the local user who has de authenticated
        ***************************************************************************************************/
        virtual void onLocalUserDeAuthenticated(uint32_t userIndex) = 0;

        virtual void onLocalUserForcedDeAuthenticated(uint32_t userIndex, UserSessionForcedLogoutType forcedLogoutType) {};
    };

#ifdef ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING

    class BLAZESDK_API BlazeSenderStateListener {
    public:
        virtual ~BlazeSenderStateListener() {}

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle basic data about RPC requests sent.

            \param[in] dataSize - the size of data the RPC command sent
            \param[in] msgId - the message id of the request sent
            \param[in] componentId - the id of the component that the request was sent to
            \param[in] commandId - the id of the RPC command that was sent
        ***************************************************************************************************/
        virtual void onRequest(size_t dataSize, uint32_t msgId, uint16_t componentId, uint16_t commandId) = 0;

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle basic data about RPC responses received.

            \param[in] dataSize - the size of the response received
            \param[in] msgId - the message id of the response received
            \param[in] componentId - the id of the component that sent the response
            \param[in] commandId - the id of the RPC command associated with the response
            \param[in] errorCode - the error code returned by the RPC command associated with the response
        ***************************************************************************************************/
        virtual void onResponse(size_t dataSize, uint32_t msgId, uint16_t componentId, uint16_t commandId, BlazeError errorCode) = 0;


        /*! ************************************************************************************************/
        /*! \brief Listener method to handle basic data about notifications received.
        
            \param[in] dataSize - the size of the notification received
            \param[in] msgId - the message id of the notification received
            \param[in] componentId - the id of the component that sent the notification
            \param[in] commandId - the id of the notification
        ***************************************************************************************************/
        virtual void onNotification(size_t dataSize, uint32_t msgId, uint16_t componentId, uint16_t commandId) = 0;

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle basic data about unexpected/unknown RPC responses received.
        
            \param[in] dataSize - the size of the response received
            \param[in] msgId - the message id of the response received
            \param[in] componentId - the id of the component that sent the response
            \param[in] commandId - the id of the RPC command associated with the response
            \param[in] errorCode - the error code returned by the RPC command associated with the response
        ***************************************************************************************************/
        virtual void onBadResponse(size_t dataSize, uint32_t msgId, uint16_t componentId, uint16_t commandId, BlazeError errorCode) = 0;
    };
#endif // ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING
}
#endif // BLAZE_STATE_PROVIDER_H
