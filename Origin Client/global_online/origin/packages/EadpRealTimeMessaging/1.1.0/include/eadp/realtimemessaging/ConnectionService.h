// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Service.h>
#include <eadp/foundation/Error.h>
#include <eadp/foundation/Callback.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Config.h>
#include <eadp/foundation/IdentityService.h>
#include <eadp/realtimemessaging/Config.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginV2Success.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginV3Response.h>
#include <com/ea/eadp/antelope/rtm/protocol/SessionNotificationV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/SessionResponseV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginErrorV1.h>

namespace eadp
{
namespace realtimemessaging
{

namespace Internal 
{
    // Forward declaration of internal class
    class RTMService;
}

/*!
 * @brief ConnectionService should be used to establish connection between client and server.
 *
 * Before making any RTM request connection should be established calling connect() function.
 */
class EADPREALTIMEMESSAGING_API ConnectionService : public eadp::foundation::IService
{
public:
    /*!
     * @brief Disconnect source containing the reason and a possible session message
     */
    class EADPREALTIMEMESSAGING_API DisconnectSource
    {
    public:

        /*!
         * @brief Disconnection reason for the RTM connection.
         */
        enum class Reason : uint32_t
        {
            FORCE_DISCONNECT,       //<! Disconnect received from the server because the user connected on another client. 
            RECONNECT_REQUEST,      //<! There was a reconnect request received from the server. Call `reconnect` with a fresh access token to maintain all Chat Channel and Presence subscriptions. 
            CONNECTION_ERROR,       //<! There was a connection error.
            MANUAL_DISCONNECT       //<! RTMService `disconnect` was called.
        };

        /*!
         * @brief Constructor for cases without a session message
         */
        DisconnectSource(Reason reason);

        /*!
         * @brief Constructor for a FORCE_DISCONNECT
         */
        DisconnectSource(Reason reason, const eadp::foundation::String& message);

        /*!
         * @brief Get the reason for the disconnect
         * @return the disconnect reason
         */
        Reason getReason() const; 

        /*!
         * @brief Get the session message associated with a FORCE_DISCONNECT. 
         * 
         * The string is empty if the reason is not a FORCE_DISCONNECT.
         *
         * @return the session message
         */
        const eadp::foundation::String& getSessionMessage() const; 

    private:

        /*!
         * @brief The reason for the disconnect
         */
        Reason m_reason; 

        /*!
         * @brief The session message. Empty for all reasons except for FORCE_DISCONNECT.
         */
        eadp::foundation::String m_sessionMessage; 
    };


    /*!
     * @brief ConnectCallback is triggered in response to the connect() and reconnect() call.
     * 
     * The loginResponse is null and error is not null when an error occurs.
     * The error is null if the connection was successful.
     * All values are null if the service is already connected. 
     * On error, you can retry connecting with forceDisconnect or call disconnect to clean up resources.
     *
     * See RealTimeMessagingError for RTM related error codes.
     */
    using ConnectCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> loginResponse, const eadp::foundation::ErrorPtr& error)>;

    /*!
     * @brief MultiConnectCallback is triggered in response to the multiple-sessions supported connect() and reconnect() overloads.
     *
     * The loginResponse is null and loginError is not null when an error occurs.
     * The error is null if the connection was successful.
     * All values are null if the service is already connected.
     * On error, you can retry connecting with forceDisconnect with a session key or call disconnect to clean up resources.
     * This callback is invoked if the session limit is reached with code RealTimeMessagingError::Code::SESSION_LIMIT_EXCEEDED.
     *
     * See RealTimeMessagingError for RTM related error codes.
     */
    using MultiConnectCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> loginResponse, eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> loginError, const eadp::foundation::ErrorPtr& error)>;
    
    /*!
     * @brief DisconnectCallback is triggered when the connection state of the ConnectionService disconnects.
     *
     * This callback will NOT be triggered by an explicit disconnect. That is, when disconnect() is called.
     * On Disconnect, you can either,
     * 1. reconnect() again in which case ChatService, PresenceService & Notification service callbacks and subscriptions will be maintained
     * 2. disconnect() to notify connectionService to cleanup all callbacks and subscriptions
     *
     * See RealTimeMessagingError for RTM related error codes.
     *
     * @param disconnectSource The source of the disconnect. 
     * @param error Error object for the disconnection. Passed only if Connection disconnected with reason as CONNECTION_ERROR
     */
    using DisconnectCallback = eadp::foundation::CallbackT<void(const DisconnectSource& disconnectSource, const eadp::foundation::ErrorPtr& error)>;

