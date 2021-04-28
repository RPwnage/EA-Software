// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Error.h>
#include <eadp/foundation/Callback.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Config.h>
#include <eadp/foundation/IdentityService.h>
#include <eadp/realtimemessaging/Config.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginV2Success.h>

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
class EADPREALTIMEMESSAGING_API ConnectionService
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
     * The loginSuccess is null if an error occurred. 
     * The error is null if the connection was successful.
     * Both values are null if the service is already connected. 
     * On error, you can retry connecting or call disconnect to clean up resources.
     * This callback is invoked if a user is already connected on another client with code RealTimeMessagingError::Code::SESSION_ALREADY_EXIST.
     *
     * See RealTimeMessagingError for RTM related error codes.
     */
    using ConnectCallback = eadp::foundation::CallbackT<void(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> loginSuccess, const eadp::foundation::ErrorPtr& error)>;

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
     * @param connectCallback Triggered once the connection is established
     * @param disconnectCallback Triggered when there is a disconnection
     * @param forceDisconnect If True, the user is disconnected out of all connected clients. Otherwise, the connect may fail with error code SESSION_ALREADY_EXIST.
     */
    virtual void connect(eadp::foundation::String accessToken, ConnectCallback connectCallback, DisconnectCallback disconnectCallback, bool forceDisconnect) = 0;

    /*!
     * @brief Is the Connection Service connected to the backend server? 
     */
    virtual bool isConnected() const = 0; 

    /*!
     * @brief Disconnects the current established connection with the server and cleans up the callbacks and subscriptions
     * This will not invoke the disconnect callback. 
     */
    virtual void disconnect() = 0;

    /*!
     * @brief Reconnects when connection is dropped. All the Chat Channel and Presence subscriptions will be maintained if reconnection is successful.
     *
     * Should only be called when a RECONNET_REQUEST is received. 
     * 
     * @param accessToken Valid accessToken for the user account
     * @param connectCallback Triggered once the connection is reestablished
     * @param disconnectCallback Triggered when there is a disconnection
     */
    virtual void reconnect(eadp::foundation::String accessToken, ConnectCallback connectCallback, DisconnectCallback disconnectCallback) = 0;

    /*!
     * @brief For internal use only. Provides the RTMService object
     */
    virtual eadp::foundation::SharedPtr<Internal::RTMService> getRTMService() = 0;
    

    virtual ~ConnectionService() = default; 

};
}
}
