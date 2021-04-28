// Copyright(C) 2019-2020 Electronic Arts, Inc. All rights reserved.

#include <eadp/foundation/Callback.h>
#include <eadp/foundation/ClientContext.h>
#include <eadp/foundation/Config.h>
#include <eadp/foundation/DirectorService.h>
#include <eadp/foundation/Error.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/IdentityService.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/foundation/PersistenceService.h>
#include <eadp/realtimemessaging/ConnectionService.h>
#include <eadp/realtimemessaging/ChatService.h>
#include <eadp/realtimemessaging/NotificationService.h>
#include <eadp/realtimemessaging/PresenceService.h>
#include <ea/origin/OriginService.h>
#include <eadp/authentication/ID20AuthService.h>

#if !EADPSDK_USE_DEFAULT_ALLOCATOR
#include <MemoryMan/MemoryMan.inl>          // Implements a global PPMalloc GeneralAllocator and operator new/delete.
#include <MemoryMan/CoreAllocator.inl>      // Implements a global ICoreAllocator.
#endif
#include <eathread/eathread.h>
#include <iostream>

using namespace eadp::foundation;

// A quick demo of how to integrate with EADP SDK Real Time Messaging. The demo has some things that are hard coded for an easier demonstration.
constexpr auto USER_INDEX = IIdentityService::kDefaultUserIndex;              
constexpr auto CLIENT_ID = "EADP_SDK_DEMO_PC_CLIENT";
constexpr auto CLIENT_SECRET = "olIZyCDMBPE7CtaCjvfzXvFkK5I0a4rAWtCDck6AiYpDuJ7zrNZrdHvra1HRyjUstmnCBCRyqd5wP8Pp";
constexpr auto REDIRECT_URL = "http://127.0.0.1/success";// User index to login
constexpr auto PRODUCT_ID = CLIENT_ID;                                                                                               // Test product id to use for API calls
constexpr auto USER_ID = "1000540706024";                                                                                            // User id to subscribe to for presence updates
constexpr auto TITLE = "Origin SDK Test App";
constexpr auto CONTENT_ID = "71314";
constexpr auto MULTIPLAYER_ID = "qatest_sdk_multiplayer";

constexpr auto CLIENT_VERSION = "EADPSDK-RTM-demo"; // client version for ConnectionService::connect() and reconnect() calls

// Defines a callback group for use when constructing callbacks to use with the API. A callback group allows 
// the segmentation of callbacks into groups to control when callbacks are invoked.  This example does not
// demonstrate using multiple group IDs.
static constexpr int RTM_GROUP_ID = 13377;

//////////////////////////////////////////////////////////////////////////
///// FoundationIntegrator 
//////////////////////////////////////////////////////////////////////////

// A class to demonstrate integration with the Foundation module.  Specifically, it creates a Hub and
// uses some other foundation services to initialize the environment. It also provides an accessor for the
// hub and a utility method update() to pump idle and invoke callbacks.
class FoundationIntegrator
{
public:
    FoundationIntegrator()
    {
        // Hubconfig is used to provide configuration information like an allocator or a clientID for DirtySDK.
        HubConfig hubConfig;
#if !EADPSDK_USE_STD_STL
        hubConfig.allocator = EA::Allocator::ICoreAllocator::GetDefaultAllocator();
#endif
        hubConfig.clientId = CLIENT_ID;

        // Creates the hub for our integration.  The hub is the center of the foundation layer and provides access
        // to support services and controls the invocation of jobs and delivery of responses in a thread-safe manner.
        m_hub = Hub::create(hubConfig);

        // Set the logging threshold to only write fatal log messages to keep the log output clean.  This value
        // could be set to VERBOSE to provide logging information to the EADP SDK team to help diagnose issues
        // found during integration.
        m_hub->getLoggingService()->setThresholdLogLevel(eadp::realtimemessaging::kPackageName, ILoggingService::LogLevel::VERBOSE);
        m_hub->getLoggingService()->setThresholdLogLevel(eadp::authentication::kPackageName, ILoggingService::LogLevel::FATAL);
        m_hub->getLoggingService()->setThresholdLogLevel(eadp::foundation::kPackageName, ILoggingService::LogLevel::FATAL);

        // Initialize the persistence service, which is required for login.
        auto persistence = PersistenceService::getService(m_hub.get());
        persistence->initialize(nullptr);
    }

