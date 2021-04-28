/*! ****************************************************************************
    \file Server.h
    \brief
    Demonstrate how to create a dedicated server, using the Game Manager API.
    In this case the dedicated server is essentially a rebroadcaster; but game logic could be added.
    See http://online.ea.com/confluence/display/blaze/Blaze+Dedicated+Servers


    \attention
        Copyright (c) Electronic Arts 2011.  ALL RIGHTS RESERVED.

*******************************************************************************/

#ifndef __SERVER_H__
#define __SERVER_H__

#include <BlazeSDK/blazenetworkadapter/connapiadapter.h>
#include <BlazeSDK/connectionmanager/connectionmanager.h>
#include <BlazeSDK/component/authenticationcomponent.h>
#include <BlazeSDK/loginmanager/loginmanagerlistener.h>

#include "BaseGameManagerAPIListener.h"
#include "BaseGameListener.h"

//------------------------------------------------------------------------------
class Server : 
    public BaseGameManagerAPIListener,
    public BaseGameListener,
    public Blaze::BlazeStateEventHandler,
    public Blaze::LoginManager::LoginManagerListener
{
    public:
        Server(void);

        void                Run(void);

        // From BaseGameListener:
        virtual void        onGameGroupInitialized(Blaze::GameManager::Game* game);
        virtual void        onGameReset(Blaze::GameManager::Game* game);
        virtual void        onPreGame(Blaze::GameManager::Game* game);
        virtual void        onGameStarted(Blaze::GameManager::Game* game);
        virtual void        onGameEnded(Blaze::GameManager::Game* game);
        virtual void        onReturnDServerToPool(Blaze::GameManager::Game* game);
        virtual void        onPlayerRemoved(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *removedPlayer, 
            Blaze::GameManager::PlayerRemovedReason playerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext titleContext);
        virtual void onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState);
        virtual void onBackFromUnresponsive(Blaze::GameManager::Game *game);

        // From BaseGameManagerAPIListener:
        virtual void        onGameDestructing(Blaze::GameManager::GameManagerAPI* gameManagerAPI, Blaze::GameManager::Game* game, Blaze::GameManager::GameDestructionReason reason);

        // From BaseBlazeStateEventHandler:
        virtual void onConnected();

        // From LoginManagerListener
        virtual void onLoginFailure(Blaze::BlazeError errorCode, const char8_t* portalUrl);
        virtual void onForgotPasswordError(Blaze::BlazeError errorCode) {}
        virtual void onSdkError(Blaze::BlazeError errorCode);

private:
        Server& operator=(const Server &tmp); // Do not allow assignment

        void                Init(void);
        static int32_t      DirtySockLogFunction(void *pParm, const char *pText);
        static void         BlazeLogFunction(const char8_t *pText, void *data);
        const char *        DescribeError(Blaze::BlazeError error);
        void                CreateApis(void);
        void                ManageConnectionToBlazeServer(void);
        void                ConnectionMessagesCb(Blaze::BlazeError error, const Blaze::Redirector::DisplayMessageList *messageList);
        void                ManageGame(void);
        void                CreateGame(void);
        void                CreateGameCb(Blaze::BlazeError error, Blaze::JobId jobId, Blaze::GameManager::Game* game);
        void                GameStateLogic(Blaze::GameManager::Game * game);
        bool                AboutOnceInNSeconds(int n);
        void                StartGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game);
        void                EndGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game);
        void                ReplayGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game);
        void                RebroadcastPackets(Blaze::GameManager::Game * game);
        void                SendPacketToOtherPlayers(Blaze::GameManager::Game * game, 
                                const Blaze::GameManager::Player * sendingPlayer, const NetGameMaxPacketT & packet);
        void                KickPlayerCb(Blaze::BlazeError error, Blaze::GameManager::Game *game, const Blaze::GameManager::Player *player);
        void                Close(void);
        void                DisconnectFromBlazeServer(void);
        void                InitGameCompleteCb(Blaze::BlazeError error, Blaze::GameManager::Game * game);
        void                HandleEmptyGame(Blaze::GameManager::Game * game);
        void                ReturnDedicatedServerToPoolCb(Blaze::BlazeError error, Blaze::GameManager::Game * game);
        void                DestroyGameCb(Blaze::BlazeError error, Blaze::GameManager::Game * game);

        char8_t*            loadCert(const char *certFilename, int32_t *certSize, const char *strBegin, const char *strEnd);

        bool                mIsExitRequested;
        bool                mIsGameCreated;
        Blaze::BlazeNetworkAdapter::ConnApiAdapter * mNetAdapter;
        Blaze::BlazeHub*    mBlazeHub;
        bool                mIsGameChangingState;
        const uint32_t      mTargetFps;
        char8_t*            mCertData;
        char8_t*            mKeyData;
};

//------------------------------------------------------------------------------
#endif // __SERVER_H__

