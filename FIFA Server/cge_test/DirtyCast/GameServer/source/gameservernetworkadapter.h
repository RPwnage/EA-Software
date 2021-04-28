/*H********************************************************************************/
/*!
    \File    gameservernetworkadapter.h

    \Description
        This module implements the NetworkMeshAdapter for the GameServer.
        It is a private header that should only be included by blaze specific
        files.

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _gameservernetworkadapter_h
#define _gameservernetworkadapter_h

/*** Include files ****************************************************************/

#include "BlazeSDK/version.h"
#include "BlazeSDK/networkmeshadapter.h"

#include "gameserverconfig.h"
#include "gameserverlink.h"

/*** Forward Declarations **********************************************************/

struct GameServerLinkCustT;
struct GameServerDistT;

/*** Interface *********************************************************************/

class GameServerBlazeNetworkAdapterC: public Blaze::BlazeNetworkAdapter::NetworkMeshAdapter
{
public:
    // constructor
    GameServerBlazeNetworkAdapterC();
    // destructor
    virtual ~GameServerBlazeNetworkAdapterC() {}

    /*! ************************************************************************************************/
    /*!
        \brief Hook for the networking module to do global setup (setup not related to a particular game instance).

        The intent is for the networking module to do any global setup during init (as opposed
        to setup that's related to a particular game).

        For example:
          * determining firewall presence & type; getting internal & external network addresses
          * attempt to open the game's listen port in any firewall/router using uPnP
          * running network quality of service metrics

        Note: initialize is called once per GameManagerApi instance.

        \param[in] blazeHub - the blazehub that uses this adapter
    ***************************************************************************************************/
    virtual void initialize(Blaze::BlazeHub *pBlazeHub);

    /*! ************************************************************************************************/
    /*!
        \brief Hook for the networking module to tear itself down.

        Note: destroy is called once per GameManagerApi instance.
    ***************************************************************************************************/
    virtual void destroy();

    /*! ************************************************************************************************/
    /*!
        \brief Returns true once the game network adapter has finished initializing.

        Most GameManagerApi calls error out until the game network adapter is initialized.

        \param[in] userIndex - the index of the gameManagerApi that's initialized
    ***************************************************************************************************/
    virtual bool isInitialized(const Blaze::Mesh *pMesh) const;

    /*! ************************************************************************************************/
    /*!
        \brief Returns true once the game network adapter has finished initializing.

        Most GameManagerApi calls error out until the game network adapter is initialized.

        \param[in] userIndex - the index of the gameManagerApi that's initialized
    ***************************************************************************************************/
    virtual bool isInitialized() const;

    /*! ************************************************************************************************/
    /*!
        \brief Query the game network adapter for a user's local address (prior to creating/joining a game)

        Typically, the network address (which is platform dependent) & port are determined once at initialization
        and cached off.

        \return a const reference to the local address
    ***************************************************************************************************/
    virtual const Blaze::NetworkAddress &getLocalAddress() const;

    #if BLAZE_SDK_VERSION < BLAZE_SDK_MAKE_VERSION(15, 1, 1, 8, 0)
    virtual const Blaze::NetworkAddress &getLocalAddress(uint32_t uUserIndex) const;
    #endif

    /*! ****************************************************************************/
    /*! \brief The NetworkMeshAdapter is idled by BlazeHub's idle.  This is where you should
        'tick' your game networking.

        \param currentTime The tick count of the system in milliseconds.
        \param elapsedTime The elapsed count of milliseconds since the last idle call.
    ********************************************************************************/
    virtual void idle(const uint32_t uCurrentTime, const uint32_t uElapsedTime);

    /*! ****************************************************************************/
    /*! \name Game network setup/teardown methods
    ********************************************************************************/

    /*! ****************************************************************************/
    /*! \brief Sets up the underlying networking for a game
        invokes NetworkMeshAdapterListener::networkMeshCreated

        \param mesh Pointer to the interface to the gamemanager network adapter.
        \param netType - determine if you are using just game data, voip or both
        \param pres - enable or disable presence
    ********************************************************************************/
    virtual void initNetworkMesh(const Blaze::Mesh *pMesh, const Config &Config);

    /*! ****************************************************************************/
    /*! \brief Creates a network connection between the local user and the given player.
        invokes NetworkMeshAdapterListener::connectedToEndpoint

        \param meshEndpoint Endpoint to connect to.
    ********************************************************************************/
    virtual void connectToEndpoint(const Blaze::MeshEndpoint *pMeshEndpoint);

    /*! ************************************************************************************************/
    /*! \brief Adds a user specified by the MeshMember to their machine.  Enables multiple
        users on a single endpoint.

        \param[in] user - the user being added to their machine endpoint.
    ***************************************************************************************************/
    virtual void addMemberOnEndpoint(const Blaze::MeshMember *pMeshMember);

    /*! ************************************************************************************************/
    /*! \brief Removes a user specified by MeshMember from their machine.

        \param[in] user - the user being removed.
    ***************************************************************************************************/
    virtual void removeMemberOnEndpoint(const Blaze::MeshMember *pMeshMember);

    /*! ****************************************************************************/
    /*! \brief Destroys the network connection between the local user, and the given player.
        invokes NetworkMeshAdapterListener::disconnectedFromEndpoint

        \param meshEndpoint Endpoint to disconnect from.
    ********************************************************************************/
    virtual void disconnectFromEndpoint(const Blaze::MeshEndpoint *pMeshEndpoint);

    /*! ****************************************************************************/
    /*! \brief Tears down the underlying networking for a specific game.
        invokes NetworkMeshAdapterListener::networkMeshDestroyed

        \param mesh The pointer to the interface to game manager network adapter.
    ********************************************************************************/
    virtual void destroyNetworkMesh(const Blaze::Mesh *pMesh);