    SharedPtr<IHub> getHub() const { return m_hub; }

    void update()
    {
        // In the game loop, you should periodically invoke idle to process outstanding jobs on the
        // hub, and then invokeCallbacks to get any pending responses back. We use the simple version of
        // invoke callbacks in this demo, which will invoke pending callbacks for all callback groups.
        m_hub->idle();
        m_hub->invokeCallbacks();
    }

private:
    SharedPtr<IHub> m_hub;
};


//////////////////////////////////////////////////////////////////////////
///// RTMServiceIntegrator 
//////////////////////////////////////////////////////////////////////////

// A class to demonstrate integration with the RTM services. It provides a methods for 
// setting the user presence, subscribing to other user presences, sending a chat message, 
// and subscribing to notifications 
class RTMServiceIntegrator
{
public:
    RTMServiceIntegrator(const FoundationIntegrator& foundation)
        : m_connectionService(eadp::realtimemessaging::ConnectionService::createService(foundation.getHub()))
        , m_chatService(eadp::realtimemessaging::ChatService::createService(foundation.getHub()))
        , m_presenceService(eadp::realtimemessaging::PresenceService::createService(foundation.getHub()))
        , m_notificationService(eadp::realtimemessaging::NotificationService::createService(foundation.getHub()))
        , m_id20Service(eadp::authentication::ID20AuthService::createService(foundation.getHub()))
        , m_originService(ea::origin::IOriginService::createService(foundation.getHub()))
        , m_hub(foundation.getHub())
        , m_allocator(foundation.getHub()->getAllocator())
        , m_persistenceObject(m_allocator.makeShared<Callback::Persistence>())
        , m_isDone(false)
    {
        // Initialize the authentication service 
        auto error = m_id20Service->initialize(CLIENT_ID, CLIENT_SECRET, m_originService);
        if (error)
        {
            // Failed to initialize the service. End the demo.
            m_isDone = true;
            
        }

        m_id20Service->setRedirectUrl(REDIRECT_URL);

        // Only called once during initialization
        auto connectCallback = [this](const ErrorPtr& error)
        {
            if (error)
            {
                std::cout << "[OriginService] Failed to connect to the Origin client." << std::endl;
                std::cout << "[OriginService] Code:    " << error->getCode() << std::endl;
                std::cout << "[OriginService] Reason:  " << error->getReason().c_str() << std::endl;
                m_isDone = true;
            }
            else
            {
                std::cout << "[OriginService] Successfully connected to the Origin client." << std::endl;
                login();
            }
        };

        // May be called at any moment
        auto disconnectCallback = [this](const ErrorPtr& error)
        {
            std::cout << "[OriginService] Disconnected from the Origin client." << std::endl;
            if (error)
            {
                std::cout << "[OriginService] Code:    " << error->getCode() << std::endl;
                std::cout << "[OriginService] Reason:  " << error->getReason().c_str() << std::endl;
            }
            m_isDone = true;
        };

        auto& allocator = foundation.getHub()->getAllocator();
        m_originService->initialize(allocator.make<String>(TITLE),
            allocator.make<String>(CONTENT_ID),
            allocator.make<String>(MULTIPLAYER_ID),
            ea::origin::IOriginService::ConnectCallback(connectCallback, m_persistenceObject, RTM_GROUP_ID),
            ea::origin::IOriginService::ClientDisconnectCallback(disconnectCallback, m_persistenceObject, RTM_GROUP_ID));
    }

    // Use ID2.0 login to get the required access token.
    void login();

    // Service Initializations 
    void initializeRTM(bool useV2); 
    void initializeChat(); 
    void initializePresence(); 
    void initializeNotification(); 

    // Subscribe to a user presence
    void subscribePresence(); 
    
    // List the connected sessions
    void getConnectedSessions();

