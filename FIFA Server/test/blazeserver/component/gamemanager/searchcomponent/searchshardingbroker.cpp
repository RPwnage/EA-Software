/*! ************************************************************************************************/
/*!
    \file searchshardingbroker.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"

#include "framework/component/componentmanager.h"
#include "gamemanager/searchcomponent/searchshardingbroker.h"
#include "gamemanager/rpc/gamemanager_defines.h"
#include "gamemanager/rpc/matchmaker_defines.h"
#include "gamemanager/rpc/gamemanagermaster.h"

namespace Blaze
{
namespace Search
{
    SearchShardingBroker::SearchShardingBroker()
    {
    }

    SearchShardingBroker::~SearchShardingBroker()
    {
    }

    bool SearchShardingBroker::registerForSearchSlaveNotifications()
    {
        // subsribe to search component notifications 
        BlazeRpcError error;
        Blaze::Search::SearchSlave* searchComponent = nullptr;
        // NOTE: By using a remote component placeholder we are able to perform an apriori subscription for notifications *without*
        // waiting for the component to actually be active.
        error = gController->getComponentManager().addRemoteComponentPlaceholder(Blaze::Search::SearchSlave::COMPONENT_ID, reinterpret_cast<Component*&>(searchComponent));
        if (error != ERR_OK || searchComponent == nullptr)
        {
            FATAL_LOG("[SearchShardingBroker].initialize(): Unable to obtain search slave component placeholder.");
            return false;
        }

        searchComponent->addNotificationListener(*this);
        error = searchComponent->notificationSubscribe();
        if (error != ERR_OK)
        {
            WARN_LOG("[SearchShardingBroker].initialize: Unable to subscribe for notifications from search component.");
        }

        return true;
    }

    void SearchShardingBroker::getFullCoverageSearchSet(Blaze::InstanceIdList& resolvedSearchSlaveList) const
    {
        gSliverManager->getFullListenerCoverage(GameManager::GameManagerMaster::COMPONENT_ID, resolvedSearchSlaveList);
    }

    BlazeRpcError SearchShardingBroker::convertSearchComponentErrorToMatchmakerError(BlazeRpcError searchRpcError)
    {
        switch(searchRpcError)
        {
        case ERR_OK:
            return ERR_OK;
        case ERR_TIMEOUT:
            return ERR_TIMEOUT;
        case ERR_SYSTEM:
            return ERR_SYSTEM;
        case ERR_CANCELED:
            return ERR_CANCELED;
        case SEARCH_ERR_INVALID_MATCHMAKING_CRITERIA:
            return MATCHMAKER_ERR_INVALID_MATCHMAKING_CRITERIA;
        case SEARCH_ERR_UNKNOWN_MATCHMAKING_SESSION_ID:
            return MATCHMAKER_ERR_UNKNOWN_MATCHMAKING_SESSION_ID;
        case SEARCH_ERR_MATCHMAKING_USERSESSION_NOT_FOUND:
            return MATCHMAKER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
        case SEARCH_ERR_INVALID_GAME_ENTRY_TYPE:
            return MATCHMAKER_ERR_INVALID_GAME_ENTRY_TYPE;
        case SEARCH_ERR_INVALID_GROUP_ID:
            return MATCHMAKER_ERR_INVALID_GROUP_ID;
        case SEARCH_ERR_PLAYER_NOT_IN_GROUP:
            return MATCHMAKER_ERR_PLAYER_NOT_IN_GROUP;
        case SEARCH_ERR_IN_TRANSITION:
        case SEARCH_ERR_OVERLOADED:
            return ERR_SYSTEM;
        default:
            // caught an unrecognized error code, pass it through, but log an error:
            ERR_LOG("[SearchShardingBroker].convertMatchmakingComponentErrorToGameManagerError: Unrecognized error '" << ErrorHelp::getErrorName(searchRpcError) 
                << "' returned from search component.");
        }
        return ERR_SYSTEM;
    }

    BlazeRpcError SearchShardingBroker::convertSearchComponentErrorToGameManagerError(BlazeRpcError searchRpcError)
    {
        switch(searchRpcError)
        {
        case ERR_OK:
            return ERR_OK;
        case ERR_TIMEOUT:
            return ERR_TIMEOUT;
        case ERR_SYSTEM:
            return ERR_SYSTEM;
        case ERR_CANCELED:
            return ERR_CANCELED;
        case SEARCH_ERR_OVERLOADED:
            return  GAMEBROWSER_ERR_SEARCH_ERR_OVERLOADED;
        case SEARCH_ERR_IN_TRANSITION:
            return ERR_SYSTEM;
        case SEARCH_ERR_INVALID_CRITERIA:
            return GAMEBROWSER_ERR_INVALID_CRITERIA;
        case SEARCH_ERR_INVALID_CAPACITY:
            return GAMEBROWSER_ERR_INVALID_CAPACITY;
        case SEARCH_ERR_INVALID_LIST_ID:
            return GAMEBROWSER_ERR_INVALID_LIST_ID;
        case SEARCH_ERR_INVALID_LIST_CONFIG_NAME:
            return GAMEBROWSER_ERR_INVALID_LIST_CONFIG_NAME;
        case SEARCH_ERR_MAX_TOTAL_NUMBER_OF_LISTS_EXCEEDED:
            return GAMEBROWSER_ERR_EXCEEDED_MAX_REQUESTS;
        case SEARCH_ERR_GAME_PROTOCOL_VERSION_MISMATCH:
            return GAMEMANAGER_ERR_GAME_PROTOCOL_VERSION_MISMATCH;
        case ERR_AUTHORIZATION_REQUIRED:
            return ERR_AUTHORIZATION_REQUIRED;
        default:
            ERR_LOG("[SearchShardingBroker:].convertSearchComponentErrorToGameManagerError: Unrecognized error '" << ErrorHelp::getErrorName(searchRpcError) 
                << "' returned from search component.");
        }
        return ERR_SYSTEM;
    }


} // namespace Search
} // namespace Blaze
