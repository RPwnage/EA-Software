/*! ****************************************************************************
    \file Server.cpp


    \attention
        Copyright (c) Electronic Arts 2011.  ALL RIGHTS RESERVED.

*******************************************************************************/

#include "Server.h"

// ANSI includes
#include <iostream>
#include <fstream>

// EA includes
#include <EABase/eabase.h>
#include <EAAssert/eaassert.h>
#include <eathread/eathread.h>
#include <EAStdC/EAScanf.h>
#include "EAStdC/EASprintf.h"

// Blaze includes
#include <BlazeSDK/blazehub.h>
#include <BlazeSDK/version.h>
#include <BlazeSDK/gamemanager/game.h>
#include <BlazeSDK/blazenetworkadapter/connapiadapter.h>
#include <BlazeSDK/gamemanager/gamemanagerapi.h>
#include <BlazeSDK/loginmanager/loginmanager.h>
#include <BlazeSDK/shared/framework/locales.h>
#include <BlazeSDK/loginmanager/loginmanager.h>

// DirtySock includes
#include "DirtySDK/dirtysock/netconn.h"

// App includes
#include "BaseGameListener.h"
#include "BaseGameManagerAPIListener.h"
#include "Utility.h"

// Windows includes
#ifdef EA_PLATFORM_WINDOWS
#include <conio.h>
#define strnicmp _strnicmp
#endif // EA_PLATFORM_WINDOWS

// These globals control the settings and behaviour.
// You'll need to adjust them; you'd probably want to use a settings file. But I don't know of any EA "settings file" package.
// I've allowed some of the params to be passed in on the commandline.
char            g_serviceName[512]              = "gosblaze-yournamehere-pc";
Blaze::EnvironmentType g_environment            = Blaze::ENVIRONMENT_SDEV;
char            g_localIp[1024]                 = "";
uint16_t        g_gamePort                      = 0;
const char *    g_accountEmail                  = "yourdedsvr@ea.com";
const char *    g_accountPassword               = "drowssap";
const uint16_t  g_maxPlayers                    = 64;
const char *    g_gameProtocolVersionString     = "Pyro";
char            g_certificateFile[1024]         = "";
char            g_keyFile[1024]                 = "";
bool            g_isBlazeLogging                = false;
bool            g_isDirtySockLogging            = false;
bool            g_isVoipEnabled                 = false;
bool            g_isReuseDedicatedServer        = true;
bool            g_isGameStateControlledByServer = false;

void ProcessCommandLine(int argc, char **argv);

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    ProcessCommandLine(argc, argv);

    Server * server = new Server();
    server->Run();
    delete server;

    CheckForMemoryLeaks();
}

//------------------------------------------------------------------------------
void ProcessCommandLine(int argc, char **argv)
{
    Log("ProcessCommandLine\n");

    for (int i = 0; i < argc; i++)
    {
        const char * param = argv[i];
        Log("  %s\n", param);

        if (EA::StdC::Strnicmp(param, "-serviceName=", EA::StdC::Strlen("-serviceName=")) == 0)
        {
            EA::StdC::Sscanf(param, "-serviceName=%s", g_serviceName);
        }
        if (EA::StdC::Strnicmp(param, "-environment=", EA::StdC::Strlen("-environment=")) == 0)
        {
            EA::StdC::Sscanf(param, "-environment=%d", &g_environment);
        }
        if (EA::StdC::Strnicmp(param, "-localIp=", EA::StdC::Strlen("-localIp=")) == 0)
        {
            EA::StdC::Sscanf(param, "-localIp=%s", g_localIp);
        }
        if (EA::StdC::Strnicmp(param, "-gamePort=", EA::StdC::Strlen("-gamePort=")) == 0)
        {
            //::sscanf(param, "-gamePort=%hu", &g_gamePort);
            EA::StdC::Sscanf(param, "-gamePort=%d", &g_gamePort);
        }
        if (EA::StdC::Strnicmp(param, "-certificateFile=", EA::StdC::Strlen("-certificateFile=")) == 0)
        {
            EA::StdC::Sscanf(param, "-certificateFile=%s", g_certificateFile);
        }
        if (EA::StdC::Strnicmp(param, "-keyFile=", EA::StdC::Strlen("-keyFile=")) == 0)
        {
            EA::StdC::Sscanf(param, "-keyFile=%s", g_keyFile);
        }
    }
}