    // disconnect
    void disconnectRTM();

    void onConnect(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> loginResponse, eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> loginError, const ErrorPtr& error);
    void onDisconnect(const eadp::realtimemessaging::ConnectionService::DisconnectSource& disconnectSource, const ErrorPtr& error); 
    void onUserSessionChange(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> notification, const ErrorPtr& error);

    bool isDone() { return m_isDone; }

private:
    /*!
     * @brief The Connection Service is used by all RTM services to communicate with the RTM backend
     */
    SharedPtr<eadp::realtimemessaging::ConnectionService> m_connectionService; 

    /*!
     * @brief The Chat Service is used for all RTM Chat APIs
     */
    SharedPtr<eadp::realtimemessaging::ChatService> m_chatService; 

    /*!
     * @brief The Presence Service is used for subscribing, unsubscribing, and updating user presences
     */
    SharedPtr<eadp::realtimemessaging::PresenceService> m_presenceService; 

    /*!
     * @brief The Notification Service is used for subscribing to notifications
     */
    SharedPtr<eadp::realtimemessaging::NotificationService> m_notificationService; 

    /*!
     * @brief ID 2.0 Auth Service is used for retrieving an access token. 
     *    This is used for this demo. Other forms of authentication can work with the RTM module as long as 
     *    a valid access token is provided to the Connection Service. 
     */
    SharedPtr<eadp::authentication::ID20AuthService> m_id20Service;
    SharedPtr<ea::origin::IOriginService> m_originService;
    SharedPtr<IHub> m_hub;
    Allocator m_allocator;
    Callback::PersistencePtr m_persistenceObject;
    bool m_isDone;
};

//////////////////////////////////////////////////////////////////////////
///// RTMServiceIntegrator Implementation 
//////////////////////////////////////////////////////////////////////////

void RTMServiceIntegrator::login()
{
    // Setup Login request
    auto allocator = m_hub->getAllocator();
    auto request = allocator.makeUnique<eadp::authentication::ID20AuthService::LoginRequest>(allocator);
    request->setUserIndex(USER_INDEX);
    request->setScope("");
    request->setUserInfo(nullptr);
    request->setGuestMode(false);

    // Define login callback. 
    auto loginCallback = [this](UniquePtr<eadp::authentication::ID20AuthService::Response> response, const ErrorPtr error)
    {
        if (response)
        {
            if (response->getSuccess())
            {
                std::cout << "[ID20AuthService] Login succeeded" << std::endl;
            }
            else
            {
                std::cout << "[ID20AuthService] Login failed" << std::endl;
                m_isDone = true;
            }
        }
        else if (error)
        {
            std::cout << "[ID20AuthService] Login failed" << std::endl;
            std::cout << "[ID20AuthService] Code:    " << error->getCode() << std::endl;
            std::cout << "[ID20AuthService] Reason:  " << error->getReason().c_str() << std::endl;
            m_isDone = true;
        }

        initializeRTM(false);
    };

    // Initiate the Login
    m_id20Service->login(EADPSDK_MOVE(request), eadp::authentication::ID20AuthService::Callback(loginCallback, m_persistenceObject, RTM_GROUP_ID));
}

