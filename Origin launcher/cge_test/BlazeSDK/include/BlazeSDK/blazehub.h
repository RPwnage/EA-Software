/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_HUB_H
#define BLAZE_HUB_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include Files *******************************************************************************/
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/apidefinitions.h"
#include "BlazeSDK/blazestateprovider.h"
#include "BlazeSDK/jobscheduler.h"
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/componentmanager.h"
#include "BlazeSDK/s2smanager/s2smanager.h"

#include "BlazeSDK/component/framework/tdf/userdefines.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"

#include "BlazeSDK/usergroup.h"

#include "BlazeSDK/blaze_eastl/fixed_vector.h"
#include "BlazeSDK/blaze_eastl/vector.h"
#include "BlazeSDK/blaze_eastl/vector_map.h"

#include "BlazeSDK/component/redirector/tdf/redirectortypes.h"

struct NetCritT;

namespace Blaze
{
    class API;
    class Idler;
    class Job;
    class AddressResolver;
    class ServiceResolver;

    // Well-known APIs
    namespace UserManager       { class UserManager; }
    namespace ConnectionManager { class ConnectionManager; }
    namespace LoginManager      { class LoginManager; }
    namespace GameManager       { class GameManagerAPI; }
    namespace CensusData        { class CensusDataAPI;}
    namespace Messaging         { class MessagingAPI; }
    namespace Telemetry         { class TelemetryAPI; }
    namespace Util              { class UtilAPI; }
    namespace Association       { class AssociationListAPI; }
    namespace ByteVaultManager  { class ByteVaultManager; }
    namespace S2SManager        { class S2SManager; }

    namespace Stats
    {
        class StatsAPI;
        class LeaderboardAPI;
    }

    typedef vector<API*> APIPtrVector;

    typedef enum {
        ENVIRONMENT_SDEV,          ///< Development environment
        ENVIRONMENT_STEST,         ///< Test environment
        ENVIRONMENT_SCERT,         ///< Certification environment
        ENVIRONMENT_PROD,          ///< Production environment
    } EnvironmentType;

    BLAZESDK_API const char8_t* EnvironmentTypeToString(EnvironmentType type);

    /*! ***************************************************************/
    /*! \class BlazeHub
        \ingroup _mod_initialization

        \brief
        BlazeHub provides initialization for the Blaze SDK and is 
        the starting point for accessing all features

        \details
        Ryan is going to put more details in here

     *******************************************************************/
    class BLAZESDK_API BlazeHub : public ConnectionManagerStateListener, public UserManagerStateListener
    {
    public:
        static const size_t CLIENT_NAME_MAX_LENGTH = Redirector::CLIENT_NAME_MAX_LENGTH;
        static const size_t CLIENT_VERSION_MAX_LENGTH = Redirector::CLIENT_VERSION_MAX_LENGTH;
        static const size_t CLIENT_SKU_ID_MAX_LENGTH = Redirector::CLIENT_SKU_ID_MAX_LENGTH;
        static const size_t SERVICE_NAME_MAX_LENGTH = Redirector::SERVICE_NAME_MAX_LENGTH;

        // TODO: Is this defined any where?

        static const size_t ADDRESS_MAX_LENGTH = 256;

        // TODO: Is this defined any where?

        static const size_t CONNECT_PARAMS_MAX_LENGTH = 128;

        // TODO: Is this defined any where?

        static const size_t DIRTYSOCK_OPTIONS_MAX_LENGTH = 16;

        static const uint16_t DEFAULT_GAME_PORT = 3659;
        /*! ***************************************************************/
        /*! \brief Initialization parameters for a BlazeHub.
        *******************************************************************/
        class BLAZESDK_API InitParameters
        {
        public:
                    
