/*************************************************************************************************/
/*!
    \file networkmeshadapter.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/networkmeshadapter.h"

namespace Blaze
{
namespace BlazeNetworkAdapter
{
    const char8_t* NetworkMeshAdapter::NetworkMeshAdapterErrorToString(NetworkMeshAdapterError gameNetworkError)
    {
        switch(gameNetworkError)
        {
        case ERR_OK:
            return "ERR_OK";
        case ERR_CONNECTION_FAILED:
            return "ERR_CONNECTION_FAILED";
        case ERR_ADAPTER_NOT_INITIALIZED:
            return "ERR_ADAPTER_NOT_INITIALIZED";
        case ERR_CANNOT_INIT_NETWORK:
            return "ERR_CANNOT_INIT_NETWORK";
        case ERR_PLAYER_EXIST:
            return "ERR_PLAYER_EXIST";
        case ERR_BAD_INPUT:
            return "ERR_BAD_INPUT";
        case ERR_NOT_IMPLEMENTED:
            return "ERR_NOT_IMPLEMENTED";
        case ERR_INTERNAL:
            return "ERR_INTERNAL";
        default:
            return "UNKNOWN";
        }
    }



    //////////////////////////////////////////////////////////////////////////
    // Network Mesh helper functions
    //////////////////////////////////////////////////////////////////////////


    NetworkMeshHelper::NetworkMeshHelper(MemoryGroupId memGroupId, NetworkMeshAdapter* networkAdapter)
        : mNetworkAdapter(networkAdapter),
        mConnectionGroupMap(memGroupId, MEM_NAME(memGroupId, "NetworkMeshHelper::ConnectionGroupMap"))
    {

    }

    NetworkMeshHelper::~NetworkMeshHelper()
    {

    }

    /*! ************************************************************************************************/
    /*! \brief increment reference count for the user's connection group and return whether it was the 1st count.
    ***************************************************************************************************/
    bool NetworkMeshHelper::referenceConnGroup(const MeshMember& user)
    {
        if ((user.getMeshEndpoint() == nullptr) || (user.getMeshEndpoint()->getMesh() == nullptr))
        {
            BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Reference cannot be incremented as user(%" PRIi64 ")'s mesh endpoint was nullptr.\n", user.getId());
            return false;
        }
        MeshIdMemberIdPairSet::key_type key(user.getMeshEndpoint()->getMesh()->getId(), user.getId());
        uint64_t connGroup = user.getMeshEndpoint()->getConnectionGroupId();
        ConnectionGroupMap::iterator iter = mConnectionGroupMap.find(connGroup);
        if (iter != mConnectionGroupMap.end())
        {
            if ((iter->second).insert(key).second == true)
            {
                BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Reference incremented to %" PRIsize " for connection group(%" PRIu64 "), on user(%" PRIi64 ") joining mesh(%" PRIu64 ").\n", (size_t)iter->second.size(), connGroup, key.second, key.first);
            }
            return false;
        }
        else
        {
            mConnectionGroupMap[connGroup].insert(key);
            BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Reference added for connection group(%" PRIu64 "), on user(%" PRIi64 ") joining mesh(%" PRIu64 "). Reference count is 1.\n", connGroup, key.second, key.first); 
            return true;
        }
    }

    bool NetworkMeshHelper::dereferenceConnGroup(const MeshMember& user)
    {
        if (EA_UNLIKELY((user.getMeshEndpoint() == nullptr) || (user.getMeshEndpoint()->getMesh() == nullptr)))
        {
            BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Internal error: Reference cannot be decremented as user(%" PRIi64 ")'s mesh endpoint was nullptr.\n", user.getId());
            return false;
        }
        uint64_t connGroup = user.getMeshEndpoint()->getConnectionGroupId();
        ConnectionGroupMap::iterator iter = mConnectionGroupMap.find(connGroup);
        if (iter != mConnectionGroupMap.end())
        {
            MeshIdMemberIdPairSet::key_type key(user.getMeshEndpoint()->getMesh()->getId(), user.getId());
            iter->second.erase(key);
            if (!iter->second.empty())
            {
                BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Reference decremented to %" PRIsize " for connection group(%" PRIu64 "), on user(%" PRIi64 ") leaving mesh(%" PRIu64 ").\n", (size_t)iter->second.size(), connGroup, key.second, key.first); 
                return false;
            }
            else
            {
                mConnectionGroupMap.erase(iter);
                BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Reference removed for connection group(%" PRIu64 "), on user(%" PRIi64 ") leaving mesh(%" PRIu64 ").\n", connGroup, key.second, key.first); 
                return true;
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Unknown connection group(%" PRIu64 ") being dereferenced!\n", connGroup);
            return false;
        }

    }

    void NetworkMeshHelper::initNetworkMesh(Mesh *mesh, const NetworkMeshAdapter::Config &config)
    {
        // need to add the local mesh members here. note non-active members get added when they go to active.
        // also add references for other users already in the game
        // NOTE: we're assuming the adapter will add any users already in the game properly.
        for (uint16_t index=0; index < mesh->getMeshMemberCount(); ++index)
        {
            const MeshMember *currentMember = mesh->getMeshMemberByIndex(index);

            if (currentMember != nullptr)
            {
                referenceConnGroup(*currentMember);
            }
        }
        mNetworkAdapter->initNetworkMesh(mesh, config);
    }


    bool NetworkMeshHelper::disconnectFromUser(const MeshMember* localUser, const MeshMember* user)
    {
        // We're disconnecting from a non-local member or we're the user disconnecting. In both cases,
        // we need to decrement the ref count for the conn group
        // localUser may be null if we're a dedicated server topology host. In this case disconnect from the user.
        if ( ((localUser != nullptr) && (user != nullptr) && (localUser->getId() != user->getId())) 
            || (localUser == user) 
            || ((localUser == nullptr) && (user != nullptr)))
        {
            if (dereferenceConnGroup(*user))
            {

                mNetworkAdapter->disconnectFromEndpoint(user->getMeshEndpoint());
                return true;
            }
            else
            {
                mNetworkAdapter->removeMemberOnEndpoint(user);
                return false;
            }
        }
        // return true here, because there were no connections
        return true;
    }

    bool NetworkMeshHelper::removeMemberFromMeshEndpoint(const MeshMember* user)
    {
        if (user != nullptr)
        {
            if (dereferenceConnGroup(*user))
            {
                return true;
            }
            else
            {
                mNetworkAdapter->removeMemberOnEndpoint(user);
                return false;
            }
        }
        return false;
    }

    bool NetworkMeshHelper::connectToUser(const Mesh* mesh, const MeshMember* localUser, const MeshMember* user)
    {
        if (mesh == nullptr)
        {
            BlazeAssertMsg(mesh != nullptr,  "connectToUser received nullptr mesh pointer!");
            return false;
        }

        if ( (localUser != nullptr && (mesh->isTopologyHost() && (mesh->getTopologyHostId() == localUser->getId()))) || (localUser == nullptr && mesh->isTopologyHost()) )
        {
            // Player host, don't need to connect to ourself, should never happen as player host should call initGameNetwork in all cases.
            if (localUser == user)
            {
                BlazeAssertMsg(false,  "player host attempting to connect to themselves!");
                return false;
            }
        }

        // Connect to the joining player.
        BLAZE_SDK_DEBUGF("[NetworkMeshHelper] Local Endpoint (%" PRIu64 ") in Connection Slot %d used by Player(%" PRId64 ":%s) connecting to Endpoint (%" PRIu64 ") in Connection Slot %d used by Player(%" PRId64 ":%s) of Game(%" PRId64 ":%s) with network %s.\n",
            (((localUser != nullptr) && (localUser->getMeshEndpoint() != nullptr)) ? localUser->getMeshEndpoint()->getConnectionGroupId() : 0), 
            (((localUser != nullptr) && (localUser->getMeshEndpoint() != nullptr)) ? localUser->getMeshEndpoint()->getConnectionSlotId() : 0), 
            ((localUser != nullptr) ? localUser->getId() : 0), ((localUser != nullptr) ? localUser->getName() : "n/a"),
            (((user != nullptr) && (user->getMeshEndpoint() != nullptr)) ? user->getMeshEndpoint()->getConnectionGroupId() : 0), 
            (((user != nullptr) && (user->getMeshEndpoint() != nullptr)) ? user->getMeshEndpoint()->getConnectionSlotId() : 0), 
            ((user != nullptr) ? user->getId() : 0), ((user != nullptr) ? user->getName() : "n/a"),
            mesh->getId(), mesh->getName(), GameNetworkTopologyToString(mesh->getNetworkTopology()));

        if (referenceConnGroup(*user))
        {
            mNetworkAdapter->connectToEndpoint(user->getMeshEndpoint());
            return true;
        }
        else
        {
            mNetworkAdapter->addMemberOnEndpoint(user);
            return false;
        }
    }

} // namespace BlazeNetworkAdapter
} // namespace Blaze
