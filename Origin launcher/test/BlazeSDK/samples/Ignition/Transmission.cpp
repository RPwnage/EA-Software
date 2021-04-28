
#include "Transmission.h"

#include "Ignition/Ignition.h"
#include "BlazeSDK/gamemanager/game.h"
#include "BlazeSDK/gamemanager/player.h"

namespace Ignition
{

TransmissionClient::TransmissionClient()
:   pLink(nullptr),
    pDist(nullptr),
    pLinkStream(nullptr)
{
}

TransmissionGame::TransmissionGame()
{
    Clear();
}

void TransmissionGame::Idle()
{
    uint32_t now = NetTick();
    uint32_t uVer;
    uint32_t uNewCRC;
    int32_t iIndex;
    
    for (iIndex = 0; iIndex < TransmissionGame::TRANSMISSION_MAX_CLIENTS; iIndex++)
    {
        if (mClients[iIndex].pLinkStream)
        {
            NetGameLinkUpdate(mClients[iIndex].pLink);
        }
    }

    if ((unsigned)NetTickDiff(now, mLastTimeStream) > mRateStream)
    {
        for (iIndex = 0; iIndex < TransmissionGame::TRANSMISSION_MAX_CLIENTS; iIndex++)
        {
            if (mClients[iIndex].pLinkStream)
            {
                mClients[iIndex].pLinkStream->Send(mClients[iIndex].pLinkStream, 0, 'kind', (void*)"test", 5);
            }
        }

        mLastTimeStream = now;
    }

    if (miHostSlot >= 0 && mClients[miHostSlot].pDist)
    {
        uint8_t aTypes[TRANSMISSION_MAX_CLIENTS];
        uint8_t uType;
        static char aInputBuffers[TRANSMISSION_MAX_CLIENTS][1024];
        void *aInputs[TRANSMISSION_MAX_CLIENTS];
        int32_t aLens[TRANSMISSION_MAX_CLIENTS];
        int32_t iRet;
        int32_t iSend, iRecv;

        for (iIndex = 0; iIndex < TRANSMISSION_MAX_CLIENTS; iIndex++)
        {
            aInputs[iIndex] = aInputBuffers[iIndex];
        }

        uType = GMDIST_DATA_INPUT_DROPPABLE;

        // send current tick as payload. Just as good as randomness.
        memcpy(aInputs[0], &now, sizeof(now));
        aLens[0] = sizeof(now);

        NetGameDistInputCheck(mClients[miHostSlot].pDist, &iSend, &iRecv);

        if (iSend == 0)
        {
            NetGameDistInputLocalMulti(mClients[miHostSlot].pDist, &uType, &aInputs[0], &aLens[0], 0);
        }

        if (iRecv != 0)
        {
            do
            {
                iRet = NetGameDistInputQueryMulti(mClients[miHostSlot].pDist, aTypes, aInputs, aLens);

                if (iRet > 0)
                {
                    mNumDistInput++;

                    for (iIndex = 0; iIndex < TRANSMISSION_MAX_CLIENTS; iIndex++)
                    {
                        if (((aTypes[iIndex] & ~GMDIST_DATA_CRC_REQUEST) != GMDIST_DATA_NONE) &&
                            ((aTypes[iIndex] & ~GMDIST_DATA_CRC_REQUEST) != GMDIST_DATA_INPUT_DROPPABLE) &&
                            ((aTypes[iIndex] & ~GMDIST_DATA_CRC_REQUEST) != GMDIST_DATA_NODATA))
                        {
                            NetPrintf(("transmission: received invalid packet, type is %u\n", aTypes[iIndex]));
                        }
                        if ((aTypes[iIndex] == GMDIST_DATA_INPUT_DROPPABLE) &&
                            (aLens[iIndex] != sizeof(uint32_t)))
                        {
                            NetPrintf(("transmission: received invalid packet, size is %u\n", aLens[iIndex]));
                        }
                    }

                    uVer = (uint32_t)NetGameDistStatus(mClients[miHostSlot].pDist, 'qver', 0, nullptr, 0);

                    if (uVer != mLastVersion)
                    {
                        NetPrintf(("transmission: client change. Resetting CRC for game %d at %d netgamedist inputs (uVer is %d, mLastVersion is %d)\n", mGame->getId(), mNumDistInput, uVer, mLastVersion));
                        NetPrintf(("transmission: CRC before reset is %u\n", mCRC));
                        mLastVersion = uVer;
                        mCRC = 0;
                    }

                    uNewCRC = ComputeCRC(aTypes, aInputs, aLens);

                    mCRC = mCRC * TRANSMISSION_CRC_MULTIPLY + uNewCRC;

                    if ((mNumDistInput % 100) == 0)
                    {
                        NetGameLinkStatT Stat;
                        NetGameLinkStatus(mClients[miHostSlot].pLink, 'stat', 0, &Stat, sizeof(NetGameLinkStatT));

                        NetPrintf(("transmission: game %d got %d netgamedist inputs\n", mGame->getId(), mNumDistInput));
                        NetPrintf(("transmission: Stat.ping is %d\n", Stat.ping));
                    }
                }
                else if (iRet < 0)
                {
                    NetPrintf(("transmission: game %d got %d netgamedist error\n", mGame->getId(), iRet));
                }
            }
            while(iRet > 0);
        }
    }

    mLastTime = now;
}

void TransmissionGame::SetupClients(int32_t iSelf, int32_t iMaxClient)
{
    if (mSelfIndex != iSelf || mTotalClients != iMaxClient)
    {
        mSelfIndex = iSelf;
        mTotalClients = iMaxClient;

        if (miHostSlot >=0 && mClients[miHostSlot].pDist)
        {
            NetGameDistMultiSetup(mClients[miHostSlot].pDist, mSelfIndex, TRANSMISSION_MAX_CLIENTS);
            NetGameDistInputRate(mClients[miHostSlot].pDist, mRate);
        }
    }
}

void TransmissionGame::AddClient(int32_t iIndex, NetGameLinkRefT *pLink)
{
    if (mClients[iIndex].pLink == nullptr)
    {
        mClients[iIndex].pLink = pLink;
        NetPrintf(("transmission: added netgamelink 0x%08x for client %d in game %d\n", pLink, iIndex, mGame->getId()));

        mClients[iIndex].pDist = NetGameDistCreate(pLink,
            (NetGameDistStatProc *)NetGameLinkStatus,
            (NetGameDistSendProc *)NetGameLinkSend,
            (NetGameDistRecvProc *)NetGameLinkRecv,
            GMDIST_DEFAULT_BUFFERSIZE_IN * 10,
            GMDIST_DEFAULT_BUFFERSIZE_OUT * 20);

        NetGameDistSetProc(mClients[iIndex].pDist, 'peek', (void *)NetGameLinkPeek2);

        mClients[iIndex].pLinkStream = NetGameLinkCreateStream(pLink, TRANSMISSION_MAX_CLIENTS, 'str0', 10000, 10000, 10000);
        if (mClients[iIndex].pLinkStream)
        {
            mClients[iIndex].pLinkStream->Recv = &StreamRecvHook;
            mClients[iIndex].pLinkStream->pRefPtr = this;
        }
    }
    else if (mClients[iIndex].pLink == pLink)
    {
        NetPrintf(("transmission: already got the same netgamelink for client %d\n", iIndex));
    }
    else
    {
        NetPrintf(("transmission: got a different netgamelink for client %d (0x%08x, 0x%08x)\n", iIndex, mClients[iIndex].pLink, pLink));
    }
}

void TransmissionGame::AddGameServer(int32_t iIndex, NetGameLinkRefT *pLink)
{
    AddClient(iIndex, pLink);

    // todo: amakoukji: this line is no good because mSelfIndex is not set yet
    //NetGameDistMultiSetup(mClients[iIndex].pDist, mSelfIndex, TRANSMISSION_MAX_CLIENTS);
}

void Transmission2::FlowControlEnable(Blaze::GameManager::Game *pGame)
{
    TransmissionGame* pTransmissionGame = GetGame(pGame);

    if (!pTransmissionGame)
    {
        NetPrintf(("transmission: could not find or create TransmissionGame\n"));
        return;
    }

    if (pTransmissionGame->mClients[pGame->getTopologyHostConnectionSlotId()].pDist)
    {
        NetGameDistControl(pTransmissionGame->mClients[pGame->getTopologyHostConnectionSlotId()].pDist, 'lsnd', TRUE, nullptr);
        NetGameDistControl(pTransmissionGame->mClients[pGame->getTopologyHostConnectionSlotId()].pDist, 'lrcv', TRUE, nullptr);
    }
}

void Transmission2::FlowControlDisable(Blaze::GameManager::Game *pGame)
{
    TransmissionGame* pTransmissionGame = GetGame(pGame);

    if (!pTransmissionGame)
    {
        NetPrintf(("transmission: could not find or create TransmissionGame\n"));
        return;
    }
    
    if (pTransmissionGame->mClients[pGame->getTopologyHostConnectionSlotId()].pDist)
    {
        NetGameDistControl(pTransmissionGame->mClients[pGame->getTopologyHostConnectionSlotId()].pDist, 'lsnd', FALSE, nullptr);
        NetGameDistControl(pTransmissionGame->mClients[pGame->getTopologyHostConnectionSlotId()].pDist, 'lrcv', FALSE, nullptr);
    }
}

void TransmissionGame::StreamRecv(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen)
{
    mNumStreamMessage++;

    if ((mNumStreamMessage % 100) == 0)
    {
        NetPrintf(("transmission: game %d got %d stream packets\n", mGame->getId(), mNumStreamMessage));
    }
}

void TransmissionGame::StreamRecvHook(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen)
{
    static_cast<TransmissionGame*>(pStream->pRefPtr)->StreamRecv(pStream, iSubchan, iKind, pBuffer, iLen);
}

void TransmissionGame::Clear()
{
    mGame = nullptr;
    memset(mClients, 0, sizeof(mClients));
    mUseNetGameDist = false;
    miHostSlot = -1;
    mLastTime = 0;
    mRate = 33;
    mLastTimeStream = 0;
    mRateStream = 500;
    mNumStreamMessage = 0;
    mNumDistInput = 0;
    mSelfIndex = 0;
    mTotalClients = 1;
    mLastVersion = (uint32_t)-1;
    mCRC = 0;
}

uint32_t TransmissionGame::ComputeCRC(uint8_t aTypes[], void *aInputs[], int32_t aLens[])
{
    int32_t iIndex;
    uint32_t uCRC = 0;
    uint32_t uFirtFourBytes;

    for(iIndex = 0; iIndex < TRANSMISSION_MAX_CLIENTS; iIndex++)
    {
        uCRC = TRANSMISSION_CRC_MULTIPLY * uCRC + aTypes[iIndex]; 
        uCRC = TRANSMISSION_CRC_MULTIPLY * uCRC + aLens[iIndex];

        if ((aTypes[iIndex] == GMDIST_DATA_INPUT) || (aTypes[iIndex] == GMDIST_DATA_INPUT_DROPPABLE))
        {
            memcpy(&uFirtFourBytes, aInputs[iIndex], sizeof(uFirtFourBytes));
            uCRC = TRANSMISSION_CRC_MULTIPLY * uCRC + uFirtFourBytes;
        }
    }

    return(uCRC);
}

Transmission2::Transmission2()
{
    mTransmission = new Transmission(); 
    mUseNetGameDist = false;
    miHostSlot = -1;
}

Transmission2::~Transmission2()
{
    delete mTransmission;
}

Transmission * Transmission2::GetTransmission()
{
    return mTransmission;
}

void Transmission2::SetupClients(Blaze::GameManager::Game *pGame, int32_t iSelf, int32_t iMaxClient)
{
    TransmissionGame* pTransmissionGame = GetGame(pGame);

    if (!pTransmissionGame)
    {
        NetPrintf(("transmission: could not find or create TransmissionGame\n"));
        return;
    }

    pTransmissionGame->SetUseNetGameDist(mUseNetGameDist);
    pTransmissionGame->SetHostIndex(miHostSlot);
    pTransmissionGame->SetupClients(iSelf, iMaxClient);
}


void Transmission2::AddClient(int32_t iIndex, NetGameLinkRefT *pLink, Blaze::GameManager::Game *pGame)
{
    TransmissionGame* pTransmissionGame = GetGame(pGame);

    if (!pTransmissionGame)
    {
        NetPrintf(("transmission: could not find or create TransmissionGame\n"));
        return;
    }

    if (pLink)
    {
        pTransmissionGame->AddClient(iIndex, pLink);

        if (pTransmissionGame->mClients[iIndex].pDist != NULL)
        {
            ConnApiRefT *pConnApi = ((Blaze::BlazeNetworkAdapter::ConnApiAdapter *)mNetworkAdapter)->getConnApiRefT(pGame);            
            ConnApiControl(pConnApi, 'dist', iIndex, 0, pTransmissionGame->mClients[iIndex].pDist);
        }
    }
}

void Transmission2::AddGameServer(int32_t iIndex, NetGameLinkRefT *pLink, Blaze::GameManager::Game *pGame)
{
    TransmissionGame* pTransmissionGame = GetGame(pGame);

    if (!pTransmissionGame)
    {
        NetPrintf(("transmission: could not find or create TransmissionGame\n"));
        return;
    }

    if (pLink)
    {
        pTransmissionGame->AddGameServer(iIndex, pLink);
    }
}

void Transmission2::RemoveClient(int32_t iIndex, NetGameLinkRefT *pLink, Blaze::GameManager::Game *pGame)
{
    TransmissionGame* pTransmissionGame = GetGame(pGame);

    if (!pTransmissionGame)
    {
        NetPrintf(("transmission: could not find TransmissionGame for client removal\n"));
        return;
    }

    if (pTransmissionGame->mClients[iIndex].pLink == nullptr)
    {
        NetPrintf(("transmission: nullptr netgamelink for current game, already removed?\n"));
    }
    else if (pLink != nullptr)
    {

        if (pTransmissionGame->mClients[iIndex].pLink != pLink)
        {
            NetPrintf(("transmission: mismatched netgamelink 0x%08x, 0x%08x for client %d in game %d at removal\n", pTransmissionGame->mClients[iIndex].pLink, pLink, iIndex, pGame->getId()));
        }
        NetPrintf(("transmission: removing netgamelink 0x%08x for client %d in game %d\n", pLink, iIndex, pGame->getId()));
    }

    if (pTransmissionGame->mClients[iIndex].pDist)
    {
        NetGameDistDestroy(pTransmissionGame->mClients[iIndex].pDist);
        pTransmissionGame->mClients[iIndex].pDist = nullptr;
        
        ConnApiRefT *pConnApi = ((Blaze::BlazeNetworkAdapter::ConnApiAdapter *)mNetworkAdapter)->getConnApiRefT(pGame);
        ConnApiControl(pConnApi, 'dist', iIndex, 0, NULL);
    }

    if (pTransmissionGame->mClients[iIndex].pLinkStream)
    {
        NetGameLinkDestroyStream(pTransmissionGame->mClients[iIndex].pLink, pTransmissionGame->mClients[iIndex].pLinkStream);
        pTransmissionGame->mClients[iIndex].pLinkStream = nullptr;
    }

    pTransmissionGame->mClients[iIndex].pLink = nullptr;

    return;
}

void Transmission2::RemoveGame(Blaze::GameManager::Game *pGame)
{
    TransmissionGame* game = GetGame(pGame);

    if (game)
    {
        int32_t iIndex;
        
        for(iIndex = 0; iIndex < TransmissionGame::TRANSMISSION_MAX_CLIENTS; iIndex++)
        {
            if (game->mClients[iIndex].pLink)
            {
                RemoveClient(iIndex, game->mClients[iIndex].pLink, pGame);
            }
        }

        game->Clear();
    }
    else
    {
        NetPrintf(("transmission: notified of game removal for a game we don't know of (%d, 0x%08x)\n", pGame->getId(), pGame));
    }

    NetPrintf(("transmission: removing game (%d, 0x%08x)\n", pGame->getId(), pGame));

}

void Transmission2::RemoveAllGamesOnDeauthentication()
{
    int32_t iIndex;

    NetPrintf(("transmission: removing all games on deauthentication\n"));

    // clears all games after state transitions from AUTHENTICATED
    for(iIndex = 0; iIndex < TRANSMISSION_MAX_GAMES; iIndex++)
    {
        if (mGames[iIndex].mGame)
        {
            mGames[iIndex].Clear();
        }
    }

    // additional cleanup not needed, as GameManagerAPI deletes associated objects silently.
}



void Transmission2::Idle()
{
   int32_t iIndex;

    for(iIndex = 0; iIndex < TRANSMISSION_MAX_GAMES; iIndex++)
    {
        if (mGames[iIndex].mGame)
        {
            mGames[iIndex].Idle();
        }
    }
    ((Blaze::BlazeNetworkAdapter::ConnApiAdapter*) GetNetworkAdapter())->flushTunnel();
}

void Transmission2::GetGameStats(Blaze::GameManager::Game *pGame, uint32_t &uNumStreamMessage, uint32_t &uNumDistInput)
{
    TransmissionGame* pTransmissionGame = GetGame(pGame);

    if (pTransmissionGame)
    {
        uNumStreamMessage = pTransmissionGame->mNumStreamMessage;
        uNumDistInput = pTransmissionGame->mNumDistInput;
    }
    else
    {
        uNumStreamMessage = 0;
        uNumDistInput = 0;
    }
}

TransmissionGame* Transmission2::GetGame(Blaze::GameManager::Game *pGame)
{
    int32_t iIndex;

    for(iIndex = 0; iIndex < TRANSMISSION_MAX_GAMES; iIndex++)
    {
        if (mGames[iIndex].mGame == pGame)
        {
            return(&mGames[iIndex]);
        }
    }

    for(iIndex = 0; iIndex < TRANSMISSION_MAX_GAMES; iIndex++)
    {
        if (!mGames[iIndex].mGame)
        {
            mGames[iIndex].mGame = pGame;
            mGames[iIndex].mLastTime = NetTick();
            mGames[iIndex].mLastTimeStream = NetTick();
            return(&mGames[iIndex]);
        }
    }

    return(nullptr);
}




Transmission::Transmission()
{
}

/*! ****************************************************************************/
/*! \brief Send game data to all valid netgamelinks associated with the game. A server-based game
           will have only one such active link. A peer-web game will have one such active link
           per client. A failover topology may have a mix (server netgamelink valid and some
           direct peer-to-peer netgamelink valid).

    \param game                The game
    \param buf                 buffer containing data to be sent
    \param bufSize             number of valid bytes to be sent
    \param skippedConnGroupId  id of connection group to not sent to (most probably because it is the originator in a rebroadcasting context)
    \return none
********************************************************************************/
void Transmission::SendToAll(Blaze::GameManager::Game *game, const char8_t *buf, int bufSize, bool bReliable, Blaze::BlazeId skippedConnGroupId)
{
    int32_t iResult;

    // prepare the NetGameLink packet
    NetGameMaxPacketT netGameLinkPacket;
    memset(&netGameLinkPacket, 0, sizeof(netGameLinkPacket));
    netGameLinkPacket.head.kind = (uint8_t)(bReliable?GAME_PACKET_USER:GAME_PACKET_USER_UNRELIABLE);
    netGameLinkPacket.head.len = (uint16_t)bufSize;
    if (bufSize <= (int)sizeof(netGameLinkPacket.body.data))
    {
        ds_memcpy(&netGameLinkPacket.body.data, buf, sizeof(netGameLinkPacket.body.data));
    }
    else
    {
        NetPrintf(("transmisison: packet too big for transmission (%d vs %d)\n", bufSize, sizeof(netGameLinkPacket.body.data)));
        return;
    }

    // query connapi from the connapi adapter
    ConnApiRefT *pConnApi = ((Blaze::BlazeNetworkAdapter::ConnApiAdapter*) GetNetworkAdapter())->getConnApiRefT(game);

    if (pConnApi != nullptr)
    {
        // now attempt to send directly to remote peers for which there also exists a valid netgamelink (excluding the skipped client)
        const ConnApiClientListT * pConnApiClientList = ConnApiGetClientList(pConnApi);
        for (int32_t iClientIndex = 0; iClientIndex < pConnApiClientList->iMaxClients; iClientIndex++)
        {
            if (pConnApiClientList->Clients[iClientIndex].pGameLinkRef != nullptr && pConnApiClientList->Clients[iClientIndex].ClientInfo.uId != static_cast<uint32_t>(skippedConnGroupId))
            {
                if ((iResult = NetGameLinkSend(pConnApiClientList->Clients[iClientIndex].pGameLinkRef, (NetGamePacketT *)&netGameLinkPacket, 1)) <= 0)
                {
                    NetPrintf(("transmission: failed to send netgamelink packet to client %d  (err = %d)\n", iClientIndex, iResult));
                }
            }
        }
    }
}


/*! ****************************************************************************/
/*! \brief Receive game data from all valid netgamelinks associated with the game. A server-based game
           will have only one such active link. A peer-web game will have one such active link
           per client. A failover topology may have a mix (server netgamelink valid and some
           direct peer-to-peer netgamelink valid).

    \param game     The game

    \return none
********************************************************************************/
void Transmission::ReceiveFromAll(Blaze::GameManager::Game *game, ReceiveHandlerCb callback)
{
    // early out if we're not active in the game or the dedicated server
    if (game->getNetworkTopology() == Blaze::NETWORK_DISABLED)
    {
        return;
    }
    if (((game->getLocalMeshEndpoint() == nullptr) || !game->getLocalMeshEndpoint()->isDedicatedServerHost()) 
        && ((game->getLocalPlayer() == nullptr) || (game->getLocalPlayer()->getPlayerState() != Blaze::GameManager::ACTIVE_CONNECTED)))
    {
        return;
    }

    int32_t iResult;

    // prepare the NetGameLink packet
    NetGameMaxPacketT netGameLinkPacket;

    // query connapi from the connapi adapter
    ConnApiRefT *pConnApi = ((Blaze::BlazeNetworkAdapter::ConnApiAdapter*) GetNetworkAdapter())->getConnApiRefT(game);

    if (pConnApi != nullptr)
    {
        // now attempt to received directly from remote peers for which there also exists a valid netgamelink
        const ConnApiClientListT * pConnApiClientList = ConnApiGetClientList(pConnApi);
        for (int32_t iClientIndex = 0; iClientIndex < pConnApiClientList->iMaxClients; iClientIndex++)
        {
            if (pConnApiClientList->Clients[iClientIndex].pGameLinkRef != nullptr)
            {
                if ((iResult = NetGameLinkRecv(pConnApiClientList->Clients[iClientIndex].pGameLinkRef, (NetGamePacketT*)&netGameLinkPacket, 1, FALSE)) > 0)
                {
                    callback(game, (char *)netGameLinkPacket.body.data, netGameLinkPacket.head.len);
                }
                else if (iResult < 0)
                {
                    NetPrintf(("transmission: failed to receive netgamelink packet from client %d  (err = %d)\n", iClientIndex, iResult));
                }
            }
        }
    }
}

}