            InitParameters();
            char8_t  ClientName[CLIENT_NAME_MAX_LENGTH];                    ///<A descriptive name of the client using this BlazeHub
            char8_t  ClientVersion[CLIENT_VERSION_MAX_LENGTH];              ///<The version of the client using this BlazeHub
            char8_t  ClientSkuId[CLIENT_SKU_ID_MAX_LENGTH];                 ///<The SKU id of the client using this BlazeHub
            char8_t  ServiceName[SERVICE_NAME_MAX_LENGTH];                  ///<The name of the Blaze service to connect to. (Max length of SERVICE_NAME_MAX_LENGTH]
            char8_t  LocalInterfaceAddress[ADDRESS_MAX_LENGTH];             ///<The local interface to use. Use the empty string for ANY. (Max length of SERVER_ADDRESS_MAX)
            char8_t  AdditionalNetConnParams[CONNECT_PARAMS_MAX_LENGTH];    ///<Added to the parameters we pass to NetConnConnect
            uint32_t UserCount;                                             ///<The number of users that will multi-plex on the Blaze connection.
            uint32_t MaxCachedUserCount;                                    ///<A hard-cap on the maximum number of users managed by the UserManager.   Pre-allocates space for these users.
            uint32_t MaxCachedUserLookups;                                  ///<!!Deprecated!! this param has no effect
            uint32_t MaxCachedUserProfileCount;                             ///<A hard-cap on the maximum number of user profiles managed by the UserManager.
            uint32_t OutgoingBufferSize;                                    ///<The size of the outgoing packet buffer for the Blaze connection.
            uint32_t OverflowOutgoingBufferSize;                            ///<The size of the overflow outgoing packet buffer for the Blaze connection, for packets larger than the OutgoingBufferSize.
            uint32_t DefaultIncomingPacketSize;                             ///<The default size of any incoming packet this connection can handle. Default is 0, meaning packets up to 32KB will be handled.
            uint32_t MaxIncomingPacketSize;                                 ///<The maximum size of any incoming packet this connection can handle.  Default is 0, meaning packets up to 512KB will be handled.
            uint32_t Locale;                                                ///<Indicates the locale to be used for this instance of blaze stored as a Iso code
            uint32_t Country;                                               ///<Indicates the country/region to be used for this instance of blaze stored as a Iso code
            ClientType Client;                                              ///<enumeration of client type 
            bool IgnoreInactivityTimeout;                                ///<If true, the Blaze server will not disconnect client because of inactivity, if the server is configured to allow this override.
            bool  Secure;                                                ///<If true, the Blaze connection will use SSL encryption.
            EnvironmentType Environment;                                    ///<Identifies the server environment the application should be connecting to
            uint16_t GamePort;                                              ///<The port game connections will be made on
            bool UsePlainTextTOS;                                        ///<If true, will get plain text version of TOS
            bool EnableQos;                                              ///<If true, QOS check will happen during connection
            uint32_t BlazeConnectionTimeout;                                ///<Time for the SDK to connect to a blazeserver, in ms. Default is 'Blaze::NETWORK_CONNECTIONS_RECOMMENDED_MINIMUM_TIMEOUT_MS'.
            uint32_t DefaultRequestTimeout;                                 ///<Default timeout if none is specified by an RPC.  Default is 10s. This setting can be overridden by the server.
            bool ExtendedConnectionErrors;                               ///<Exposes new errors used to diagnose Blaze server connection failures via ConnectionManager.
            bool EnableNetConnIdle;                                      ///<If true, the SDK will call DirtySDK's NetConnIdle.

#ifdef BLAZE_USER_PROFILE_SUPPORT
            bool IncludeProfileRawData;                                     ///<Whether each UserProfile object will keep a copy of the raw data (payload) from first party. Default is true.
#endif

#if !defined(BLAZESDK_TRIAL_SUPPORT)
            bool isTrial;                               ///<if true, the client will be auto-directed to the trial-service name if there exists one>
#endif
            struct
            {
                char8_t RedirectorAddress[ADDRESS_MAX_LENGTH];
                bool RedirectorSecure;
                uint16_t RedirectorPort;
                char8_t DisplayUri[ADDRESS_MAX_LENGTH];
            } Override;

            //! \cond INTERNAL_DOCS
            /*! ****************************************************************************/
            /*! \brief prints the parameters to the blaze log. Called internally by BlazeSDK.
            ********************************************************************************/
            void logParameters() const;
            //! \endcond 
        };

    public:

        /*! ****************************************************************************/
        /*! \brief Creates and initializes the manager.

            If the function succeeds then the hub will be created and its state set to "Initialized".

            \param hub [out] Address of the pointer to receive the hub.
            \param params The initialization parameters for this hub.
            \param allocator A pointer to a memory allocator. The caller is responsible for newing and deleting the Allocator. While caller owns the allocator, the caller should not delete the allocator during the lifetime of the BlazeHub. 
            \param hook   The function to use to output debug messages.
            \param data   An optional data pointer to be passed back to the logging function.
            \param certData An optional pointer to a certificate for mTLS
            \param keyData An optional pointer to a private key for mTLS

            \return The error code of the operation - ERR_OK if the initialization completes successfully.
        ********************************************************************************/
        static BlazeError initialize(BlazeHub **hub, const InitParameters &params, EA::Allocator::ICoreAllocator *allocator, Blaze::Debug::LogFunction hook, void *data, const char8_t* certData = nullptr, const char8_t* keyData = nullptr);