void RTMServiceIntegrator::initializeRTM(bool useV2)
{
    std::cout << "[Connection Service] Initializing the Connection Service..." << std::endl; 
    m_connectionService->initialize(PRODUCT_ID); 

    std::cout << "[Connection Service] Attempting to connect..." << std::endl;
    
    auto multiConnectCallback = [this](eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> loginSuccess, eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> loginError, const ErrorPtr& error)
    { 
        onConnect(loginSuccess, loginError, error);
    };
    
    // LoginRequestV2 callback adapted for V3 
    auto connectCallback = [this](eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> loginSuccess, const ErrorPtr& error)
    {
        auto response = m_hub->getAllocator().makeShared< com::ea::eadp::antelope::rtm::protocol::LoginV3Response>(m_hub->getAllocator());
        response->setSessionKey(loginSuccess->getSessionKey());
        onConnect(EADPSDK_MOVE(response), nullptr, error);
    };

    auto disconnectCallback = [this](const eadp::realtimemessaging::ConnectionService::DisconnectSource& disconnectSource, const ErrorPtr& error)
    { 
        onDisconnect(disconnectSource, error);
    }; 

    auto userSessionChangeCallback = [this](eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> notification, const ErrorPtr& error)
    {
        onUserSessionChange(notification, error);
    };
    const String& accessToken = m_hub->getIdentityService()->getAccessToken(USER_INDEX).c_str();
    auto options =
        m_hub->getAllocator().makeUnique<eadp::realtimemessaging::ConnectionService::MultiConnectionRequest>(
            m_hub->getIdentityService()->getAccessToken(USER_INDEX), 
            CLIENT_VERSION,
            eadp::realtimemessaging::ConnectionService::MultiConnectCallback(multiConnectCallback, m_persistenceObject, RTM_GROUP_ID),
            eadp::realtimemessaging::ConnectionService::DisconnectCallback(disconnectCallback, m_persistenceObject, RTM_GROUP_ID),
            eadp::realtimemessaging::ConnectionService::UserSessionChangeCallback(userSessionChangeCallback, m_persistenceObject, RTM_GROUP_ID), 
            false,
            "");

    // If the connection is successful, then initialize the other services
    if (useV2)
    {
        m_connectionService->connect(m_hub->getIdentityService()->getAccessToken(USER_INDEX), CLIENT_VERSION, eadp::realtimemessaging::ConnectionService::ConnectCallback(connectCallback, m_persistenceObject, RTM_GROUP_ID),
            eadp::realtimemessaging::ConnectionService::DisconnectCallback(disconnectCallback, m_persistenceObject, RTM_GROUP_ID), true);
    }
    else
    {
        m_connectionService->connect(EADPSDK_MOVE(options));
    }
    
    
}

void RTMServiceIntegrator::onConnect(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> loginResponse, eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> loginError, const ErrorPtr& error)
{
    if (error)
    {
        if (loginError)
        {
            std::cout << "[Connection Service] SESSION_LIMIT_REACHED." << std::endl;
        }

        std::cout << "[Connection Service] Failed to establish a connection with the RTM backend. Ending demo." << std::endl;
        m_isDone = true;
        return;
    }

    std::cout << "[Connection Service] Successfully established a connection." << std::endl;
    std::cout << "[Connection Service] Session Key: " << loginResponse->getSessionKey().c_str() << std::endl;

    // Initialize the services
    initializeChat();
    initializePresence();
    initializeNotification();

    subscribePresence();
}

void RTMServiceIntegrator::onDisconnect(const eadp::realtimemessaging::ConnectionService::DisconnectSource& disconnectSource, const ErrorPtr& error)
{
    std::cout << "[On Disconnect] Disconnected." << std::endl; 
    if (error)
    {
        std::cout << "Code:   " << error->getCode() << std::endl;
        std::cout << "Reason: " << error->getReason().c_str() << std::endl;
    }

    if (disconnectSource.getReason() == eadp::realtimemessaging::ConnectionService::DisconnectSource::Reason::RECONNECT_REQUEST)
    {
        auto connectCallback = [this](eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> loginResponse, eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> loginError, const ErrorPtr& error)
        {
            onConnect(loginResponse, loginError, error);
        };

        auto connectionUpdateCallback = [this](const eadp::realtimemessaging::ConnectionService::DisconnectSource disconnectSource, const ErrorPtr& error)
        {
            onDisconnect(disconnectSource, error);
        };

        auto userSessionChangeCallback = [this](eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> notification, const ErrorPtr& error)
        {
            onUserSessionChange(notification, error);
        };

        // Recommended to use a new access token. We use the old token for simplicity. 
        UniquePtr<eadp::realtimemessaging::ConnectionService::MultiConnectionRequest> options =
            m_hub->getAllocator().makeUnique<eadp::realtimemessaging::ConnectionService::MultiConnectionRequest>(
                m_hub->getIdentityService()->getAccessToken(USER_INDEX),
                "EADPSDK-RTM-demo",
                eadp::realtimemessaging::ConnectionService::MultiConnectCallback(connectCallback, m_persistenceObject, RTM_GROUP_ID),
                eadp::realtimemessaging::ConnectionService::DisconnectCallback(connectionUpdateCallback, m_persistenceObject, RTM_GROUP_ID),
                eadp::realtimemessaging::ConnectionService::UserSessionChangeCallback(userSessionChangeCallback, m_persistenceObject, RTM_GROUP_ID),
                false,
                "");

        
        m_connectionService->reconnect(EADPSDK_MOVE(options));
    }

    // Simply end the program 
    m_isDone = true; 
}

