/*! ************************************************************************************************/
/*!
    \file gamebrowser.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SEARCH_USERSETSEARCHMANAGER_H
#define BLAZE_SEARCH_USERSETSEARCHMANAGER_H

#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/searchcomponent/gamelist.h"

#include "EASTL/set.h"

namespace Blaze
{
    class UserSession;


namespace Search
{
    class SearchSlaveImpl;
    class UserSetGameBrowserList;
    
    /*! ************************************************************************************************/
    /*! \brief The GameBrowser is a slave only "pseudo" component that lives inside the game manager component.
    *************************************************************************************************/
    class UserSetSearchManager 
    {
        NON_COPYABLE(UserSetSearchManager );
        class UserSetGameList;

    public:
        UserSetSearchManager (SearchSlaveImpl& searchSlave);
        
        ~UserSetSearchManager() {}

        void onUpdate(GameManager::GameId gameId);
        void onErase(GameManager::GameId gameId);

        CreateUserSetGameListError::Error processCreateUserSetGameList(const Blaze::Search::CreateUserSetGameListRequest &request, Blaze::Search::CreateGameListResponse &response, const Message* message);
        UpdateUserSetGameListError::Error processUpdateUserSetGameList(const Blaze::Search::UserSetWatchListUpdate &request, const Message* message);

        void onUserJoinGame(const BlazeId& joiningBlazeId, GameManager::GameId gameId) { updateGameOnUserAction(joiningBlazeId, gameId, true); }
        void onUserLeaveGame(const BlazeId& joiningBlazeId, GameManager::GameId gameId) { updateGameOnUserAction(joiningBlazeId, gameId, false); }
    
    private:
        void updateGameOnUserAction(const BlazeId& blazeId, GameManager::GameId gameId, bool isJoining);       

        void addListToGlobalUserMap(BlazeId blazeId, UserSetGameList& list);
        void removeListFromGlobalUserMap(BlazeId blazeId, UserSetGameList& list);
        
        void addListToGlobalGameMap(GameManager::GameId gameId, UserSetGameList& list);
        void removeListFromGlobalGameMap(GameManager::GameId gameId, UserSetGameList& list);

    private:
        class UserSetGameList : public GameListBase
        {
            NON_COPYABLE(UserSetGameList);
        public:
            UserSetGameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId mOriginalInstanceSessionId, UserSessionId ownerSessionId, const GameManager::GameBrowserListConfig& listConfig);

            bool isUserList() const override { return true; }

            void cleanup() override;

            void updateGame(GameManager::GameId gameId) { markGameAsUpdated(gameId, 0, false); }
            void unmapGame(GameManager::GameId gameId);
            void removeGame(GameManager::GameId gameId);

            void trackUser(BlazeId player);
            void stopTrackingUser(BlazeId player);

            void addUserMatch(GameManager::GameId gameId, BlazeId player);
            void removeUserMatch(GameManager::GameId gameId, BlazeId player);

        private:
            typedef eastl::hash_set<BlazeId> WatchedMemberSet;
            typedef eastl::hash_map<GameManager::GameId, WatchedMemberSet> GameIdToWatchedMembersMap;   
            GameIdToWatchedMembersMap mGameIdToWatchedMembersMap;

            WatchedMemberSet mTrackedUsers;
        };

        friend class UserSetGameList;

        SearchSlaveImpl& mSearchSlave;

        // map so each game can track the lists its a member of - for fast list fixup when a game is destroyed.
        typedef eastl::hash_set<UserSetGameList*> UserSetGameListSet;
        typedef eastl::hash_map<GameManager::GameId, UserSetGameListSet > GameIdUserSetListReferenceMap;
        GameIdUserSetListReferenceMap mGameIdUserSetListMap;

        typedef eastl::map<BlazeId, UserSetGameListSet> BlazeIdReferenceMap;
        BlazeIdReferenceMap mWatcheeUserSetGameListMap;
    };

} // namespace Search
} // namespace Blaze
#endif // BLAZE_SEARCH_USERSETSEARCHMANAGER_H