        /*! ****************************************************************************/
        /*! \brief Shuts down and destroys the BlazeHub singleton.

            This function returns immediately, and will disconnect from any Blaze server if already 
            connected. All pointers to any object including the hub singleton will              
            immediately become invalid.

            \param blazeHub The BlazeHub to shutdown.
        ********************************************************************************/
        static void shutdown( BlazeHub *blazeHub );

    public:

        /*! ***************************************************************************/
        /*!    \brief Gets the initialization parameters this hub was created with.
        *******************************************************************************/
        const BlazeHub::InitParameters& getInitParams() const { return mInitParams; } 

        /*! ****************************************************************************/
        /*! \brief Gets the client cert
        ********************************************************************************/
        const char8_t* getCertData() const { return mCertData; }

        /*! ****************************************************************************/
        /*! \brief Gets the client key
        ********************************************************************************/
        const char8_t* getKeyData() const { return mKeyData; }

        /*! ***************************************************************************/
        /*!    \brief Sets the service name in the initialization parameters for this hub.
                      Note that the service name will only be changed if the hub is 
                      currently disconnected. 

               \return true if the service name was set
        *******************************************************************************/
        bool setServiceName( const char8_t* serviceName );

        /*! ***************************************************************************/
        /*!    \brief Sets the EnvironmentType in the initialization parameters for this hub.
                      Note that the EnvironmentType will only be changed if the hub is 
                      currently disconnected.
               
               \return true if the EnvironmentType was set
        *******************************************************************************/
        bool setEnvironment( EnvironmentType environment );

        /*! ****************************************************************************/
        /*! \brief Gets the max number of users.
        ********************************************************************************/
        uint32_t getNumUsers() const { return mNumUsers; };

        /*! ****************************************************************************/
        /*! \brief Add a user state event handler.
        
            \param handler   The handler to add.
        ********************************************************************************/
        void addUserStateEventHandler(BlazeStateEventHandler *handler);

        /*! ****************************************************************************/
        /*! \brief Remove a user state event handler.
        
            \param handler   The handler to remove.
        ********************************************************************************/
        void removeUserStateEventHandler(BlazeStateEventHandler *handler);         

        /*! ****************************************************************************/
        /*! \brief Add a user state API event handler, which is executed before the user 
                state event handler on startup and after the the user state event handler on
                shutdown.
        
            \param handler   The handler to add.
        ********************************************************************************/
        void addUserStateAPIEventHandler(BlazeStateEventHandler *handler);

        /*! ****************************************************************************/
        /*! \brief Remove a user state API event handler (reverses addUserStateAPIEventHandler.)
        
            \param handler   The handler to remove.
        ********************************************************************************/
        void removeUserStateAPIEventHandler(BlazeStateEventHandler *handler);         

        /*! ****************************************************************************/
        /*! \brief Add a user state Framework event handler, which is executed before the user 
                state API event handler on startup and after the the user state API event handler on
                shutdown.
        
            \param handler   The handler to add.
        ********************************************************************************/
        void addUserStateFrameworkEventHandler(BlazeStateEventHandler *handler);

        /*! ****************************************************************************/
        /*! \brief Remove a user state Framework event handler (reverses addUserStateFrameworkEventHandler.)
        
            \param handler   The handler to remove.
        ********************************************************************************/
        void removeUserStateFrameworkEventHandler(BlazeStateEventHandler *handler);         


        /*! ****************************************************************************/
        /*! \brief Add an Idler.
        
            \param idler The idler to add.
            \param idlerName idlerName
        ********************************************************************************/
        void addIdler(Idler *idler, const char* idlerName = "unknownIdler") 
        { 
            idler->mIdlerName = idlerName;  
            mIdlerDispatcher.addDispatchee(idler); 
        }

        /*! ****************************************************************************/
        /*! \brief Removes an Idler.
        
            \param idler The idler to remove.
        ********************************************************************************/
        void removeIdler(Idler *idler) { mIdlerDispatcher.removeDispatchee(idler); }

        /*! ****************************************************************************/
        /*! \brief Blaze SDK idle function.

        This function idles the Blaze SDK and should be called once per frame for
        correct behavior.
        ********************************************************************************/
        void idle();

        /*! ***************************************************************************/
        /*!    \brief Returns true if the SDK is currently idling.
        
            \return True if the SDK is idling, false otherwise.
        *******************************************************************************/
        bool isIdling() const { return mIsIdling; }

