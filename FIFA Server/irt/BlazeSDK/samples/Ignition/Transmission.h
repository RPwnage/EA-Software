/*! ****************************************************************************
    \file Transmission.h
    \brief
    Low-tech helper class for sending and receiving data via ConnApiAdapter.
    Uses a header for sender and recipient id, and a best-effort, topology-agnostic approach.
    Knows about blaze objects (Game, Player, ConnApiAdapter) and DirtySock's DirtyCast.
    Makes no attempt at optimization or security; this code is designed for simplicity rather than production.

    \attention
        Copyright (c) Electronic Arts 2005-2008.  ALL RIGHTS RESERVED.

*******************************************************************************/

#ifndef TRANSMISSION_H
#define TRANSMISSION_H



#include "EASTL/map.h"

#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/callback.h"

namespace Ignition
{
class TransmissionClient
{
public:
    TransmissionClient();
    NetGameLinkRefT *pLink;
    NetGameDistRefT *pDist;
    NetGameLinkStreamT *pLinkStream;
};

class TransmissionGame
{
public:
    // just a large prime number. We CRC with CRC(n+1) = CRC(n) * TRANSMISSION_CRC_MULTIPLY + input(n)
    static const uint32_t TRANSMISSION_CRC_MULTIPLY = 982451653;
    static const int32_t TRANSMISSION_MAX_CLIENTS = 32;   // one client = a console

    TransmissionGame();
    void Clear();
    void Idle();

    void SetupClients(int32_t iSelf, int32_t iMaxClient);

    void AddClient(int32_t iIndex, NetGameLinkRefT *pLink);
    void AddGameServer(int32_t iIndex, NetGameLinkRefT *pLink);

    void StreamRecv(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen);

    uint32_t ComputeCRC(uint8_t aTypes[], void *aInputs[], int32_t aLens[]);

    void SetUseNetGameDist(bool bUse) { mUseNetGameDist = bUse; }
    void SetHostIndex(int32_t iHostSlot) { miHostSlot = iHostSlot; }

    static void StreamRecvHook(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen);


    Blaze::GameManager::Game *mGame;
    TransmissionClient mClients[TRANSMISSION_MAX_CLIENTS];
    uint32_t mLastTime;
    uint32_t mRate;
    uint32_t mLastTimeStream;
    uint32_t mRateStream;
    uint32_t mNumStreamMessage;
    uint32_t mNumDistInput;
    int32_t mSelfIndex;
    int32_t mTotalClients;
    uint32_t mLastVersion;
    uint32_t mCRC;
    bool mUseNetGameDist;
    int32_t miHostSlot;
};


class Transmission
{
public:
    typedef Blaze::Functor3<Blaze::GameManager::Game *, char8_t *, int> ReceiveHandlerCb;

public:
    Transmission();

    enum
    {
        MAX_PAYLOAD_SIZE = NETGAME_DATAPKT_MAXSIZE
    };

    void SendToAll(Blaze::GameManager::Game *game, const char8_t *buf, int bufSize, bool isReliable, Blaze::BlazeId skippedConnGroupId);
    void ReceiveFromAll(Blaze::GameManager::Game *game, ReceiveHandlerCb callback);

    Blaze::BlazeNetworkAdapter::NetworkMeshAdapter* GetNetworkAdapter() {return(mNetworkAdapter);};
    void SetNetworkAdapter(Blaze::BlazeNetworkAdapter::NetworkMeshAdapter* pAdapter) {mNetworkAdapter = pAdapter;};

private:
    Blaze::BlazeNetworkAdapter::NetworkMeshAdapter* mNetworkAdapter;
};


class Transmission2
{
public:
    static const int32_t TRANSMISSION_MAX_GAMES = 16;

    Transmission2();
    ~Transmission2();

    void SetupClients(Blaze::GameManager::Game *pGame, int32_t iSelf, int32_t iMaxClient);

    void AddClient(int32_t iIndex, NetGameLinkRefT *pLink, Blaze::GameManager::Game *pGame);
    void AddGameServer(int32_t iIndex, NetGameLinkRefT *pLink, Blaze::GameManager::Game *pGame);

    void RemoveClient(int32_t iIndex, NetGameLinkRefT *pLink, Blaze::GameManager::Game *pGame);

    void RemoveGame(Blaze::GameManager::Game *pGame);
    void Idle();

    void GetGameStats(Blaze::GameManager::Game *pGame, uint32_t &uNumStreamMessage, uint32_t &uNumDistInput);

    void FlowControlEnable(Blaze::GameManager::Game *game);
    void FlowControlDisable(Blaze::GameManager::Game *game);

    void SetUseNetGameDist(bool bUse) { mUseNetGameDist = bUse; }
    void SetHostIndex(int32_t iHostSlot) { miHostSlot = iHostSlot; }

    Blaze::BlazeNetworkAdapter::NetworkMeshAdapter* GetNetworkAdapter() {return(mNetworkAdapter);};
    void SetNetworkAdapter(Blaze::BlazeNetworkAdapter::NetworkMeshAdapter* pAdapter) 
    {
        mNetworkAdapter = pAdapter;
        mTransmission->SetNetworkAdapter(pAdapter);
    };

    Transmission *GetTransmission();

    void RemoveAllGamesOnDeauthentication();

private:
    Blaze::BlazeNetworkAdapter::NetworkMeshAdapter* mNetworkAdapter;
    Transmission* mTransmission;

    TransmissionGame* GetGame(Blaze::GameManager::Game *pGame);

    TransmissionGame mGames[TRANSMISSION_MAX_GAMES];

    bool mUseNetGameDist;
    int32_t miHostSlot;
};


typedef struct NetGameDistPrivateData
{
    uint32_t mMuteMask;
    bool mGameNeedsReset;
    bool mGameRegisteredIdler;
} NetGameDistPrivateData;

typedef void (*SyncedReceiveHandlerType)(NetGameDistPrivateData *privateData, char8_t **bufs, int32_t *bufSizes, uint8_t *types);



void _ReceiveSyncHandler(NetGameDistPrivateData *privateData, char8_t **bufs, int32_t *bufSizes, uint8_t *types);

}

#endif