    /*!
     * @brief UserSessionChangeCallback is triggered for multi-sessions when a user logs in or logs out.
     *
     * @param notification The notification of the log in or log out.
     * @param error Error object
     */
    using UserSessionChangeCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> notification, const eadp::foundation::ErrorPtr& error)>;
    
    /*!
     * @brief GetSessionsCallback is triggered when the request to get all connected sessions is completed.
     *
     * @param sessionResponse The currently connected sessions.
     * @param error Error object
     */
    using GetSessionsCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1> sessionResponse, const eadp::foundation::ErrorPtr& error) > ;
    
    /*!
     * @brief CleanupSessionCallback is triggered when the session cleanup request is completed.
     *
     * @param error Error object
     */
    using CleanupSessionCallback = eadp::foundation::CallbackT<void(const eadp::foundation::ErrorPtr& error) >;

    /*!
     * @brief MultiConnectionRequest are required parameters for the multiple-sessions supported overloads of ConnectionService::connect() and ConnectionService::reconnect().
     *
     */
    struct EADPREALTIMEMESSAGING_API MultiConnectionRequest
    {
        explicit MultiConnectionRequest(const eadp::foundation::Allocator& allocator);
        MultiConnectionRequest(
            const eadp::foundation::String& accessToken,
            const eadp::foundation::String& clientVersion,
            MultiConnectCallback connectCallback,
            DisconnectCallback disconnectCallback,
            UserSessionChangeCallback userSessionChangeCallback,
            bool forceDisconnect,
            const eadp::foundation::String& forceDisconnectSessionKey,
            const eadp::foundation::String& reconnectSessionKey
        );

        eadp::foundation::String m_accessToken;                 /*!< Valid access token for the user account. */
        eadp::foundation::String m_clientVersion;               /*!< Client version identifier set by integrator. */
        MultiConnectCallback m_multiConnectCallback;            /*!< Triggered once the connection is established. */
        DisconnectCallback m_disconnectCallback;                /*!< Triggered when there is a disconnection. */
        UserSessionChangeCallback m_userSessionChangeCallback;  /*!< Triggered when another user session logs in or logs out. */
        bool m_forceDisconnect;                                 /*!< If True, disconnect the session associated with the sessionKey parameter. If false, the connect may fail with error code SESSION_LIMIT_EXCEEDED. */
        eadp::foundation::String m_forceDisconnectSessionKey;   /*!<  The sessionKey to disconnect if forceDisconnect is true. */
        eadp::foundation::String m_reconnectSessionKey;         /*!<  The sessionKey to reconnect to, if performing a reconnect. */
    };

    /*!
     * @brief Get the Connection service instance
     *
     * The service will be available when the Hub is live.
     *
     * @param hub The hub provides the allocator
     * @return The ConnectionService instance
     */
    static eadp::foundation::SharedPtr<ConnectionService> createService(eadp::foundation::SharedPtr<eadp::foundation::IHub> hub);

    /*!
     * @brief Initialize the connection service object.
     *
     * @param productId Unique identifier for a Game in the Origin system
     */
    virtual void initialize(eadp::foundation::String productId) = 0;

    /*!
     * @brief Starts a connection with backend server if a connection doesn't already exists
     *
     * @param accessToken Valid accessToken for the user account
     * @param clientVersion Client version identifier set by integrator
     * @param connectCallback Triggered once the connection is established
     * @param disconnectCallback Triggered when there is a disconnection
     * @param forceDisconnect If True, the user is disconnected out of all connected clients. Otherwise, the connect may fail with error code SESSION_ALREADY_EXIST.
     */
    virtual void connect(const eadp::foundation::String& accessToken, const eadp::foundation::String& clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback, bool forceDisconnect) = 0;

    /*!
     * @brief Starts a connection with the backend server using the multi-session API
     *
     * @param options Required options for connection
     */
    virtual void connect(eadp::foundation::UniquePtr<MultiConnectionRequest> options) = 0;

    /*!
     * @brief Is the Connection Service connected to the backend server? 
     */
    virtual bool isConnected() const = 0; 

    /*!
     * @brief Disconnects the current established connection with the server and cleans up the callbacks and subscriptions
     * This will invoke the disconnect callback with MANUAL_DISCONNECT
     */
    virtual void disconnect() = 0;

    /*!
     * @brief Reconnects when connection is dropped. All the Chat Channel and Presence subscriptions will be maintained if reconnection is successful.
     *
     * Should only be called when a RECONNECT_REQUEST is received.
     *
     * @param accessToken Valid accessToken for the user account
     * @param clientVersion Client version identifier set by integrator
     * @param connectCallback Triggered once the connection is reestablished
     * @param disconnectCallback Triggered when there is a disconnection
     */
    virtual void reconnect(const eadp::foundation::String& accessToken, const eadp::foundation::String& clientVersion, ConnectCallback connectCallback, DisconnectCallback disconnectCallback) = 0;

    /*!
     * @brief Reconnects a multiple-sessions supported session when connection is dropped. All the Chat Channel and Presence subscriptions will be maintained if reconnection is successful.
     * A session key must be provided representing the session to reconnect to.
     *
     * Should only be called when a RECONNECT_REQUEST is received. 
     * 
     * @param options Required options for connection
     */
    virtual void reconnect(eadp::foundation::UniquePtr<MultiConnectionRequest> options) = 0;

    /*!
     * @brief Gets the currently connected sessions of the currently connected user.
     *
     * Should only be called when connected.
     *
     * @param callback Callback to receive the connected sessions
     */
    virtual void getSessions(GetSessionsCallback callback) = 0;

    /*!
     * @brief Logout another session (of our current user) using its sessionKey
     *
     * Should only be called when connected.
     *
     * @param callback Callback to be triggered on successful cleanup or error.
     */
    virtual void cleanupSession(const eadp::foundation::String& sessionKey, CleanupSessionCallback callback) = 0;

    /*!
     * @brief For internal use only. Provides the RTMService object
     */
    virtual eadp::foundation::SharedPtr<Internal::RTMService> getRTMService() = 0;
    

    virtual ~ConnectionService() = default; 

};
}
}
