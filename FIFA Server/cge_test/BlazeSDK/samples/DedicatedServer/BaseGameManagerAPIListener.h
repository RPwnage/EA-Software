/*! ****************************************************************************
    \file BaseGameManagerAPIListener.h
    \brief
    Base class for GameManagerAPIListener; all calls are logged.


    \attention
        Copyright (c) Electronic Arts 2011.  ALL RIGHTS RESERVED.

*******************************************************************************/

#ifndef __BASEGAMEMANAGERAPILISTENER_H__
#define __BASEGAMEMANAGERAPILISTENER_H__

#include <BlazeSDK\gamemanager\gamemanagerapi.h>

#include "Utility.h"

//------------------------------------------------------------------------------
class BaseGameManagerAPIListener : public Blaze::GameManager::GameManagerAPIListener
{
    protected:
        virtual void onGameDestructing(Blaze::GameManager::GameManagerAPI* gameManagerAPI, Blaze::GameManager::Game* game, Blaze::GameManager::GameDestructionReason reason)
            {Log("BaseGameManagerAPIListener::onGameDestructing\n");}

        virtual void onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList* gameBrowserList,
                                                   const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* removedGames,
                                                   const Blaze::GameManager::GameBrowserList::GameBrowserGameVector* updatedGames)
            {Log("BaseGameManagerAPIListener::onGameBrowserListUpdated\n");}

        virtual void onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList* gameBrowserList)
            {Log("BaseGameManagerAPIListener::onGameBrowserListUpdateTimeout\n");}

        virtual void onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList)
            {Log("BaseGameManagerAPIListener::onGameBrowserListDestroy\n");}

        virtual void onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, 
                                                   const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, 
                                                   Blaze::GameManager::Game* game)
            {Log("BaseGameManagerAPIListener::onMatchmakingScenarioFinished\n");}

        virtual void onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario,
                                                   const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList)
            {Log("BaseGameManagerAPIListener::onMatchmakingScenarioAsyncStatus\n");}

        virtual void onUserGroupJoinGame(Blaze::GameManager::Game* game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId groupId)
            {Log("BaseGameManagerAPIListener::onUserGroupJoinGame\n");}

        virtual void onPlayerJoinGameQueue(Blaze::GameManager::Game *game)
            {Log("BaseGameManagerAPIListener::onPlayerJoinGameQueue\n");}

    private:
};

//------------------------------------------------------------------------------
#endif //__BASEGAMEMANAGERAPILISTENER_H__
