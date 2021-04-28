/*! ************************************************************************************************/
/*!
    \file UnsortedGameList.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/searchcomponent/unsortedgamelist.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"

namespace Blaze
{
namespace Search
{

/*! ************************************************************************************************/
/*! \brief constructor; list is not ready for use until it's also been initialized.
***************************************************************************************************/
UnsortedGameList::UnsortedGameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId mOriginalInstanceSessionId, GameManager::ListType listType, UserSessionId ownerSessionId, bool filterOnly)
    : GameList(searchSlave, gameBrowserListId, mOriginalInstanceSessionId, listType, ownerSessionId, filterOnly),
      mVisibleListSize(0)
{ 
}

/*! ************************************************************************************************/
/*! \brief destructor
***************************************************************************************************/
UnsortedGameList::~UnsortedGameList()
{
}

// RETE Network Listener
bool UnsortedGameList::onTokenAdded(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info)
{   
    if (!gUserSessionManager->getSessionExists(mOwnerSessionId))
    {
        TRACE_LOG("[UnsortedGameList].onTokenAdded owner(" << mOwnerSessionId << ") of this list id(" << mGameBrowserListId << ") has logged out");
        return true;
    }

    GameSessionSearchSlave* game = mSearchSlave.getGameSessionSlave(token.mId);

    if (EA_UNLIKELY(game == nullptr))
    {
        // This should never happen, but if it does it means that the game was deleted before we finished processing.
        ERR_LOG("[UnsortedGameList].onTokenAdded game(" << token.mId << ") not found for list(" << mGameBrowserListId << ")");
        return true;
    }

    // evaluate post-RETE rules.
    // Don't evaluate additional fit score, as it won't impact match/no match status, and we don't rank results for this list.
    GameManager::DebugRuleResultMap debugResultMap;
    GameManager::FitScore nonReteRuleFitScore = mCriteria.evaluateGame(*game, debugResultMap);
    if (!GameManager::Matchmaker::isFitScoreMatch(nonReteRuleFitScore))
    {
        TRACE_LOG("[UnsortedGameList].onTokenAdded NO_MATCH after post-RETE evaluation for list(" << mGameBrowserListId << ") game(" 
            << token.mId << "), rete fitscore '" << info.fitScore << "'.");
        return false;
    }

    info.fitScore += nonReteRuleFitScore;
    GameManager::FitScore postReteFitScore = mCriteria.evaluateGameReteFitScore(*game, debugResultMap);
    info.fitScore += postReteFitScore;

    if(!checkListCapacity(token))
        return false;

    processMatchingGame(*game, info.fitScore, info);
    ++mVisibleListSize;
    TRACE_LOG("[UnsortedGameList].onTokenAdded MATCH list(" << mGameBrowserListId << ") game(" 
        << token.mId << "), rete fitscore '" << info.fitScore << "'.");

    return true;
}

void UnsortedGameList::insertVisibleMatch(const GameSessionSearchSlave& gameSession, GameManager::FitScore fitScore)
{
    EA_ASSERT(mVisibleListSize < mVisibleListCapacity);
    markGameAsUpdated(gameSession.getGameId(), fitScore, true);
    mSearchSlave.incrementMetric_VisibleGamesUpdatesByIdleEnd();
}

/*! ************************************************************************************************/
/*! \brief add/update a matching game to this list's containers
***************************************************************************************************/
void UnsortedGameList::processMatchingGame(const GameSessionSearchSlave &game, GameManager::FitScore fitScore, GameManager::Rete::ProductionInfo& pInfo)
{
    // deal with a matching game; we need to determine:
    //   1) is this a new match (first insertion) or an update of an existing game (still matching, but an updated fitScore)
    //   2) does this game belong in the visible list

    // sanity check the state of the visible container
    EA_ASSERT_MSG( (mVisibleListSize <= mVisibleListCapacity), "visible map should never exceed its capacity");

    mSearchSlave.incrementMetric_GameUpdatesByIdleEnd();

    if (!isListAtCapacity())
    {
        // if the visible list wasn't full to begin with, just insert into it
        insertVisibleMatch(game, fitScore);
    }
}

bool UnsortedGameList::checkListCapacity(const GameManager::Rete::ProductionToken &token)
{
    // GB_AUDIT: We may wish to move this above the checking for null game if we find that onTokenAdded is taking
    // too much time during stress testing.
    // If list is at both visible and invisible capacity abort
    EA_ASSERT_MSG( (mVisibleListSize <= mVisibleListCapacity), "reserve map should never exceed its capacity");
    if (isListAtCapacity())
    {
        TRACE_LOG("[UnsortedGameList].onTokenAdded Capacity Full (" << mVisibleListCapacity << ") already for list id(" << mGameBrowserListId << "), aborting adding game " << token.mId);
        return false;
    }

    return true;
}

bool UnsortedGameList::notifyOnTokenAdded(GameManager::Rete::ProductionInfo& pInfo) const
{
    return !isListAtCapacity();
}

bool UnsortedGameList::notifyOnTokenRemoved(GameManager::Rete::ProductionInfo& pInfo)
{
    return false;
}

} // namespace Search
} // namespace Blaze