        /*! ***************************************************************************/
        /*!    \brief Gets the current time of the last idle.  If called within an idle this 
                   is the time the idle started.
             
            \return The idle time in milliseconds.
        *******************************************************************************/
        uint32_t getLastIdleTime() const { return mLastCurrMillis; }


        /*! ***************************************************************************/
        /*!    \brief Gets the current time.
        
            \return The current time in milliseconds.
        *******************************************************************************/
        uint32_t getCurrentTime() const;


        /*! ***************************************************************************/
        /*!    \brief Gets server time.

        \return The current server GMT time in seconds since epoch.
        *******************************************************************************/
        uint32_t getServerTime() const;

        /*! ***************************************************************************/
        /*!    \brief Locks SDK's internal critical section, blocks until locks is obtained.

            This function enters the SDK's internal critical section.  If you are making
            a call into Blaze from a thread different from the thread calling 
            BlazeHub::idle you MUST wrap the call with lock/unlock.  Doing so avoids
            any potential overwrites or other threading mishaps.
        *******************************************************************************/
        void lock();


        /*! ***************************************************************************/
        /*!    \brief Tries to lock the SDK's internal critical section.
            
            This function performs the same operation as BlazeHub::lock but does not 
            block the thread if the lock is not obtained.
            
            \return True if the lock was successful, false otherwise.
        *******************************************************************************/
        bool tryLock();

        /*! ***************************************************************************/
        /*!    \brief Unlocks the SDK's internal critical section.

            This function leaves the SDK's critical section after SDK operations have 
            been completed.
        *******************************************************************************/
        void unlock();


        /*! ***************************************************************************/
        /*!    \brief Gets the JobScheduler for this Blaze Hub instance.
        
            \return The scheduler.  Cannot be nullptr.
        *******************************************************************************/
        JobScheduler *getScheduler() { return &mScheduler; }

        ServiceResolver *getServiceResolver() const { return mServiceResolver; }

        /*! ****************************************************************************/
        /*! \brief Gets the user manager
        ********************************************************************************/
        UserManager::UserManager *getUserManager() { return mUserManager; }

        /*! ****************************************************************************/
        /*! \brief Gets the bytevault manager
        ********************************************************************************/
        ByteVaultManager::ByteVaultManager *getByteVaultManager() { return mByteVaultManager; }

        /*! ****************************************************************************/
        /*! \brief Gets the low level component manager for a particular user.

        \param userIndex The user index of component manager to retrieve
        \return The given user's ComponentManager class, or nullptr if the Blaze SDK
        is not currently connected.
        ********************************************************************************/
        ComponentManager *getComponentManager(uint32_t userIndex) const;

        /*! ************************************************************************************************/
        /*! \brief Gets the low level component manager for the primary local user.
            \return the primary local users component manager.
        ***************************************************************************************************/
        ComponentManager *getComponentManager() const;
        /*! ****************************************************************************/
        /*! \brief Indicates whether the specified component is available on the server

        \param componentId The ID of the component to check
        \return True if the component is available, false otherwise
        ********************************************************************************/
        bool isServerComponentAvailable(uint16_t componentId) const;

        /*! ****************************************************************************/
        /*! \brief Gets the connection manager
        ********************************************************************************/
        ConnectionManager::ConnectionManager *getConnectionManager() const { return mConnectionManager; } 


        /*! ****************************************************************************/
        /*! \brief Gets the ClientPlatformType (ex. ps4, xone, pc) that Blaze thinks is being used. 
        ********************************************************************************/
        ClientPlatformType getClientPlatformType() const;

        /*! ****************************************************************************/
        /*! \brief Gets the login manager

        \param userIndex The user index
        ********************************************************************************/
        LoginManager::LoginManager *getLoginManager(uint32_t userIndex) const;

        /*! ****************************************************************************/
        /*! \brief Add a UserGroupProvider for a specific entity type

        \note The job of a UserGroupProvider is to return UserGroup-derived objects like Room, Game, etc. 
        \param bobjType The blaze object type of user group provider to add.
        \param provider The user group provider to add.
        ********************************************************************************/
        bool addUserGroupProvider(const EA::TDF::ObjectType& bobjType, UserGroupProvider *provider);

