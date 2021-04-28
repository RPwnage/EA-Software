/*! ****************************************************************************
    \file BaseGameListener.h
    \brief
    Base class for GameListener; all calls are logged.


    \attention
        Copyright (c) Electronic Arts 2011.  ALL RIGHTS RESERVED.

*******************************************************************************/

#ifndef __BASEGAMELISTENER_H__
#define __BASEGAMELISTENER_H__

#include <BlazeSDK\gamemanager\game.h>
#include <BlazeSDK\gamemanager\player.h>

#include "Utility.h"

//------------------------------------------------------------------------------
class BaseGameListener : public Blaze::GameManager::GameListener
{
    public:

    protected:
        // Blaze::GameManager::Game state related methods
        virtual void onGameGroupInitialized(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onGameGroupInitialized\n");}
        virtual void onPreGame(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onPreGame\n");}
        virtual void onGameStarted(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onGameStarted\n");}
        virtual void onGameEnded(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onGameEnded\n");}
        virtual void onGameReset(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onGameReset\n");}
        virtual void onReturnDServerToPool(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onReturnDServerToPool\n");}

        // Blaze::GameManager::Game Settings and Attribute updates
        virtual void onGameAttributeUpdated(Blaze::GameManager::Game* game, const Blaze::Collections::AttributeMap * changedAttributeMap)
            {Log("BaseGameListener::onGameAttributeUpdated\n");}
        virtual void onAdminListChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *adminPlayer, Blaze::GameManager::UpdateAdminListOperation operation, Blaze::GameManager::PlayerId updaterId)
            {Log("BaseGameListener::onAdminListChanged\n");}
        virtual void onGameSettingUpdated(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onGameSettingUpdated\n");}
        virtual void onPlayerCapacityUpdated(Blaze::GameManager::Game *game)
            {Log("BaseGameListener::onPlayerCapacityUpdated\n");}
        virtual void onGameTeamIdUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::TeamIndex teamIndex, Blaze::GameManager::TeamId newTeamId)
            {Log("BaseGameListener::onGameTeamIdUpdated\n");}

        // Blaze::GameManager::Player related methods
        virtual void onPlayerJoining(Blaze::GameManager::Player* player)
            {Log("BaseGameListener::onPlayerJoining %s\n", player->getName());}
        virtual void onPlayerJoiningQueue(Blaze::GameManager::Player *player)
            {Log("BaseGameListener::onPlayerJoiningQueue %s\n", player->getName());}
        virtual void onPlayerJoinComplete(Blaze::GameManager::Player* player)
            {Log("BaseGameListener::onPlayerJoinComplete %s\n", player->getName());}
        virtual void onQueueChanged(Blaze::GameManager::Game *game)
            {Log("BaseGameListener::onQueueChanged\n");}
        virtual void onPlayerRemoved(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *removedPlayer, 
            Blaze::GameManager::PlayerRemovedReason playerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext titleContext)
            {Log("BaseGameListener::onPlayerRemoved %s for Blaze reason %s\n", removedPlayer->getName(), PlayerRemovedReasonToString(playerRemovedReason));}
        virtual void onPlayerAttributeUpdated(Blaze::GameManager::Player* player, const Blaze::Collections::AttributeMap *changedAttributeMap)
            {Log("BaseGameListener::onPlayerAttributeUpdated %s\n", player->getName());}
        virtual void onPlayerTeamUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::TeamIndex previousTeamIndex)
            {Log("BaseGameListener::onPlayerTeamUpdated %s\n", player->getName());}
        virtual void onPlayerCustomDataUpdated(Blaze::GameManager::Player* player)
            {Log("BaseGameListener::onPlayerCustomDataUpdated %s\n", player->getName());}

        // Networking(Host Migration) related methods
        virtual void onHostMigrationStarted(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onHostMigrationStarted\n");}
        virtual void onHostMigrationEnded(Blaze::GameManager::Game* game)
            {Log("BaseGameListener::onHostMigrationEnded\n");}
        virtual void onNetworkCreated(Blaze::GameManager::Game *game)
            {Log("BaseGameListener::onNetworkCreated\n");}
        virtual void onPlayerJoinedFromQueue(Blaze::GameManager::Player *player)
            {Log("BaseGameListener::onPlayerJoinedFromQueue\n");}
        virtual void onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState)
            {Log("BaseGameListener::onUnresponsive\n");}
        virtual void onBackFromUnresponsive(Blaze::GameManager::Game *game)
            {Log("BaseGameListener::onBackFromUnresponsive\n");}

        /* TO BE DEPRECATED - will never be notified if you don't use GameManagerAPI::receivedMessageFromEndpoint() */
        virtual void onMessageReceived(Blaze::GameManager::Game *game, const void *buf, size_t bufSize, Blaze::BlazeId senderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags) 
            {}

    private:
};

//------------------------------------------------------------------------------
#endif //__BASEGAMELISTENER_H__