//------------------------------------------------------------------------------
Server::Server(void) :
    mIsExitRequested(false),
    mIsGameCreated(false),
    mNetAdapter(nullptr),
    mBlazeHub(nullptr),
    mIsGameChangingState(false),
    mTargetFps(30),
    mCertData(nullptr),
    mKeyData(nullptr)
{
}

//------------------------------------------------------------------------------
void Server::Run(void)
{
    Init();

    CreateApis();

    // main loop
    while (!mIsExitRequested)
    {
        // Sleep a little, to achieve a target framerate
        EA::Thread::ThreadTime threadTime = 1000U / mTargetFps;
        EA::Thread::ThreadSleep(threadTime);

        // Tick DirtySock
        NetConnIdle();

        // Tick Blaze
        mBlazeHub->idle();

        ManageConnectionToBlazeServer();

        ManageGame();

#if defined EA_PLATFORM_WINDOWS && !defined NDEBUG
        if (::_kbhit())
        {
            mIsExitRequested = true;
        }
#endif
    }

    Close();
}

//------------------------------------------------------------------------------
void Server::Init(void)
{
    Log("Server::Init\n");

    Log("Blaze version = %s\n", Blaze::getBlazeSdkVersionString());
    Log("DirtySock version = %s\n", GetDirtySockVersionString());

#if DIRTYCODE_DEBUG
    NetPrintfHook(DirtySockLogFunction, nullptr);
#endif // DIRTYCODE_DEBUG

    NetConnStartup("-servicename=dedicatedserver");

    // This code demonstrates how to bind the dedicated server to a specific NIC, rather than to all
    // NIC's in a multihomed PC.
    // Note that BlazeHub::InitParameters::LocalInterfaceAddress is scheduled for deprecation.
    // Use netstat -a -o | find "<pid>" to verify the binding, where <pid> is the pid of DedicatedServer.exe.
    if (::strlen(g_localIp) > 0)
    {
        const int32_t serverIp = SocketInTextGetAddr(g_localIp);
        if (serverIp != 0)
        {
            Log("Using local IP %s\n", g_localIp);
            NetConnControl('ladr', serverIp, 0, nullptr, nullptr);
        }
    }

    EA::Allocator::ICoreAllocator * allocator = EA::Allocator::ICoreAllocator::GetDefaultAllocator();

    // Setup the options for starting up Blaze
    Blaze::BlazeHub::InitParameters initParameters;

    strncpy(initParameters.ServiceName, g_serviceName, sizeof(initParameters.ServiceName));
    strncpy(initParameters.ClientName, "DedicatedServer", sizeof(initParameters.ClientName));
    strncpy(initParameters.ClientVersion, "DedicatedServerVersion", sizeof(initParameters.ClientVersion));
    strncpy(initParameters.ClientSkuId, "DedicatedServerSkuId", sizeof(initParameters.ClientSkuId));
    initParameters.Locale       = Blaze::LOCALE_DEFAULT;
    initParameters.Client       = Blaze::CLIENT_TYPE_DEDICATED_SERVER;
    initParameters.Environment  = g_environment;
    initParameters.Secure       = true;

    if (g_gamePort != 0)
    {
        // Override the default gameport.
        // Typically in production you will need to specify the gameport for each dedicated server instance,
        // because those specific ports will need to be opened on the firewall.
        // Note that the GamePort is not bound until the game is reset, so ports must be carefully managed.
        initParameters.GamePort = g_gamePort;
        Log("Using GamePort %d\n", initParameters.GamePort);
    }

    int32_t certSize = 0;
    int32_t keySize = 0;
    if ((mCertData = loadCert(g_certificateFile, &certSize, "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----")) == nullptr)
    {
        Log("[Server::onConnected()] Failed to load certificate data!\n");
        EA_FAIL();
        exit(1);
    }

    if ((mKeyData = loadCert(g_keyFile, &keySize, "-----BEGIN RSA PRIVATE KEY-----", "-----END RSA PRIVATE KEY-----")) == nullptr)
    {
        Log("[Server::onConnected()] Failed to load key data!\n");
        EA_FAIL();
        exit(1);
    }

    Log("Blaze::BlazeHub::initialize with service name %s (%s)\n",
        g_serviceName, Blaze::EnvironmentTypeToString(g_environment));
    Blaze::BlazeError error = Blaze::BlazeHub::initialize(&mBlazeHub, initParameters, allocator, BlazeLogFunction, nullptr, mCertData, mKeyData);
    if (error == Blaze::ERR_OK)
    {
        mBlazeHub->addUserStateEventHandler(this);
        mBlazeHub->getLoginManager(0)->addListener(this);
    }
    else
    {
        Log("BlazeHub::initialize failed with error %s\n", DescribeError(error));
        EA_FAIL();
        exit(1);
    }
}