        /*! ****************************************************************************/
        /*! \brief Add a UserGroupProvider for all entity types of a component

        \note The job of a UserGroupProvider is to return UserGroup-derived objects like Room, Game, etc 
        \param componentId The component type of user group provider to add.
        \param provider The user group provider to add.
        ********************************************************************************/
        bool addUserGroupProvider(const ComponentId componentId, UserGroupProvider *provider);

        /*! ****************************************************************************/
        /*! \brief Removes a UserGroupProvider for a specific entity type

        \note The job of a UserGroupProvider is to return UserGroup-derived objects like Room, Game, etc.
        \param bobjType The blaze object type of user group provider to remove.
        \param provider The user group provider to remove.
        ********************************************************************************/
        bool removeUserGroupProvider(const EA::TDF::ObjectType& bobjType, UserGroupProvider *provider);
        
        /*! ****************************************************************************/
        /*! \brief Removes a UserGroupProvider for a specific component

        \note The job of a UserGroupProvider is to return UserGroup-derived objects like Room, Game, etc.
        \param componentId The component type of user group provider to remove.
        \param provider The user group provider to remove.
        ********************************************************************************/
        bool removeUserGroupProvider(const ComponentId componentId, UserGroupProvider *provider);

        /*! ************************************************************************************************/
        /*! \brief When called returns a UserGroup contained by the class implementing this interface.

        \param[in] userGroupId - blaze object id information
        \return UserGroup - underlying UserGroup object mapped by the provided EA::TDF::ObjectId
        ***************************************************************************************************/
        UserGroup* getUserGroupById(const EA::TDF::ObjectId& userGroupId) const;

        /*! ************************************************************************************************/
        /*! \brief Check if any users are in an active Game, GameGroup, or Matchmaking Session.
                   Used internally to determine if QoS is safe to run. 

        \return bool - true if a user is in an active session.
        ***************************************************************************************************/
        bool areUsersInActiveSession() const;

        /*! ****************************************************************************/
        /*! \brief Creates an API with the hub where there is a single api for all users.

            This function is used to register an API with the hub.  It should only be 
            called by a corresponding API's "create" function.  This is handled for 
            built in Blaze APIs, but if you are writing a custom API you must call this
            from your APIs create function.  All API creation must be done before the 
            BlazeHub moves out of the "Initialized" state.

            \param id        The API id
            \param api  The api.
        ********************************************************************************/
        void createAPI(APIId id, API *api);

        /*! ****************************************************************************/
        /*! \brief Creates an API with the hub where there is more than one API per user.

            This function is used to register a multiple API with the hub.  It should only be 
            called by a corresponding API's "create" function.  This is handled for 
            built in Blaze APIs, but if you are writing a custom API you must call this
            from your APIs create function.  All API creation must be done before the 
            BlazeHub moves out of the "Initialized" state.

            \param id        The API id
            \param apiPtrs   A vector of size "userCount" of pointers to the APIs.
        ********************************************************************************/
        void createAPI(APIId id, APIPtrVector *apiPtrs);

        /*! ****************************************************************************/
        /*! \brief Gets an API.  

            If the API is not a singleton (i.e. one per user), the API for the first user
            index is returned.

            \param id        The API id
        ********************************************************************************/
        API *getAPI(APIId id) const  { return id < MAX_API_ID ? mAPIStorage[id] : nullptr; }

        /*! ****************************************************************************/
        /*! \brief Gets the API with the given ID for the given user.

            If the API is a singleton (i.e. one per user), the userIndex is ignored.

            \param id        The API id
            \param userIndex The user index
        ********************************************************************************/
        API *getAPI(APIId id, uint32_t userIndex) const;
        
        /*! ****************************************************************************/
        /*! \name Well-known API accessors
        ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Gets the CensusSession API.
        ********************************************************************************/
        Blaze::CensusData::CensusDataAPI *getCensusDataAPI() const { return mCensusDataAPI; }


        /*! ****************************************************************************/
        /*! \brief Gets the GameManager API.
        ********************************************************************************/
        Blaze::GameManager::GameManagerAPI *getGameManagerAPI() const { return mGameManagerAPI; }

        /*! ****************************************************************************/
        /*! \brief Gets the Messaging API.
        ********************************************************************************/
        Blaze::Messaging::MessagingAPI *getMessagingAPI(uint32_t userIndex) const;

        /*! ****************************************************************************/
        /*! \brief Gets the Leaderboard API
        ********************************************************************************/
        Blaze::Stats::LeaderboardAPI *getLeaderboardAPI() const { return mLeaderboardAPI; }

        /*! ****************************************************************************/
        /*! \brief Gets the Stats API
        ********************************************************************************/
        Blaze::Stats::StatsAPI *getStatsAPI() const { return mStatsAPI; }

