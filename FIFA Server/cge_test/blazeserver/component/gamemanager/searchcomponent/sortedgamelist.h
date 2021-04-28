/*! ************************************************************************************************/
/*!
    \file sortedgamelist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SEARCH_SORTED_GAME_LIST_H
#define BLAZE_SEARCH_SORTED_GAME_LIST_H

#include "gamemanager/gamebrowser/gamebrowser.h"
#include "gamemanager/searchcomponent/gamelist.h"
#include "gamemanager/rete/legacyproductionlistener.h"
#include "gamemanager/gamesessionsearchslave.h" // for GameIntrusiveMap

namespace Blaze
{
class UserSession;

namespace GameManager
{
namespace Matchmaker
{
    struct MatchmakingSupplementalData;
}
}

namespace Search
{


/*! ************************************************************************************************/
/*! \brief Each GameBrowserList is identified by an id, and contains a unique set of matchmaking criteria.

    From the client's perspective, each list has a max size (can be limited to return only the best X matches).
    However, each list actually tracks all matching games (regardless of the list's capacity).  The games
    that we show to the client are from the 'visible' container and the rest of the matching games are kept in the
    reserve container.  This lets us efficiently find the best reserved game and make it visible when a visible game
    is destroyed (or it stops being a match). The size of the visible list is the size as specified in create list 
    request regardless of fact that there can be other lists for the same requests being gathered on different search
    slave. It is Game Manager's responsibility to merge all the lists together.
    
*************************************************************************************************/
class SortedGameList : public GameList
{
    NON_COPYABLE(SortedGameList);

public:
    SortedGameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId originalInstanceSessionId, GameManager::ListType listType, UserSessionId owner, bool filterOnly);
    ~SortedGameList() override;

    void updateGame(const GameSessionSearchSlave& gameSession) override;
    void removeGame(GameManager::GameId id) override;

    bool onTokenAdded(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override;
    void onTokenRemoved(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override;
    void onTokenUpdated(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override;

    bool notifyOnTokenAdded(GameManager::Rete::ProductionInfo& pInfo) const override;
    bool notifyOnTokenRemoved(GameManager::Rete::ProductionInfo& pInfo) override;

    void processMatchingGame(const GameSessionSearchSlave& game, GameManager::FitScore fitScore, GameManager::Rete::ProductionInfo& pInfo) override;

    void destroyIntrusiveNode(Blaze::GameManager::GbListGameIntrusiveNode& node) override;

    void addNextBestGame();

protected:
    GameManager::FitScore getFitScore(const GameSessionSearchSlave& game) const { return mVisibleGameSessionContainer.find(game.getGameId())->second.mMatchingGameSessionMapIterator->first; }

    void insertVisibleMatch(GameMatchContainer::Match& match);
    void removeVisibleMatch(const GameMatchContainer::MatchIter& itr);

    void destroyGameInfo(GameMatchContainer::iterator visibleIter);

private:

    bool nextToken(const GameManager::Rete::ProductionInfo& pInfo, const GameManager::Rete::ProductionToken*& token) const;

    Blaze::GameManager::GbListGameIntrusiveNode* createIntrusiveNode(GameSessionSearchSlave& game);

    bool checkListCapacity(GameManager::FitScore totalFitScore, const GameManager::Rete::ProductionToken &token);
private:

    Blaze::GameManager::GameIntrusiveMap mGameIntrusiveMap;

};

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_SORTED_FILTERED_GAME_BROWSER_LIST_H