    /*! ****************************************************************************/
    /*! \brief Start a game session. Mainly for supporting xbox network.

        \param mesh The pointer to the interface to game manager network adapter.
    ********************************************************************************/
    virtual void startGame(const Blaze::Mesh *pMesh);

    /*! ****************************************************************************/
    /*! \brief End a game session. Mainly for supporting xbox network.

        \param mesh The pointer to the interface to game manager network adapter.
    ********************************************************************************/
    virtual void endGame(const Blaze::Mesh *pMesh);

    /*! ****************************************************************************/
    /*! \brief  Inform the game has been reset, and will need to start hosting again.

        \param mesh The pointer to the interface to game manager network adapter.
    ********************************************************************************/
    virtual void resetGame(const Blaze::Mesh *pMesh);

    /*! ****************************************************************************/
    /*! \brief Gets ConnApi connection timers for a given endpoint

        \param endpoint The target endpoint
        \param gameConnectTimers a struct to contain the game ConnApi connection timers
        \param voipConnectTimers a struct to contain the voip ConnApi connection timers
        \return a bool true, if the call was successful
    ********************************************************************************/
    virtual bool getConnectionTimersForEndpoint(const Blaze::MeshEndpoint *pEndpoint, Blaze::BlazeNetworkAdapter::MeshEndpointConnTimer &GameConnectTimers, Blaze::BlazeNetworkAdapter::MeshEndpointConnTimer &VoipConnectTimers);

    /*! ****************************************************************************/
    /*! \brief Gets the QOS statistics for a given endpoint

        \param endpoint The target endpoint
        \param qosData a struct to contain the qos values
        \param bInitialQosStat To select if initial latency average or recent latency value should be reported
        \return a bool true, if the call was successful
    ********************************************************************************/
    virtual bool getQosStatisticsForEndpoint(const Blaze::MeshEndpoint *pEndpoint, Blaze::BlazeNetworkAdapter::MeshEndpointQos &QosData, bool bInitialQosStats);

    /*! ****************************************************************************/
    /*! \brief Get Connection Status Flag

        \param endpoint The target endpoint
        \param uConnectionFlag The Connection Flag
    ********************************************************************************/
    virtual void getConnectionStatusFlagsForMeshEndpoint(const Blaze::MeshEndpoint *pEndpoint, uint32_t &uConnectionFlag);

    /*! ****************************************************************************/
    /*! \brief Migrate the topology host of a specific game
        invokes NetworkMeshAdapterListener::migratedTopologyHost

        \param mesh The pointer to the interface to game manager network adapter.
    ********************************************************************************/
    virtual void migrateTopologyHost(const Blaze::Mesh *pMesh);

    /*! ****************************************************************************/
    /*! \brief Migrate the platform host of a specific game
        invokes NetworkMeshAdapterListener::migratedPlatformHost

        \param mesh The pointer to the interface to game manager network adapter.
    ********************************************************************************/
    virtual void migratePlatformHost(const Blaze::Mesh *pMesh);

    /*! ****************************************************************************/
    /*! \brief Establish the voip connection that has been delayed because of QoS
        validation.

        When QoS validation is enabled in matchmaking we need to insure that before
        any connections are establish we have validated that we have a good connection.
        Otherwise voip connection is established but then the connection will get
        cut off if validation fails.

        \param endpoint What endpoint we are establishing the voip connection on
    ********************************************************************************/
    virtual void activateDelayedVoipOnEndpoint(const Blaze::MeshEndpoint *pEndpoint);

    /*! ************************************************************************************************/
    /*! \brief Returns a string identifying the adapter name (and possibly version info), used to identify the adapter
        while logging.

        \return a log-able string describing the adapter version: ex: "ConnApiAdapter 1.3".
    ***************************************************************************************************/
    virtual const char8_t *getVersionInfo() const;

    // returns the gameserverlink ref for those that need access
    GameServerLinkT *GetGameServerLink();

    // returns the gameserverdist ref for those that need access
    GameServerDistT *GetGameServerDist();

    // configure the adapter
    void SetConfig(const GameServerConfigT *pConfig, const GameServerLinkCreateParamT *pLinkParams);

private:
    // disabled copy constructor
    GameServerBlazeNetworkAdapterC(GameServerBlazeNetworkAdapterC &);
    // disabled move constructor
    GameServerBlazeNetworkAdapterC(GameServerBlazeNetworkAdapterC &&);

    // gameserverlink event callback
    static void GameClientEvent(GameServerLinkClientListT *pClientList, GameServLinkClientEventE eEvent, int32_t iClient, void *pUpdateData);
    // gameserverlink update callback
    static int32_t GameLinkUpdate(GameServerLinkClientListT *pClientList, int32_t iClient, void *pUpdateData, char *pLogBuffer);

    uint64_t m_aConnections[64];                //!< connection group ids for the clients
    const Blaze::Mesh *m_pMesh;                 //!< pointer to the mesh for easy access
    Blaze::NetworkAddress m_LocalAddress;       //!< the local address of the server

    const GameServerConfigT *m_pConfig;         //!< gameserver configuration
    const GameServerLinkCreateParamT *m_pLinkParams;//!< gameserverlink creation parameters

    GameServerLinkT *m_pGameLink;               //!< gameserverlink that handles the client connections
    GameServerLinkCustT *m_pGameLinkCust;       //!< custom gameserverlink implemented by customers
    GameServerDistT *m_pGameDist;               //!< gameserverdist module that implements input pairing using netgamedistserv
};

#endif