//------------------------------------------------------------------------------
int32_t Server::DirtySockLogFunction(void *pParm, const char *pText)
{
    if (g_isDirtySockLogging)
    {
        Log("DirtySock: %s", pText);
    }

    return 0;
}

//------------------------------------------------------------------------------
void Server::BlazeLogFunction(const char8_t *pText, void *data)
{
    if (g_isBlazeLogging)
    {
        Log("Blaze: %s", pText);
    }
}

//------------------------------------------------------------------------------
const char * Server::DescribeError(Blaze::BlazeError error)
{
    static char errorString[1024] = "";

    if (error == Blaze::ERR_OK)
    {
        ::_snprintf(errorString, sizeof(errorString), "");
    }
    else
    {
        if ((mBlazeHub != nullptr) && (::strlen(mBlazeHub->getErrorName(error)) > 0))
        {
            ::_snprintf(errorString, sizeof(errorString), "[error %s]", mBlazeHub->getErrorName(error));
        }
        else
        {
            ::_snprintf(errorString, sizeof(errorString), "[error %d]", error);
        }
    }

    return errorString;
}

//------------------------------------------------------------------------------
void Server::CreateApis(void)
{
    Log("Server::CreateApis\n");

    // Create the network adapter
    Blaze::BlazeNetworkAdapter::ConnApiAdapterConfig adapterConfig;
    EA_ASSERT(mNetAdapter == nullptr);
    mNetAdapter = new Blaze::BlazeNetworkAdapter::ConnApiAdapter(adapterConfig);

    Blaze::GameManager::GameManagerAPI::GameManagerApiParams params;
    params.mNetworkAdapter = mNetAdapter;
    params.mGameProtocolVersionString = g_gameProtocolVersionString;
    Blaze::GameManager::GameManagerAPI::createAPI(*mBlazeHub, params);

    Log("using mGameProtocolVersionString: %s\n", g_gameProtocolVersionString);

    mBlazeHub->getGameManagerAPI()->addListener(this);
}

//------------------------------------------------------------------------------
// Periodically check that we are connected
void Server::ManageConnectionToBlazeServer(void)
{
    // Note - the connection retry logic here assumes that dedicated servers are
    // not all launched at the same instant.
    const uint32_t timeNowMs = NetTick();
    static uint32_t timeLastMs = 0;
    if (timeNowMs - timeLastMs >= 15000)
    {
        if (!(mBlazeHub->getConnectionManager()->isConnected()))
        {
            Log("connect\n");
            mBlazeHub->getConnectionManager()->connect(MakeFunctor(this, &Server::ConnectionMessagesCb));
        }

        timeLastMs = timeNowMs;
    }
}

