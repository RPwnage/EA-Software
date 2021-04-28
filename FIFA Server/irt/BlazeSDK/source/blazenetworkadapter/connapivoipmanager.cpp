/*************************************************************************************************/
/*!
    \file connapivoipmanager.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazenetworkadapter/connapivoipmanager.h"
#include "BlazeSDK/gamemanager/game.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/util/utilapi.h" // for UtilAPI in networkMeshCreated

#include "DirtySDK/voip/voiptunnel.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"

namespace Blaze
{

ConnApiVoipManager::ConnApiVoipManager(BlazeHub *blazeHub, const Mesh *mesh, uint32_t voipPort, MemoryGroupId memGroupId) :
    mMesh(mesh),
    mBlazeHub(blazeHub),
    mVoipTunnelRef(nullptr),
    mVoipPort(voipPort),
    mMaxClients(static_cast<const GameManager::Game *>(mesh)->getMaxPlayerCapacity()),
    mMemGroup(memGroupId)
{
    mBlazeHub->addIdler(this, "ConnApiVoipManager");

    if (mVoipTunnelRef == nullptr)
    {
        // virtualize the voip port
        NetConnControl('vadd', mVoipPort, 0, nullptr, nullptr);

        // create the voiptunnel module with number of clients and hosting 1 game
        DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(mMemGroup));
        mVoipTunnelRef = VoipTunnelCreate(mVoipPort, mMaxClients, 1);
        DirtyMemGroupLeave();

        if (mVoipTunnelRef == nullptr)
        {
            BLAZE_SDK_DEBUGF("ConnApiVoipManager: Failed to create VoipTunnel instance on port %u (maxClients=%u) voip will not function!\n", mVoipPort, mMaxClients);

            // devirtualize the voip port
            NetConnControl('vdel', mVoipPort, 0, nullptr, nullptr);
        }
        else
        {
            //use config overrides found in util.cfg
            Blaze::Util::UtilAPI::createAPI(*mBlazeHub);
            mBlazeHub->getUtilAPI()->OverrideConfigs(mVoipTunnelRef);

            //  add game to tunnel game list.
            if (VoipTunnelGameListAdd(mVoipTunnelRef, 0) < 0)
            {
                BLAZE_SDK_DEBUGF("ConnApiVoipManager: Failed VoipTunnelGameListAdd for MeshId=%" PRIu64 " on port %u (maxClients=%u) voip will not function!\n", mesh->getId(), mVoipPort, mMaxClients);
            }

            // add the clients already in the mesh to the voiptunnel
            for (uint16_t uMeshEndpointIndex = 0; uMeshEndpointIndex < mesh->getMeshEndpointCount(); uMeshEndpointIndex += 1)
            {
                const MeshEndpoint *pEndpoint = mesh->getMeshEndpointByIndex(uMeshEndpointIndex);
                if (!pEndpoint->isLocal())
                {
                    connectToEndpoint(pEndpoint);
                }
            }
        }
    }
}


ConnApiVoipManager::~ConnApiVoipManager()
{
    if (mVoipTunnelRef != nullptr)
    {
        //  clear out game from voip tunnel and our internal mesh list.
        VoipTunnelGameListDel(mVoipTunnelRef, 0);

        //  reset mesh list so that next game network create recreates the voip tunnel instance.
        VoipTunnelDestroy(mVoipTunnelRef);
        mVoipTunnelRef = nullptr;

        // devirtualize the voip port
        NetConnControl('vdel', mVoipPort, 0, nullptr, nullptr);
    }

    mBlazeHub->removeIdler(this);
}

void ConnApiVoipManager::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    if (mVoipTunnelRef != nullptr)
    {
        VoipTunnelUpdate(mVoipTunnelRef);
    }
}

void ConnApiVoipManager::refreshSendLists(const Blaze::Mesh *pMesh)
{
    uint16_t uMeshEndpointCount = pMesh->getMeshEndpointCount();
    uint16_t iIndex, iNumClients;
    uint32_t aClientIds[VOIPTUNNEL_MAXGROUPSIZE];

    // note: this functions assumes  that the client at index 0 is the dedicated server
    if (uMeshEndpointCount > (VOIPTUNNEL_MAXGROUPSIZE))
    {
        BLAZE_SDK_DEBUGF("ConnApiVoipManager: critical error - SendList refresh skipped because mesh size (%d) is bigger than voiptunnel client list maximum (%d)\n", uMeshEndpointCount, VOIPTUNNEL_MAXGROUPSIZE);
        return;
    }

    memset(aClientIds, 0, sizeof(aClientIds));
    iNumClients = 0;

    // loop through the clients, if we get a valid client id add it to the list
    // the server returns 0 so is not added
    for(iIndex = 0; iIndex < VOIPTUNNEL_MAXGROUPSIZE; iIndex++)
    {
        const MeshEndpoint *pEndpoint = pMesh->getMeshEndpointByIndex(iIndex);

        // ignore the endpoint that corresponds to us, the dedicated server
        if (pEndpoint && !pEndpoint->isLocal())
        {
            BlazeAssert(!pEndpoint->isDedicatedServerHost());
            BlazeAssert(pEndpoint->getConnectionSlotId() != 0);
            aClientIds[pEndpoint->getConnectionSlotId()-1] = GetClientIdFromEndpoint(pEndpoint);
            iNumClients++;
        }
    }

    // for each client, find them in the voiptunnel and refresh their
    // send list based on the same list of clients
    for(iIndex = 0; iIndex < VOIPTUNNEL_MAXGROUPSIZE; iIndex++)
    {
        // skip holes in the list
        if (aClientIds[iIndex] != 0)
        {
            VoipTunnelClientT *pClient;
            if ((pClient = VoipTunnelClientListMatchId(mVoipTunnelRef, aClientIds[iIndex])) != nullptr)
            {
                pClient->iNumClients = iNumClients;
                memcpy(pClient->aClientIds, aClientIds, sizeof(pClient->aClientIds));

                VoipTunnelClientRefreshSendMask(mVoipTunnelRef, pClient);
            }
        }
    }
}

uint32_t ConnApiVoipManager::GetClientIdFromEndpoint(const Blaze::MeshEndpoint *pEndpoint)
{
    if (pEndpoint->isLocal())
    {
        return(0);
    }
    else
    {
        return(pEndpoint->getRemoteLowLevelConnectivityId());
    }

}

void ConnApiVoipManager::connectToEndpoint(const Blaze::MeshEndpoint *pEndpoint)
{
    //  player has joined the voip network - add his player (key = endpoint blazeId)
    if (mVoipTunnelRef != nullptr)
    {
        uint32_t clientId=0;
        VoipTunnelClientT *clientRef;

        clientId = GetClientIdFromEndpoint(pEndpoint);

        clientRef = VoipTunnelClientListMatchId(mVoipTunnelRef, clientId);

        if (clientRef == nullptr)
        {
            VoipTunnelClientT client;
            memset(&client, 0, sizeof(client));
            client.iNumClients = 0;
            client.uClientId = clientId;
            client.uRemoteAddr = 0;
            client.iGameIdx = 0;

            // add client to the voip tunnel at its corresponding connection slot -1 to account for the server being part of the ConnApi client list
            // we offset so that we do not lose the voip slot, which supports a maximum of 32 clients
            int32_t ret = VoipTunnelClientListAdd2(mVoipTunnelRef, &client, &clientRef, pEndpoint->getConnectionSlotId() - 1);

            if (ret < 0)
            {
                BLAZE_SDK_DEBUGF("ConnApiVoipManager: attempt to add peer id=0x%08x to tunnel client list failed (%d).\n", clientId, ret);
            }
        }
        else
        {
            BlazeVerify(false);
            BLAZE_SDK_DEBUGF("ConnApiVoipManager: attempting to add existing peer id=0x%08x into tunnel client list.\n", clientId);
        }

        refreshSendLists(pEndpoint->getMesh());
    }
}

void ConnApiVoipManager::disconnectFromEndpoint(const Blaze::MeshEndpoint *pEndpoint)
{
    if (mVoipTunnelRef != nullptr)
    {
        uint32_t clientId=0;
        VoipTunnelClientT *clientRef;

        clientId = GetClientIdFromEndpoint(pEndpoint);

        clientRef = VoipTunnelClientListMatchId(mVoipTunnelRef, clientId);

        if (clientRef != nullptr)
        {
            BLAZE_SDK_DEBUGF("ConnApiVoipManager: disconnectedFromEndpoint clientId 0x%08x\n", clientId);
            VoipTunnelClientListDel(mVoipTunnelRef, clientRef, 0);
        }
        else
        {
            BLAZE_SDK_DEBUGF("ConnApiVoipManager: disconnectedFromEndpoint failed to match client with clientId 0x%08x\n", clientId);
        }
    }
}

} // namespace Blaze
