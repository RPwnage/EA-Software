/*! ****************************************************************************
    \file network.h


    \attention
        (c) Electronic Arts. All Rights Reserved.

*******************************************************************************/
#ifndef BLAZE_NETWORKADAPTER_NETWORK_H
#define BLAZE_NETWORKADAPTER_NETWORK_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/internal/blazenetworkadapter/clientconnstats.h"
#include "BlazeSDK/mesh.h"

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/game/connapi.h"
#include "DirtySDK/voip/voipdef.h"

#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"

namespace Blaze
{
    namespace BlazeNetworkAdapter
    {

class ConnApiAdapterConfig;

class Network
{

public:
    virtual ~Network();
    static Network* createNetwork();
    void destroyNetwork();

    ConnApiRefT* getConnApiRefT() const;
    const ConnApiClientT* getClientHandleForEndpoint(const MeshEndpoint* meshEndpoint) const;
    ClientConnStats *getConnApiStatsForEndpoint(const MeshEndpoint *meshEndpoint);
    bool initNetworkMesh(const Mesh *pMesh, ConnApiAdapterData* pData, const NetworkMeshAdapter::Config &config);
    void connectToEndpoint(const MeshEndpoint *meshEndpoint);
    void addMemberOnEndpoint(const MeshMember *meshMember);
    void removeMemberOnEndpoint(const MeshMember *meshMember);
    void disconnectFromEndpoint(const MeshEndpoint *meshEndpoint);
    void resetGame();
    void migrateTopologyHost();
    void activateDelayedVoipOnEndpoint(const Blaze::MeshEndpoint* endpoint);
   
    void idleNetwork(const uint32_t currentTime, const uint32_t elapsedTime);
private:
    Network();
    Network& operator=(Network&);

    bool setupConnApi();
    void setupClients();
    bool setupTunnel();
    void setupDemangler();
    void setupMesh();

    void setupVoip();

    void activateNetwork();
    void onConnApiEvent(const ConnApiCbInfoT* pCbInfo);       // Adds event to deferred list
    void handleConnApiEvent(const ConnApiCbInfoT* pCbInfo);   // Processes events from the deferred list
    void onConnApiVoipEvent(const ConnApiCbInfoT* pCbInfo);
    void onConnApiGameEvent(const ConnApiCbInfoT* pCbInfo);

    bool isConnApiNeeded() const;

#if ENABLE_BLAZE_SDK_LOGGING
    void printMeshMembersForMeshEndpoint(const MeshEndpoint* pMeshEndpoint);
    void printConnApiClient(ConnApiClientInfoT *pUserInfo, const MeshEndpoint* pMeshEndpoint) const;
#endif

    void addConnApiClient(const MeshEndpoint* pMeshEndpoint);
    bool initConnApiClient(ConnApiClientInfoT *pUserInfo, const MeshEndpoint* pMeshEndpoint);
    void handleDeferredEvents();

    static void staticConnApiCallBack(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData);
    static const int32_t MAX_NUM_USERS = 255; // Limitations: https://eadpjira.ea.com/browse/GOS-32883?focusedCommentId=2004983&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-2004983 
                                              // todo, should be made dynamic

    ConnApiClientInfoT mConnapiClientList[MAX_NUM_USERS];
    uint64_t mConnectionGroupByIndex[MAX_NUM_USERS];
    ClientConnStats mConnApiConnClientStatsList[MAX_NUM_USERS];
    ConnApiRefT* mConnApi;
    const Mesh *mMesh;
    ConnApiAdapterData* mData;
    int32_t mClientListSize; // Number of clients in this mesh.
    uint32_t mServerTunnelPort;
    NetworkMeshAdapter::Config mNetworkMeshConfig;
    bool mParticipatingVoipLocalUsers[VOIP_MAXLOCALUSERS];
    typedef Blaze::list<ConnApiCbInfoT> DeferredEventList;
    DeferredEventList mDeferredEventList;
    ConnApiGameTopologyE mGameTopology;
    ConnApiVoipTopologyE mVoipTopology;
};

    }
}

#endif


