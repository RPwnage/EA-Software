/*! ************************************************************************************************/
/*!
    \file unsortedgamelist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SEARCH_UNSORTED_GAME_LIST_H
#define BLAZE_SEARCH_UNSORTED_GAME_LIST_H

#include "gamemanager/gamebrowser/gamebrowser.h"
#include "gamemanager/searchcomponent/gamelist.h"
#include "gamemanager/rete/legacyproductionlistener.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameIntrusiveMap

namespace Blaze
{
class UserSession;

namespace Search
{

class SearchSlaveImpl;

/*! ************************************************************************************************/
/*! \brief Each UnsortedGameList is identified by an id, and contains a unique set of matchmaking criteria.

    From the client's perspective, each list has a max size (can be limited to return only the best X matches).
    However, each list actually tracks all matching games (regardless of the list's capacity). We do not do any sorting on the list
    of matching games, that is left to the client.
*************************************************************************************************/
class UnsortedGameList : public GameList
{
    NON_COPYABLE(UnsortedGameList);

public:
    UnsortedGameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId mOriginalInstanceSessionId, GameManager::ListType listType, UserSessionId ownerSessionId, bool filterOnly);
    ~UnsortedGameList() override;

    // no ops because unsorted lists are snapshot only
    void updateGame(const GameSessionSearchSlave& gameSession) override {}
    void removeGame(GameManager::GameId gameId) override {}

    bool onTokenAdded(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override;
    // again, no-op because it is a snapshot only list
    void onTokenRemoved(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override {}
    void onTokenUpdated(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override {}

    bool notifyOnTokenAdded(GameManager::Rete::ProductionInfo& pInfo) const override;
    bool notifyOnTokenRemoved(GameManager::Rete::ProductionInfo& pInfo) override;

    void processMatchingGame(const GameSessionSearchSlave& game, GameManager::FitScore fitScore, GameManager::Rete::ProductionInfo& pInfo) override;

    // no-op because we don't create intrusive nodes for the unsorted list.
    void destroyIntrusiveNode(Blaze::GameManager::GbListGameIntrusiveNode& node) override {}

protected:

    void insertGame(const GameSessionSearchSlave& gameSession);

    void insertVisibleMatch(const GameSessionSearchSlave& gameSession, GameManager::FitScore fitScore);

    bool isListAtCapacity() const override { return mVisibleListSize == mVisibleListCapacity; }

private:
    bool checkListCapacity(const GameManager::Rete::ProductionToken &token);
    size_t mVisibleListSize;
};

} // namespace Search
} // namespace Blaze
#endif // BLAZE_SEARCH_UNSORTED_GAME_LIST_H