void RTMServiceIntegrator::onUserSessionChange(eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionNotificationV1> notification, const ErrorPtr& error)
{
    switch (notification->getType())
    {
    case com::ea::eadp::antelope::rtm::protocol::SessionNotificationTypeV1::USER_LOGGED_IN:
        std::cout << "[On UserSession Change] User Logged in." << std::endl;
        std::cout << "[On UserSession Change] Message: " << notification->getMessage().c_str() << std::endl;
        break;
    case com::ea::eadp::antelope::rtm::protocol::SessionNotificationTypeV1::USER_LOGGED_OUT:
        std::cout << "[On UserSession Change] User Logged out." << std::endl;
        std::cout << "[On UserSession Change] Message: " << notification->getMessage().c_str() << std::endl;
        break;
    default:
        std::cout << "[On UserSession Change] Unknown SessionNotificationV1 Type." << std::endl;
        std::cout << "[On UserSession Change] Message: " << notification->getMessage().c_str() << std::endl;
        break;
    }
}

void RTMServiceIntegrator::initializeChat()
{
    std::cout << "[Chat Service] Initializing the Chat Service..." << std::endl;
    m_chatService->initialize(m_connectionService); 
}

void RTMServiceIntegrator::initializePresence()
{
    std::cout << "[Presence Service] Initializing the Presence Service..." << std::endl;
    auto userPresenceCallback = [this](const SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceV1> presenceUpdate)
    {
        std::cout << "[On Presence Update] Received user presence." << std::endl;
        std::cout << "User Id:       " << presenceUpdate->getPlayer()->getPlayerId().c_str() << std::endl;
        std::cout << "Status:        " << presenceUpdate->getStatus().c_str() << std::endl;
        std::cout << "Timestamp:     " << presenceUpdate->getTimestamp().c_str() << std::endl;
        getConnectedSessions();
    };

    auto subscriptionErrorCallback = [this](const SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceSubscriptionErrorV1> presenceSubscriptionError)
    {
        std::cout << "[On Presence Subscription Error] Failed to subscribe to presence." << std::endl;
        std::cout << "User Id:       " << presenceSubscriptionError->getPlayer()->getPlayerId().c_str() << std::endl;
        std::cout << "Error Code:    " << presenceSubscriptionError->getErrorCode().c_str() << std::endl;
        std::cout << "Error Message: " << presenceSubscriptionError->getErrorMessage().c_str() << std::endl;
        m_isDone = true; 
    };

    auto presenceUpdateErrorCallback = [this](const SharedPtr<com::ea::eadp::antelope::rtm::protocol::PresenceUpdateErrorV1> presenceUpdateError)
    {
        std::cout << "[On Presence Update Error] Failed to update the user presence status." << std::endl;
        std::cout << "Error Code:    " << presenceUpdateError->getErrorCode().c_str() << std::endl;
        std::cout << "Error Message: " << presenceUpdateError->getErrorMessage().c_str() << std::endl;
        m_isDone = true;
    };

    m_presenceService->initialize(m_connectionService,
                                  eadp::realtimemessaging::PresenceService::PresenceUpdateCallback(userPresenceCallback, m_persistenceObject, RTM_GROUP_ID),
                                  eadp::realtimemessaging::PresenceService::PresenceSubscriptionErrorCallback(subscriptionErrorCallback, m_persistenceObject, RTM_GROUP_ID),
                                  eadp::realtimemessaging::PresenceService::PresenceUpdateErrorCallback(presenceUpdateErrorCallback, m_persistenceObject, RTM_GROUP_ID));
}