//------------------------------------------------------------------------------
void Server::ConnectionMessagesCb(Blaze::BlazeError error, const Blaze::Redirector::DisplayMessageList *messageList)
{
    Log("Server::ConnectionMessagesCb %s\n", DescribeError(error));

    Blaze::Redirector::DisplayMessageList::const_iterator msgListIter = messageList->begin();
    Blaze::Redirector::DisplayMessageList::const_iterator msgListEnd = messageList->end();
    for (; msgListIter != msgListEnd; ++msgListIter)
    {
        Log("%s\n", msgListIter->c_str());
    }
}

//------------------------------------------------------------------------------
void Server::ManageGame(void)
{
    // We only do blaze game stuff if we are authenticated and QOS is complete
    if ((mBlazeHub->getUserManager()->getLocalUser(0) != nullptr) &&
        mBlazeHub->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
    {
        if (!mIsGameCreated)
        {
            const uint32_t timeNowMs = NetTick();
            static uint32_t timeLastMs = 0;
            if (timeNowMs - timeLastMs >= 10000)
            {
                CreateGame();
                timeLastMs = timeNowMs;
            }
        }
        else if (mBlazeHub->getGameManagerAPI()->getGameCount() > 0)
        {
            Blaze::GameManager::Game * game = mBlazeHub->getGameManagerAPI()->getGameByIndex(0);

            GameStateLogic(game);
            RebroadcastPackets(game);
        }
    }
}

//------------------------------------------------------------------------------
void Server::CreateGame(void)
{
    Log("Server::CreateGame\n");

    Blaze::GameManager::Game::CreateGameParameters params;
    params.mGameName            = mBlazeHub->getLoginManager(0)->getPersonaName(); // or can use computer name
    params.mNetworkTopology     = Blaze::CLIENT_SERVER_DEDICATED;

    if (g_isVoipEnabled)
    {
        params.mVoipTopology    = Blaze::VOIP_DEDICATED_SERVER;
    }
    else
    {
        params.mVoipTopology    = Blaze::VOIP_DISABLED;
    }

    // mMaxPlayerCapacity is a hard cap on the largest size the game can be reset to accommodate
    params.mMaxPlayerCapacity   = g_maxPlayers;

    Log("createGame\n");
    // Note: there is no notification for createGame other than the callback
    mBlazeHub->getGameManagerAPI()->createGame(params, MakeFunctor(this, &Server::CreateGameCb));
    mIsGameCreated = true;
}

//------------------------------------------------------------------------------
void Server::CreateGameCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::GameManager::Game* game)
{
    Log("Server::CreateGameCb %s\n", DescribeError(error));

    if (error == Blaze::ERR_OK)
    {
        game->addListener(this);

        Log("state = %s\n", Blaze::GameManager::GameStateToString(game->getGameState()));
    }
    else
    {
        mIsGameCreated = false;
    }
}

//------------------------------------------------------------------------------
void Server::GameStateLogic(Blaze::GameManager::Game * game)
{
    EA_ASSERT(game != nullptr);

    // Randomly cycle game state. This code is for demonstration purposes only.

    if (g_isGameStateControlledByServer && !mIsGameChangingState)
    {
        switch (game->getGameState())
        {
            case Blaze::GameManager::PRE_GAME:
            {
                if (AboutOnceInNSeconds(120))
                {
                    Log("startGame\n");
                    // Note: notification for startGame is onGameStarted
                    game->startGame(MakeFunctor(this, &Server::StartGameCb));
                    mIsGameChangingState = true;
                }
                break;
            }

            case Blaze::GameManager::IN_GAME:
            {
                if (AboutOnceInNSeconds(120))
                {
                    Log("endGame\n");
                    // Note: notification for endGame is onGameEnded
                    game->endGame(MakeFunctor(this, &Server::EndGameCb));
                    mIsGameChangingState = true;
                }
                break;
            }

            case Blaze::GameManager::POST_GAME:
            {
                if (AboutOnceInNSeconds(60))
                {
                    Log("replayGame\n");
                    // Note: notification for replayGame is onPreGame
                    game->replayGame(MakeFunctor(this, &Server::ReplayGameCb));
                    mIsGameChangingState = true;
                }
                break;
            }

            default:
                break;
        }
    }
}