        /*! ****************************************************************************/
        /*! \brief Gets the Telemetry API
        ********************************************************************************/
        Blaze::Telemetry::TelemetryAPI *getTelemetryAPI(uint32_t userIndex) const;

        /*! ****************************************************************************/
        /*! \brief Gets the Util API.
        ********************************************************************************/
        Blaze::Util::UtilAPI *getUtilAPI() const { return mUtilAPI; }

        /*! ****************************************************************************/
        /*! \brief Gets the Util API.
        ********************************************************************************/
        Blaze::Association::AssociationListAPI *getAssociationListAPI(uint32_t userIndex) const;

        /*! ***************************************************************************/
        /*!    \brief Modifies the locale this hub was created with.
        *******************************************************************************/
        void setLocale(uint32_t locale) { mInitParams.Locale = locale; }

        /*! ****************************************************************************/
        /*! \brief Gets the Locale
        ********************************************************************************/
        virtual uint32_t getLocale() const { return mInitParams.Locale; }

        /*! ***************************************************************************/
        /*!    \brief Modifies the country/region this hub was created with.
        *******************************************************************************/
        void setCountry(uint32_t country) { mInitParams.Country = country; }

        /*! ****************************************************************************/
        /*! \brief Gets the Country/Region
        ********************************************************************************/
        virtual uint32_t getCountry() const { return mInitParams.Country; }

        /*! ****************************************************************************/
        /*! \brief Gets the name of an error with a given error code originating from 
               a connection identified by the given user index.
        ********************************************************************************/
        const char8_t* getErrorName(BlazeError errorCode, uint32_t userIndex = 0) const;

        /*! ****************************************************************************/
        /*! \brief Gets the name of an error with a given error code originating from 
               a connection identified by the given user index.
        ********************************************************************************/
        const char8_t* getNotificationName(uint16_t componentId, uint16_t notificationId, uint32_t userIndex = 0)
            { return getComponentManager(userIndex)->getNotificationName(componentId, notificationId); }

        /*! ****************************************************************************/
        /*! \brief Gets the name of an command with a given ID belonging to a component
               a connection identified by the given user index.
        ********************************************************************************/
        const char8_t* getCommandName(uint16_t componentId, uint16_t commandId, uint32_t userIndex = 0)
            { return getComponentManager(userIndex)->getCommandName(componentId, commandId); }

        /*! ************************************************************************************************/
        /*! \brief Returns the primary local user index as maintained by the UserManager.
            \return the primary local user index.
        ***************************************************************************************************/
        uint32_t getPrimaryLocalUserIndex() const;

        /*! ************************************************************************************************/
        /*! \brief Gets the S2S Manager
        ***************************************************************************************************/
        S2SManager::S2SManager *getS2SManager() { return mS2SManager; }

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notification from ConnectionManager that the connection
            to the blaze server has been established.  Implies netconn connection as well
            as first ping response.
        ***************************************************************************************************/
        void onConnected();
        
        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notification from ConnectionManager that the connection
            to the blaze server has been lost.
            \param error - the error code for why the disconnect happened.
        ***************************************************************************************************/
        void onDisconnected(BlazeError error);

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notifications that a local user has authenticated.
        
            \param[in] userIndex - the index of the local user who has authenticated
        ***************************************************************************************************/
        void onLocalUserAuthenticated(uint32_t userIndex);

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notifications that a local user has deauthenticated.
        
            \param[in] userIndex - the index of the local user who has de authenticated
        ***************************************************************************************************/
        void onLocalUserDeAuthenticated(uint32_t userIndex);

        /*! ************************************************************************************************/
        /*! \brief Listener method to handle notifications that a local user has deauthenticated forcefully.

            \param[in] userIndex - the index of the local user who has de authenticated
            \param[in] forcedLogoutType - the type of forced logout for the user
        ***************************************************************************************************/
        void onLocalUserForcedDeAuthenticated(uint32_t userIndex, UserSessionForcedLogoutType forcedLogoutType);


        /*! ************************************************************************************************/
        /*! \brief How many milliseconds can pass between Blaze idles before an error is logged.  Default 10000 ms.

            \param[in] maxStarveMs - How many milliseconds can pass between Blaze idles before an error is logged
        ***************************************************************************************************/
        void setMaxIdleStarveTime(uint32_t maxStarveMs) { mIdleStarveMax = maxStarveMs; }