void RTMServiceIntegrator::initializeNotification()
{
    std::cout << "[Notification Service] Initializing the Notification Service..." << std::endl;
    auto notificationCallback = [](const SharedPtr<com::ea::eadp::antelope::rtm::protocol::NotificationV1> notificationUpdate)
    {
        std::cout << "[On Notification] Received notification." << std::endl;
        std::cout << "Type:    " << notificationUpdate->getType().c_str() << std::endl;
        std::cout << "Payload: " << notificationUpdate->getPayload().c_str() << std::endl;
    };

    m_notificationService->initialize(m_connectionService, 
                                      eadp::realtimemessaging::NotificationService::NotificationCallback(notificationCallback, m_persistenceObject, RTM_GROUP_ID));
}

void RTMServiceIntegrator::subscribePresence()
{
    std::cout << "[Presence Service] Subscribing presence..." << std::endl; 

    // Create the Subscribe Request to Self
    auto request = m_allocator.makeUnique<com::ea::eadp::antelope::rtm::protocol::PresenceSubscribeV1>(m_allocator);
    auto playerToSubscribeTo = m_allocator.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator);
    playerToSubscribeTo->setPlayerId(USER_ID);
    playerToSubscribeTo->setProductId(PRODUCT_ID); 
    request->getPlayers().push_back(playerToSubscribeTo); 

    // Callback for handling general errors
    auto generalErrorCallback = [this](const ErrorPtr& error)
    {
        if (error)
        {
            std::cout << "[Presence Service] Failed to subscribe to user presence." << std::endl;
            std::cout << "Code:   " << error->getCode() << std::endl;
            std::cout << "Reason: " << error->getReason().c_str() << std::endl;
            m_isDone = true; 
        }
        else
        {
            std::cout << "[Presence Service] Subscribe request sent." << std::endl; 
        }
    };

    m_presenceService->subscribe(EADPSDK_MOVE(request), eadp::realtimemessaging::PresenceService::ErrorCallback(generalErrorCallback, m_persistenceObject, RTM_GROUP_ID));
}

void RTMServiceIntegrator::getConnectedSessions()
{
    std::cout << "[Get Connected Sessions] Getting connected sessions..." << std::endl;
    auto getSessionsCallback = [this](eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionResponseV1> sessionResponse, const eadp::foundation::ErrorPtr& error)
    {
        if (error)
        {
            std::cout << "[Connection Service] Failed to get connected sessions." << std::endl;
            std::cout << "Code:   " << error->getCode() << std::endl;
            std::cout << "Reason: " << error->getReason().c_str() << std::endl;
            m_isDone = true;
        }
        else
        {
            std::cout << "[Connection Service] Retrieved connected sessions." << std::endl;

            for (auto session : sessionResponse->getConnectedSessions())
            {
                std::cout << "  SessionKey: " << session->getSessionKey().c_str() << std::endl;
            }
            
            disconnectRTM();
        }
    };

    m_connectionService->getSessions(eadp::realtimemessaging::ConnectionService::SessionRequestCallback(getSessionsCallback, m_persistenceObject, RTM_GROUP_ID));
}

void RTMServiceIntegrator::disconnectRTM()
{
    std::cout << "[Disconnect RTM] Disconnecting..." << std::endl;

    // Disconnect
    m_connectionService->disconnect();
    m_isDone = true; 
}

//////////////////////////////////////////////////////////////////////////
///// Main 
//////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    // Creates the foundation hub and initializes it using the helper class defined above.
    FoundationIntegrator foundation;

    // Initialize our RTM integration wrapper which will call RPCs on the service. This will also connect the OriginService and call ID20AuthService login.
    RTMServiceIntegrator rtmIntegrator = RTMServiceIntegrator(foundation);

    while (!rtmIntegrator.isDone())
    {
        // Periodically call update on the foundation integrator instance to pump idle and invoke callbacks.
        foundation.update();
        EA::Thread::ThreadSleep((EA::Thread::ThreadTime)30);
    }
    return 0;
}