//------------------------------------------------------------------------------
bool Server::AboutOnceInNSeconds(int n)
{
    return ((::rand() % (n * mTargetFps)) == 0);
}

//------------------------------------------------------------------------------
void Server::StartGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game)
{
    Log("Server::StartGameCb %s\n", DescribeError(error));

    if (error != Blaze::ERR_OK)
    {
        mIsGameChangingState = false;
    }
}

//------------------------------------------------------------------------------
void Server::EndGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game)
{
    Log("Server::EndGameCb %s\n", DescribeError(error));

    if (error != Blaze::ERR_OK)
    {
        mIsGameChangingState = false;
    }
}

//------------------------------------------------------------------------------
void Server::ReplayGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game)
{
    Log("Server::ReplayGameCb %s\n", DescribeError(error));

    if (error != Blaze::ERR_OK)
    {
        mIsGameChangingState = false;
    }
}

//------------------------------------------------------------------------------
// Read packets from each player and send them to all the other players
void Server::RebroadcastPackets(Blaze::GameManager::Game * game)
{
    EA_ASSERT(game != nullptr);

    for (uint16_t index = 0; index < game->getActivePlayerCount(); index++)
    {
        const Blaze::GameManager::Player * player = game->getActivePlayerByIndex(index);
        const ConnApiClientT * pClient = mNetAdapter->getClientHandleForEndpoint(player->getMeshEndpoint());
        NetGameLinkRefT * pGameLinkRef = pClient->pGameLinkRef;

        if (pGameLinkRef != nullptr)
        {
            NetGameMaxPacketT packet;
            while (NetGameLinkRecv(pGameLinkRef, (NetGamePacketT *) &packet, 1, FALSE) > 0)
            {
                SendPacketToOtherPlayers(game, player, packet);
            }
        }
    }
}