        /*! ************************************************************************************************/
        /*! \brief How many milliseconds can a Blaze idle process before an error is logged.  Default 5 ms.

            \param[in] maxProcessingMs - How many milliseconds can a Blaze idle process before an error is logged
        ***************************************************************************************************/
        void setMaxIdleProcessingTime(uint32_t maxProcessingMs) {
            mIdleProcessingMax = maxProcessingMs; 
            mScheduler.setMaxJobExecuteTime(mIdleProcessingMax);
        }

        /*! ************************************************************************************************/
        /*! \brief Print a log in idle() call after this interval to indicate it is being called. The related starvation
            log may not assist in the debugging if the idle is not even being called.

            \param[in] idleLogMs - How many milliseconds can pass between Blaze idles before logging the idle call. 0 means disabled.
        ***************************************************************************************************/
        void setIdleLogInterval(uint32_t idleLogMs) { mIdleLogInterval = idleLogMs; }

        /*! ****************************************************************************/
        /*! \brief DEPRECATED  Helper functions for converting to/from PlatformInfo
        If the local ClientPlatformType is 'INVALID', returns the external ID from the
        passed-in PlatformInfo's current platform.
        ********************************************************************************/
        ExternalId getExternalIdFromPlatformInfo(const PlatformInfo& platformInfo) const;
        void setExternalStringOrIdIntoPlatformInfo(PlatformInfo& platformInfo, ExternalId extId, const char8_t* extString, ClientPlatformType platform = INVALID) const;

        /*! **********************************************************************************************************/
        /*! \brief  Returns true if the provided PlatformInfo has a valid external identifier for its ClientPlatform.
        **************************************************************************************************************/
        static bool hasExternalIdentifier(const PlatformInfo& platformInfo);

        /*! ****************************************************************************/
        /*! \brief  Helper function for logging PlatformInfo
        ********************************************************************************/
        static const char8_t* platformInfoToString(const PlatformInfo& platformInfo, eastl::string& str);

    private:

        /*! ****************************************************************************/
        /*! \brief Constructor declared private to avoid external initialization.

            \param[in] params      init parameters  
            \param[in] memGroupId  mem group to be used by this class to allocate memory
        ********************************************************************************/
        BlazeHub(const InitParameters &params, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

        /*! ****************************************************************************/
        /*! \brief Destructor declared private to avoid external shutdown.
        ********************************************************************************/
        virtual ~BlazeHub();


        /*! ***************************************************************************/
        /*!    \brief The internal initialization function.

            \return Any initialization errors.
        *******************************************************************************/
        BlazeError initializeInternal(const char8_t* certData, const char8_t* keyData);

        NON_COPYABLE(BlazeHub);

    private:
        static const uint32_t IDLE_STARVE_MAX = 10000; //10 seconds = 10000 milliseconds
        static const uint32_t IDLE_PROCESSING_MAX = 5; //5 milliseconds

        uint32_t mIdleStarveMax;
        uint32_t mIdleProcessingMax;
        uint32_t mIdleLogInterval;

        static uint32_t msNumInitializedHubs;

        NetCritT *mCritSec;

        typedef fixed_vector<API *, MAX_API_ID, true> APIPtrVectorFixed;
        APIPtrVectorFixed mAPIStorage;

        typedef fixed_vector<APIPtrVector *, MAX_API_ID, true> APIMultiStorageVector;
        APIMultiStorageVector mAPIMultiStorage;

        uint32_t mNumUsers;

        InitParameters mInitParams;

        char8_t* mCertData;
        char8_t* mKeyData;

        ConnectionManager::ConnectionManager  *mConnectionManager;

        typedef vector<LoginManager::LoginManager*> LoginManagerVector;
        LoginManagerVector mLoginManagers;

        UserManager::UserManager *mUserManager;

        typedef vector_map<EA::TDF::ObjectType, UserGroupProvider*> UserGroupProviderByTypeMap;
        UserGroupProviderByTypeMap mUserGroupProviderByTypeMap;
        
        typedef vector_map<EA::TDF::ObjectType, UserGroupProvider*> UserGroupProviderByComponentMap;
        UserGroupProviderByComponentMap mUserGroupProviderByComponentMap;


        uint32_t mLastCurrMillis;
        uint32_t mLastIdleLogMillis;
        bool mIsIdling;
        Dispatcher<Idler> mIdlerDispatcher;

        Dispatcher<BlazeStateEventHandler>  mStateDispatcher;
        Dispatcher<BlazeStateEventHandler>  mApiStateDispatcher;
        Dispatcher<BlazeStateEventHandler>  mFrameworkStateDispatcher;

        JobScheduler mScheduler;
        ServiceResolver* mServiceResolver;

        // We cache off the per-user api array for well known APIs (to avoid a map lookup by API_ID)
        CensusData::CensusDataAPI *mCensusDataAPI;
        GameManager::GameManagerAPI *mGameManagerAPI;
        Stats::StatsAPI *mStatsAPI;
        Stats::LeaderboardAPI *mLeaderboardAPI;
        APIPtrVector *mMessagingAPIs;
        APIPtrVector *mTelemetryAPIs;
        Util::UtilAPI *mUtilAPI; 
        APIPtrVector *mAssociationListAPIs;
        ByteVaultManager::ByteVaultManager *mByteVaultManager;
        S2SManager::S2SManager *mS2SManager;

#if ENABLE_BLAZE_SDK_LOGGING
    public: 

        struct TDFLoggingOptions
        {
            TDFLoggingOptions() : logInbound(false), logOutbound(false) {}
            TDFLoggingOptions(bool enableInbound, bool enableOutbound) : logInbound(enableInbound), logOutbound(enableOutbound) {}
            bool logInbound;
            bool logOutbound;
        };

        // Enable/Disable TDF logging globally
        void setGlobalTDFLoggingOptions(const TDFLoggingOptions& options) { mGlobalLoggingOptions = options; } 
        const TDFLoggingOptions& getGlobalTDFLoggingOptions() const { return mGlobalLoggingOptions; }
        void setGlobalTDFLogging(bool enableGlobalTDFLogging) { mGlobalLoggingOptions = TDFLoggingOptions(enableGlobalTDFLogging, enableGlobalTDFLogging); }
        bool getGlobalTDFLogging() const { return mGlobalLoggingOptions.logInbound || mGlobalLoggingOptions.logOutbound; }

        // Enable/Disable TDF logging per component (Overrides global setting)
        void setComponentTDFLoggingOptions(ComponentId componentId, const TDFLoggingOptions& options);
        const TDFLoggingOptions& getComponentTDFLoggingOptions(ComponentId componentId) const;
        void setComponentTDFLogging(ComponentId componentId, bool enableComponentTDFLogging);
        bool getComponentTDFLogging(ComponentId componentId) const;

        // Enable/Disable TDF logging per component RPC/Notification (Overrides component setting)
        void setComponentCommandTDFLoggingOptions(ComponentId componentId, uint16_t commandId, const TDFLoggingOptions& options, bool notification = false);
        const TDFLoggingOptions& getComponentCommandTDFLoggingOptions(ComponentId componentId, uint16_t commandId, bool notification = false) const;
        void setComponentCommandTDFLogging(ComponentId componentId, uint16_t commandId, bool enableCommandTDFLogging, bool notification = false);
        bool getComponentCommandTDFLogging(ComponentId componentId, uint16_t commandId, bool notification = false) const;

        // Clear/Reset Logging
        void resetTDFLogging();

    private:

        // Store the component/command/notification okay/not call here. 
        TDFLoggingOptions mGlobalLoggingOptions;
        struct ComponentTDFLogging
        {
            ComponentTDFLogging(TDFLoggingOptions options, MemoryGroupId memGroupId, const char8_t *debugMemName) :
                mComponentLoggingOptions(options),
                mCommandLoggingOptions(memGroupId, debugMemName), 
                mNotificationLoggingOptions(memGroupId, debugMemName) {}

            TDFLoggingOptions mComponentLoggingOptions;
            typedef hash_map<uint16_t, TDFLoggingOptions, eastl::hash<uint16_t> > CommandLoggingMap;
            CommandLoggingMap mCommandLoggingOptions;
            CommandLoggingMap mNotificationLoggingOptions;
        };

        typedef hash_map<ComponentId, ComponentTDFLogging*, eastl::hash<ComponentId> > ComponentTDFLoggingMap;
        ComponentTDFLoggingMap mComponentTDFLoggingMap;
#endif

#ifdef ENABLE_BLAZE_SDK_CUSTOM_RPC_TRACKING
    public:
        BlazeSenderStateListener* getBlazeSenderStateListener() const { return mBlazeSenderStateListener; }
        void setBlazeSenderStateListener(BlazeSenderStateListener* blazeSenderStateListener ) { mBlazeSenderStateListener = blazeSenderStateListener; }

    private:
         BlazeSenderStateListener* mBlazeSenderStateListener;
#endif

    };
}

#endif // BLAZE_HUB_H
