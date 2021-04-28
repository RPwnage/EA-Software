/*************************************************************************************************/
/*!
    \file networkmeshadapterlistener.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_GAME_NETWORK_LISTENER_H
#define BLAZE_GAME_NETWORK_LISTENER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/networkmeshadapter.h" 

namespace Blaze
{

namespace BlazeNetworkAdapter
{

    /*! **************************************************************************************************************/
    /*! 
    \class NetworkMeshAdapterListener
    \brief
           A listener used internally by GameManagerAPI to listen for network events. 

    \details
    This class is internal to GameManagerApi. It is not supposed to be used title code.
    Please use GameListener for high level events and NetworkMeshAdapterUserListener for
    low level adapter events.

    \see Blaze::GameManager::GameListener 
    ******************************************************************************************************************/
    class BLAZESDK_API NetworkMeshAdapterListener
    {
    public:

        static const uint16_t CONNECTION_FLAG_COUNT = 16;
        static const uint16_t CONNECTION_FLAG_DEMANGLED = 1;        //!< set if demangler was attempted and succeeded
        static const uint16_t CONNECTION_FLAG_PKTRECEIVED = 2;  

        typedef eastl::bitset<CONNECTION_FLAG_COUNT> ConnectionFlagsBitset;

        /*! ****************************************************************************/
        /*! \brief Called when the game network has been created 
            by NetworkMeshAdapter:initNetworkMesh

            \param mesh The pointer to the interface to game manager network adapter.
            \param userIndex The creating local user's index. Use 0 if Multiple Local Users is not used.
            \param error The error if any.
        ********************************************************************************/
        virtual void networkMeshCreated( const Blaze::Mesh *mesh, uint32_t userIndex,
            NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;

        /*! ****************************************************************************/
        /*! \brief Called when the game network has been destroyed
            by NetworkMeshAdapter:destroyNetworkMesh

            \param mesh The pointer to the interface to game manager network adapter.
            \param error The error if any.
        ********************************************************************************/
        virtual void networkMeshDestroyed( const Blaze::Mesh *mesh, 
            NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;

        /*! ****************************************************************************/
        /*! \brief Called when the local user establishes a connection to a player within
            a game, by NetworkMeshAdapter:connectToEndpoint

            \param mesh The pointer to the mesh
            \param targetConnGroupId the connection group id disconnected from
            \param connectionFlags Connection-specific flags
            \param error The error if any.
        ********************************************************************************/
        virtual void connectedToEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, 
            const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;

        /*! ****************************************************************************/
        /*! \brief Called when the connection from local user to a player within a game
             is cut due to an explicit request to disconnect.

            \param endpoint The pointer to the mesh endpoint
            \param connectionFlags Connection-specific flags
            \param error The error if any.
        ********************************************************************************/
        virtual void disconnectedFromEndpoint( const Blaze::MeshEndpoint *endpoint,
             const ConnectionFlagsBitset connectionFlags, NetworkMeshAdapter::NetworkMeshAdapterError error) { }

        /*! ****************************************************************************/
        /*! \brief Called when the connection from local user to a player within a game
            is cut due to network conditions.

            \param mesh The pointer to the mesh
            \param targetConnGroupId the connection group id disconnected from
            \param connectionFlags Connection-specific flags
            \param error The error if any.
        ********************************************************************************/
        virtual void connectionToEndpointLost( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, 
            const ConnectionFlagsBitset connectionFlags, NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;


         /*! ****************************************************************************/
         /*! \brief Called when the local user establishes a voip connection to a player within
            a mesh, by NetworkMeshAdapter:connectToEndpoint

            \param mesh The pointer to the mesh
            \param targetConnGroupId the connection group id disconnected from
            \param connectionFlags Connection-specific flags
            \param error The error if any.
        ********************************************************************************/
        virtual void connectedToVoipEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, 
            const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;


        /*! ****************************************************************************/
        /*! \brief Called when the voip connection from local user to a player within a mesh
             is cut due to an explicit request to disconnect.

            \param mesh The pointer to the mesh
            \param targetConnGroupId the connection group id disconnected from
            \param connectionFlags Connection-specific flags
            \param error The error if any.
        ********************************************************************************/
        virtual void disconnectedFromVoipEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, 
             const ConnectionFlagsBitset connectionFlags, NetworkMeshAdapter::NetworkMeshAdapterError error) = 0;

        /*! ****************************************************************************/
        /*! \brief Called when the voip connection from local user to a player within a mesh
            is cut due to network conditions.

            \param mesh The pointer to the mesh
            \param targetConnGroupId the connection group id disconnected from
            \param connectionFlags Connection-specific flags
            \param error The error if any.
        ********************************************************************************/
        virtual void connectionToVoipEndpointLost( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, 
            const ConnectionFlagsBitset connectionFlags, NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;

        /*! ****************************************************************************/
        /*! \brief Called when migration of the topology host completes
            from NetworkMeshAdapter:migrateTopologyHost

            \param mesh The pointer to the interface to game manager network adapter.
            \param error The error if any.
        ********************************************************************************/
        virtual void migratedTopologyHost( const Blaze::Mesh *mesh, 
            NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;

        /*! ****************************************************************************/
        /*! \brief Called when migration of the platform host completes
            from NetworkMeshAdapter:migratePlatformHost

            \param mesh The pointer to the interface to game manager network adapter.
            \param error The error if any.
        ********************************************************************************/
        virtual void migratedPlatformHost( const Blaze::Mesh *mesh, 
            NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;

        /*! ****************************************************************************/
        /*! \brief DEPRECATED - This functionality is handled server side now by default. Stock Blaze does not handle it
            Called when a new np session has become available.

            \param mesh The pointer to the interface to game manager network adapter.
            \param npSession The new np session
        ********************************************************************************/
        virtual void npSessionAvailable(const Mesh* mesh, const Blaze::NpSessionId& npSession) {}

        /* TO BE DEPRECATED - do not use - not MLU ready - receiving game data should be implemented against DirtySDk's NetGameLink api directly */
        virtual void receivedMessageFromEndpoint( const Blaze::MeshEndpoint *endpoint, const void *buf, size_t bufSize,
            BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags, 
            BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error) = 0;

        /*! ****************************************************************************/
        /*! \brief DEPRECATED - This functionality is handled server side now by default. Stock Blaze does not handle it
             Called when the presence session for a mesh has changed

            \param mesh The pointer to the interface to game manager network adapter.
            \param error The error if any
        ********************************************************************************/
        virtual void presenceChanged( const Mesh *mesh, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error) {}

        /*! ****************************************************************************/
        /*! \name Destructor
        ********************************************************************************/
        virtual ~NetworkMeshAdapterListener() {}
    };

    /*! **************************************************************************************************************/
    /*! \class NetworkMeshAdapterUserListener
        \brief A listener used by the sdk user for low level adapter events 
    ******************************************************************************************************************/
    class BLAZESDK_API NetworkMeshAdapterUserListener
    {
    public:
        
        /*! ****************************************************************************/
        /*! \brief Called when initialization of the adapter occurs

            \param mesh The pointer to the interface to game manager network adapter.
            \param error The error if any (the adapter can't be used on error)
        ********************************************************************************/
        virtual void onInitialized( const Blaze::Mesh *mesh, NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;
        
        /*! ****************************************************************************/
        /*! \brief Called when the adapter is about to go into an unusable state.

            \param mesh The pointer to the interface to game manager network adapter.
            \param error The error if any.
        ********************************************************************************/
        virtual void onUninitialize( const Blaze::Mesh *mesh, NetworkMeshAdapter::NetworkMeshAdapterError error ) = 0;
        
        /*! ****************************************************************************/
        /*! \name Destructor
        ********************************************************************************/
        virtual ~NetworkMeshAdapterUserListener() {}
    };


}
} //namespace GameManager
#endif //BLAZE_GAME_NETWORK_ADAPTER_H