//------------------------------------------------------------------------------
// Send the given packet to all players in the game except for the sender
void Server::SendPacketToOtherPlayers(Blaze::GameManager::Game * game,
    const Blaze::GameManager::Player * sendingPlayer, const NetGameMaxPacketT & packet)
{
    EA_ASSERT(game != nullptr);
    EA_ASSERT(sendingPlayer != nullptr);

    for (uint16_t index = 0; index < game->getActivePlayerCount(); index++)
    {
        const Blaze::GameManager::Player * player = game->getActivePlayerByIndex(index);

        if (player != sendingPlayer)
        {
            const ConnApiClientT * pClient = mNetAdapter->getClientHandleForEndpoint(player->getMeshEndpoint());
            NetGameLinkRefT * pGameLinkRef = pClient->pGameLinkRef;

            if (pGameLinkRef != nullptr)
            {
                const int bytesSent = NetGameLinkSend(pGameLinkRef, (NetGamePacketT *) &packet, 1);
                // NetGameLinkSend returns positive for success, zero for overflow, or negative for error.
                if (bytesSent <= 0)
                {
                    // The send failed.
                    Log("NetGameLinkSend failed to %s\n", player->getName());

                    // We treat this as non-recoverable. We assume that the dedicated server has a sufficiently
                    // large send buffer, calls idle at a sufficient rate, and has sufficient bandwidth.
                    // We therefore destroy the connection to this client.
                    Log("kickPlayer %s\n", player->getName());
                    game->kickPlayer(player, MakeFunctor(this, &Server::KickPlayerCb));
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
void Server::KickPlayerCb(Blaze::BlazeError error, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player)
{
    Log("Server::KickPlayerCb %s %s\n", player->getName(), DescribeError(error));
}

//------------------------------------------------------------------------------
void Server::Close(void)
{
    Log("Server::Close\n");

    mBlazeHub->getLoginManager(0)->removeListener(this);
    mBlazeHub->getGameManagerAPI()->deactivate();
    Blaze::BlazeHub::shutdown(mBlazeHub);
    mBlazeHub = nullptr;

    if (mNetAdapter != nullptr)
    {
        delete mNetAdapter;
        mNetAdapter = nullptr;
    }

    delete[] mCertData;
    mCertData = nullptr;
    delete[] mKeyData;
    mKeyData = nullptr;

    NetConnShutdown(0);
}

//------------------------------------------------------------------------------
// Blaze::BlazeStateEventHandler notifications
//------------------------------------------------------------------------------

// This listener callback drives the major connection events
void Server::onConnected()
{
    Blaze::ConnectionManager::ConnectionManager* connMgr = mBlazeHub->getConnectionManager();

    if (connMgr == nullptr)
    {
        Log("[Server::onConnected()] ConnectionManager is nullptr!\n");
        return;
    }

    mBlazeHub->getLoginManager(0)->startTrustedLoginProcess(mCertData, mKeyData, "Sample", "DS");
}

char8_t* Server::loadCert(const char *certFilename, int32_t *certSize, const char *strBegin, const char *strEnd)
{
    char8_t *certBuf, *start, *end;

    using namespace std;
    if (certFilename[0] != '\0')
    {
        ifstream certFile(certFilename, ios::in|ios::binary|ios::ate);
        if (!certFile.is_open())
            return nullptr;

        streampos size;
        size = certFile.tellg();
        certBuf = new char8_t[(uint32_t)size];
        certFile.seekg(0, ios::beg);
        certFile.read(certBuf, size);
        certFile.close();

        if ((start = blaze_stristr(certBuf, strBegin)) == nullptr)
        {
            delete [] certBuf;
            return nullptr;
        }

        for (start += strlen(strBegin); *start == '\r' || *start == '\n' || *start == ' '; start += 1)
            ;

        memmove(certBuf, start, strlen(start)+1);

        if ((end = blaze_stristr(certBuf, strEnd)) == nullptr)
        {
            delete [] certBuf;
            return nullptr;
        }

        for (end -= 1; (*end == '\r') || (*end == '\n') || (*end == ' '); end -= 1)
            ;

        end[1] = '\0';

        *certSize = (int32_t)(end - certBuf + 1);
        return certBuf;
    }

    return nullptr;
}

//------------------------------------------------------------------------------
void Server::DisconnectFromBlazeServer(void)
{
    if (mBlazeHub->getConnectionManager()->isConnected())
    {
        Log("disconnect\n");
        mBlazeHub->getConnectionManager()->disconnect();
    }
}

//------------------------------------------------------------------------------
// BaseGameListener notifications
//------------------------------------------------------------------------------

void Server::onGameReset(Blaze::GameManager::Game* game)
{
    Log("Server::onGameReset\n");

    Log("  getMaxPlayerCapacity:             %d\n", game->getMaxPlayerCapacity());
    Log("  getPlayerCapacity(SLOT_PUBLIC_PARTICIPANT):   %d\n", game->getPlayerCapacity(Blaze::GameManager::SLOT_PUBLIC_PARTICIPANT));
    Log("  getPlayerCapacity(SLOT_PRIVATE_PARTICIPANT):  %d\n", game->getPlayerCapacity(Blaze::GameManager::SLOT_PRIVATE_PARTICIPANT));
    Log("  getPlayerCapacity(SLOT_PUBLIC_SPECTATOR):   %d\n", game->getPlayerCapacity(Blaze::GameManager::SLOT_PUBLIC_SPECTATOR));
    Log("  getPlayerCapacity(SLOT_PRIVATE_SPECTATOR):  %d\n", game->getPlayerCapacity(Blaze::GameManager::SLOT_PRIVATE_SPECTATOR));
    Log("  getRanked:                        %d\n", game->getRanked());

    // It is often useful to see the attributes, for diagnostic purposes
    Log("  Attributes:\n");
    for (Blaze::Collections::AttributeMap::const_iterator iter = game->getGameAttributeMap().begin();
        iter != game->getGameAttributeMap().end(); ++iter)
    {
        const Blaze::Collections::AttributeName & name      = iter->first;
        const Blaze::Collections::AttributeValue & value    = iter->second;
        Log("    %s = %s\n", name.c_str(), value.c_str());
    }

    if (g_isGameStateControlledByServer)
    {
        Log("initGameComplete\n");
        // Note: notification for initGameComplete is onPreGame
        game->initGameComplete(MakeFunctor(this, &Server::InitGameCompleteCb));
        mIsGameChangingState = true;
    }
}

//------------------------------------------------------------------------------
void Server::InitGameCompleteCb(Blaze::BlazeError error, Blaze::GameManager::Game * game)
{
    Log("Server::InitGameCompleteCb %s\n", DescribeError(error));

    if (error != Blaze::ERR_OK)
    {
        mIsGameChangingState = false;
    }
}

//------------------------------------------------------------------------------
void Server::onPlayerRemoved(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *removedPlayer,
    Blaze::GameManager::PlayerRemovedReason playerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext titleContext)
{
    Log("Server::onPlayerRemoved %s for reason %s\n", removedPlayer->getName(), PlayerRemovedReasonToString(playerRemovedReason));

    // Handle empty games
    if (game->getPlayerCount() == 0)
    {
        Log("Player count is zero.\n");

        switch (game->getGameState())
        {
            case Blaze::GameManager::INITIALIZING:  // note - this case can happen if resetting client crashes
            case Blaze::GameManager::PRE_GAME:
            case Blaze::GameManager::POST_GAME:
                HandleEmptyGame(game);
                break;

            case Blaze::GameManager::IN_GAME:
                // It is important to not skip POST_GAME
                Log("endGame\n");
                // Note: notification for endGame is onGameEnded
                game->endGame(MakeFunctor(this, &Server::EndGameCb));
                mIsGameChangingState = true;
                break;
            case Blaze::GameManager::UNRESPONSIVE:
                // we can be here if we actually became responsive again but just got the (queued) notification of the
                // state change to UNRESPONSIVE. The state change out of UNRESPONSIVE should come soon, wait for it.
                Log("empty game detected but we are still recovering from unresponsive. awaiting onBackFromUnresponsive before returning to pool\n");
                mIsGameChangingState = true;
                break;
            default:
                break;
        }
    }
}

void Server::onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState)
{
    Log("Server::onUnresponsive Game became unresponsive. Awaiting reconnection..\n");
}

void Server::onBackFromUnresponsive(Blaze::GameManager::Game *game)
{
    Log("Server::onBackFromUnresponsive Game came back from being unresponsive. Restored state is [%s]\n", Blaze::GameManager::GameStateToString(game->getGameState()));
    mIsGameChangingState = false;

    if (game->getPlayerCount() == 0)
    {
        if (game->getGameState() == Blaze::GameManager::IN_GAME)
        {
            // It is important to not skip POST_GAME
            Log("endGame\n");
            // Note: notification for endGame is onGameEnded
            game->endGame(MakeFunctor(this, &Server::EndGameCb));
            mIsGameChangingState = true;
        }
        else
        {
            HandleEmptyGame(game);
        }
    }
}

//------------------------------------------------------------------------------
void Server::HandleEmptyGame(Blaze::GameManager::Game* game)
{
    Log("Server::HandleEmptyGame\n");

    if (g_isReuseDedicatedServer)
    {
        Log("returnDedicatedServerToPool\n");
        // Note: notification for returnDedicatedServerToPool is onReturnDServerToPool
        game->returnDedicatedServerToPool(MakeFunctor(this, &Server::ReturnDedicatedServerToPoolCb));
        mIsGameChangingState = true;
    }
    else
    {
        Log("destroyGame\n");
        // Note: notification for destroyGame is onGameDestructing
        game->destroyGame(Blaze::GameManager::TITLE_REASON_BASE_GAME_DESTRUCTION_REASON,
            MakeFunctor(this, &Server::DestroyGameCb));
        mIsGameChangingState = true;
    }
}

//------------------------------------------------------------------------------
void Server::ReturnDedicatedServerToPoolCb(Blaze::BlazeError error, Blaze::GameManager::Game * game)
{
    Log("Server::ReturnDedicatedServerToPoolCb %s\n", DescribeError(error));

    if (error != Blaze::ERR_OK)
    {
        mIsGameChangingState = false;
    }
}

//------------------------------------------------------------------------------
void Server::DestroyGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game)
{
    Log("Server::DestroyGameCb %s\n", DescribeError(error));

    if (error != Blaze::ERR_OK)
    {
        mIsGameChangingState = false;
    }
}

//------------------------------------------------------------------------------
void Server::onGameGroupInitialized(Blaze::GameManager::Game* game)
{
    Log("Server::onGameGroupInitialized\n");

    mIsGameChangingState = false;
}

//------------------------------------------------------------------------------
void Server::onPreGame(Blaze::GameManager::Game* game)
{
    Log("Server::onPreGame\n");

    mIsGameChangingState = false;
}

//------------------------------------------------------------------------------
void Server::onGameStarted(Blaze::GameManager::Game* game)
{
    Log("Server::onGameStarted\n");

    mIsGameChangingState = false;
}

//------------------------------------------------------------------------------
void Server::onGameEnded(Blaze::GameManager::Game* game)
{
    Log("Server::onGameEnded\n");

    mIsGameChangingState = false;

    // If a game becomes empty IN-GAME then it must first be transitioned to POST-GAME before it can be reset.
    // Catch this case here.
    if (game->getPlayerCount() == 0)
    {
        HandleEmptyGame(game);
    }
}

//------------------------------------------------------------------------------
void Server::onReturnDServerToPool(Blaze::GameManager::Game* game)
{
    Log("Server::onReturnDServerToPool\n");

    mIsGameChangingState = false;
}

//------------------------------------------------------------------------------
void Server::onGameDestructing(Blaze::GameManager::GameManagerAPI* gameManagerAPI, Blaze::GameManager::Game* game, Blaze::GameManager::GameDestructionReason reason)
{
    Log("Server::onGameDestructing\n");

    mIsGameChangingState = false;
    mIsGameCreated = false;

    if (!g_isReuseDedicatedServer)
    {
        mIsExitRequested = true;
    }
}

void Server::onLoginFailure(Blaze::BlazeError errorCode, const char8_t* portalUrl)
{
    Log("Server::onLoginFailure %s\n", DescribeError(errorCode));

    if (errorCode != Blaze::ERR_OK)
    {
        // Authentication can fail for reasons such as ERR_TIMEOUT.
        // Our state here will be AUTHENTICATE_TRANSITION.
        // Easiest thing to do is to start over with the Blaze connection.
        DisconnectFromBlazeServer();
    }
}

void Server::onSdkError(Blaze::BlazeError errorCode)
{
    Log("Server::onSdkError %s\n", DescribeError(errorCode));

    if (errorCode != Blaze::ERR_OK)
    {
        // Authentication can fail for reasons such as ERR_TIMEOUT.
        // Our state here will be AUTHENTICATE_TRANSITION.
        // Easiest thing to do is to start over with the Blaze connection.
        DisconnectFromBlazeServer();
    }
}

int Vsnprintf8 (char8_t*  pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}
