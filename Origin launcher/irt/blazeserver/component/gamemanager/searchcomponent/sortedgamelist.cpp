/*! ************************************************************************************************/
/*!
    \file sortedgamelist.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/searchcomponent/sortedgamelist.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamesessionsearchslave.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "gamemanager/searchcomponent/gamematchcontainer.h"

namespace Blaze
{

namespace Search
{

/*! ************************************************************************************************/
/*! \brief constructor; list is not ready for use until it's also been initialized.
***************************************************************************************************/
    SortedGameList::SortedGameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId originalInstanceSessionId, GameManager::ListType listType, UserSessionId owner, bool filterOnly)
    : GameList(searchSlave, gameBrowserListId, originalInstanceSessionId, listType, owner, filterOnly),
    mGameIntrusiveMap(BlazeStlAllocator("SortedGameList::mGameIntrusiveMap", SearchSlave::COMPONENT_MEMORY_GROUP))
{
}

/*! ************************************************************************************************/
/*! \brief destructor
***************************************************************************************************/
SortedGameList::~SortedGameList()
{
    GameMatchContainer::iterator visibleIter = mVisibleGameSessionContainer.begin();
    GameMatchContainer::iterator visibleEnd = mVisibleGameSessionContainer.end();
    for (; visibleIter != visibleEnd; ++visibleIter)
    {
        destroyGameInfo(visibleIter);
    }

    //Drop all our references.
    Blaze::GameManager::GameIntrusiveMap::iterator iter = mGameIntrusiveMap.begin();
    Blaze::GameManager::GameIntrusiveMap::iterator end = mGameIntrusiveMap.end();
    while (iter != end)
    {
        // store off our value before we increment.
        Blaze::GameManager::GbListGameIntrusiveNode &n = *iter->second;
        // increment here so we can delete.
        ++iter;
        // Note: we don't call destroyIntrusiveNode here to prevent from erase from the gameMap, which
        // is just getting destroyed by this destructor.
        mSearchSlave.deleteGameIntrusiveNode(&n);
    }
    mGameIntrusiveMap.clear();
}

// RETE Network Listener
bool SortedGameList::onTokenAdded(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info)
{
   GameSessionSearchSlave* game = mSearchSlave.getGameSessionSlave(token.mId);

    if (EA_UNLIKELY(game == nullptr))
    {
        // This should never happen, but if it does it means that the game was deleted before we finished processing.
        ERR_LOG("[SortedGameList].onTokenAdded game(" << token.mId << ") not found for list(" << mGameBrowserListId << ")");
        return true;
    }

    if (!game->isJoinableForUser(mOwnerSessionInfo, mIgnoreGameEntryCriteria))
    {
        TRACE_LOG("[SortedGameList].onTokenAdded game (" << token.mId << ") not joinable for owner(" << mOwnerSessionId << ")");
        return true;
    }

    GameManager::DebugRuleResultMap debugResultMap;
    GameManager::FitScore additionalFitScore = mSearchSlave.evaluateAdditionalFitScoreForGame(*game, info, debugResultMap);
    GameManager::FitScore totalReteFitScore = info.fitScore + additionalFitScore;

    if(!checkListCapacity(totalReteFitScore + mCriteria.getMaxPossibleNonReteFitScore() + mCriteria.getMaxPossiblePostReteFitScore(), token))
    {
        TRACE_LOG("[SortedGameList].onTokenAdded Capacity Full (" << mVisibleListCapacity 
            << ") already for list id(" << mGameBrowserListId << "), aborting adding game " << token.mId);
        return false;
    }

    GameManager::FitScore postReteFitScore = mCriteria.evaluateGameReteFitScore(*game, debugResultMap);

    if(!checkListCapacity(totalReteFitScore + postReteFitScore + mCriteria.getMaxPossibleNonReteFitScore(), token))
    {
        TRACE_LOG("[SortedGameList].onTokenAdded Capacity Full (" << mVisibleListCapacity 
            << ") already for list id(" << mGameBrowserListId << "), aborting adding game " << token.mId);
        return false;
    }

    GameManager::FitScore nonReteRuleFitScore = mCriteria.evaluateGame(*game, debugResultMap);

    GameManager::FitScore totalFitScore = totalReteFitScore + postReteFitScore + nonReteRuleFitScore;

    EA_ASSERT_MSG((mVisibleGameSessionContainer.size() <= mVisibleListCapacity), "reserve map should never exceed its capacity");
    if(!checkListCapacity(totalFitScore, token))
    {
        TRACE_LOG("[SortedGameList].onTokenAdded Capacity Full (" << mVisibleListCapacity 
            << ") already for list id(" << mGameBrowserListId << "), aborting adding game " << token.mId);
        return false;
    }

    // see if our node already exists as reference.
    GameManager::GameIntrusiveMap::iterator iter = mGameIntrusiveMap.find(game->getGameId());

    // create the intrusive nodes here
    GameManager::GbListGameIntrusiveNode *node = nullptr;
    if (iter != mGameIntrusiveMap.end())
    {
        TRACE_LOG("[SortedGameList].onTokenAdded WARN attempted to add same token twice - session '" 
            << getProductionId() << "' Token id '" << token.mId << "'");
        node = iter->second;
    }
    else
    {
        // When a game is added through our RETE network we need to 
        // listen for updates.
        node = createIntrusiveNode(*game);
    }

    // Check that the non rete rules match this game and that the game is joinable.
    if (GameManager::Matchmaker::isFitScoreMatch(nonReteRuleFitScore))
    {
        processMatchingGame(*game, totalFitScore, info);
        TRACE_LOG("[SortedGameList].onTokenAdded MATCH list(" << mGameBrowserListId << ") game(" << token.mId 
            << "), rete FitScore '" << info.fitScore << "' + postRete FitScore '" << postReteFitScore << "' + nonRete FitScore '" << nonReteRuleFitScore << "' + additional FitScore '" 
            << additionalFitScore << "' = total FitScore '" << totalFitScore << "'.");
    }
    else
    {
        TRACE_LOG("[SortedGameList].onTokenAdded NO_MATCH list(" << mGameBrowserListId << ") game(" 
            << game->getGameId() << "), rete FitScore '" << info.fitScore << "' + postRete FitScore '" << postReteFitScore << "' + nonRete FitScore '" << nonReteRuleFitScore 
            << "' + additional FitScore '" << additionalFitScore << "' = total FitScore '" << totalFitScore << "'");
    }

    node->mNonReteFitScore = nonReteRuleFitScore;
    node->mReteFitScore = totalReteFitScore;
    node->mPostReteFitScore = postReteFitScore;
    node->mPInfo = &info;

    return true;
}

void SortedGameList::onTokenRemoved(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info)
{
    TRACE_LOG("[SortedGameList].onTokenRemoved GAME BROWSER LIST(" << mGameBrowserListId << ") TOKEN REMOVED id(" << token.mId << ") GameManager::FitScore(" << info.fitScore << ").");

    // remove this list's association with this game
   GameManager::GameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(token.mId);
    if (itr != mGameIntrusiveMap.end())
    {
        GameManager::GbListGameIntrusiveNode &n = *itr->second;
        destroyIntrusiveNode(n);
    }
    else
    {
        TRACE_LOG("[SortedGameList].onTokenRemoved list(" << mGameBrowserListId << ") GameManager::FitScore not found for game(" << token.mId << ")");
    }
    
    GameSessionSearchSlave* game = mSearchSlave.getGameSessionSlave(token.mId);
    if (EA_LIKELY(game != nullptr))
    {
        TRACE_LOG("[SortedGameList].onTokenRemoved NO_MATCH list(" << mGameBrowserListId << "') game(" << game->getGameId() << ")");
        removeGame(game->getGameId());

        // If there is another game waiting, add it right away to keep the game list up to date.
        // Note: We do not want to delay this addNextBestGame call into next idle, since the above removed game might be part of a RETE token update 
        // (when RETE tree is updating a token, it will first remove it then re-insert it with new fitscore right after), so the updated game will be inserted back into the game list successfully 
        // during current idle even its fitscore might be worse than the nextBestGame's.
        addNextBestGame();
    }
    else
    {
        // This should never happen, but if it does it means that the game was deleted before we finished processing.
        ERR_LOG("[SortedGameList].onTokenRemoved game(" << token.mId << ") not found for list(" << mGameBrowserListId << ")");
    }
}


void SortedGameList::onTokenUpdated(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info)
{
    // Find the game in our matched game map.
    GameManager::GameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(token.mId);
    if (itr != mGameIntrusiveMap.end())
    {
        GameManager::GbListGameIntrusiveNode &n = *itr->second;

        GameManager::DebugRuleResultMap debugResultMap;
        // Note that only these rules can possibly be updated by the RETE tree currently.  So no reason 
        // to re-run addition fit score, or non rete rules.
        GameManager::FitScore postReteFitScore = mCriteria.evaluateGameReteFitScore(n.mGameSessionSlave, debugResultMap);
        n.mPostReteFitScore = postReteFitScore;

        TRACE_LOG("[SortedGameList].onTokenAdded MATCH list(" << mGameBrowserListId << ") game(" << token.mId 
            << " new postReteFitScore '" << postReteFitScore << "' and total fitScore '" << n.getTotalFitScore() << "'");
    }
    // otherwise we didn't cache off the match, so just do nothing.
}


// TODO: Searching the Beta Memory for a new token is expensive for the remove.
// Profile this method and potentially create a new worst iterator class
// that contains the worst pInfo (pNode) and also the worst token from that
// pNode.  The tricky part is how to handle removes that could potentially
// invalidate the token iterator.
bool SortedGameList::nextToken(const GameManager::Rete::ProductionInfo& pInfo, const GameManager::Rete::ProductionToken*& token) const
{
    GameManager::Rete::BetaMemory::const_iterator memItr = pInfo.pNode->getTokens().begin();
    GameManager::Rete::BetaMemory::const_iterator memEnd = pInfo.pNode->getTokens().end();

    for (; memItr != memEnd; ++memItr)
    {
        const GameManager::Rete::ProductionToken& curToken = *memItr->second;

        if (mGameIntrusiveMap.find(curToken.mId) == mGameIntrusiveMap.end())
        {
            // new token found that isn't in the list!
            token = &curToken;
            return true;
        }
    }

    return false;
}

void SortedGameList::removeGame(GameManager::GameId id)
{
    GameMatchContainer::iterator itr = mVisibleGameSessionContainer.find(id);

    if (itr != mVisibleGameSessionContainer.end())
    {
        removeVisibleMatch(itr->second.mMatchingGameSessionMapIterator);
    }
}

// This method is used to update dirty games for non RETE rules
// If there are no RETE rules then this method should NOOP.
void SortedGameList::updateGame(const GameSessionSearchSlave& game)
{
    if (mOwnerSessionId == UserSession::INVALID_SESSION_ID)
    {
        TRACE_LOG("[SortedGameList].updateGame ownerless list id(" << mGameBrowserListId << ") processing update for game '" << game.getGameId() << "'");
    }
    else if (!gUserSessionManager->getSessionExists(mOwnerSessionId))
    {
        TRACE_LOG("[SortedGameList].updateGame owner(" << mOwnerSessionId << ") of this list id(" << mGameBrowserListId << ") has logged out");
        return;
    }

    //is this game already a match?
    bool previouslyMatch = mVisibleGameSessionContainer.contains(game.getGameId());

    GameManager::DebugRuleResultMap debugResultMap;
    EntryCriteriaName criteriaName;
    GameManager::FitScore startingFitScore = 0;
    GameManager::Rete::ProductionInfo* pInfo = nullptr;

    GameManager::GameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(game.getGameId());
    if (itr == mGameIntrusiveMap.end())
    {
        // TODO: Can we handle this case with out pInfo without erroring out?
        // Should the game get removed?
        ERR_LOG("[SortedGameList].updateGame list '" << mGameBrowserListId << "' missing starting GameManager::FitScore for game '" << game.getGameId() << "'");
        return;
    }

    GameManager::FitScore nonReteRuleFitScore = mCriteria.evaluateGame(game, debugResultMap);
    GameManager::GbListGameIntrusiveNode &n = *itr->second;
    startingFitScore = n.mReteFitScore + n.mPostReteFitScore;
    n.mNonReteFitScore = nonReteRuleFitScore;      // Update the nonRete score with the latest results
    pInfo = n.mPInfo;
    
    bool currentMatch = (GameManager::Matchmaker::isFitScoreMatch(nonReteRuleFitScore)) && (game.isJoinableForUser(mOwnerSessionInfo, mIgnoreGameEntryCriteria));

    GameManager::FitScore totalFitScore = startingFitScore + nonReteRuleFitScore;

    //did the match status change?
    //If snapshot, its past initial search, so no op
    if (currentMatch && (mListType != GameManager::GAME_BROWSER_LIST_SNAPSHOT))
    {
        TRACE_LOG("[SortedGameList].updateGame MATCH list(" << mGameBrowserListId << ") game(" << game.getGameId() 
            << "), rete FitScore '" << startingFitScore << "' + nonRete FitScore '" << nonReteRuleFitScore << "' = total FitScore '" 
            << totalFitScore << "'");
        processMatchingGame(game, totalFitScore, *pInfo);
    }
    else if (previouslyMatch && //don't bother with something that was not already a match
        (mListType != GameManager::GAME_BROWSER_LIST_SNAPSHOT))
    {
        TRACE_LOG("[SortedGameList].updateGame NO_MATCH list(" << mGameBrowserListId << ") game(" << game.getGameId() 
            << "), rete FitScore '" << startingFitScore << "' + nonRete FitScore '" << nonReteRuleFitScore << "' = total FitScore '" 
            << totalFitScore << "'");
        //Take out the game, don't remove the ref as its still a match in our p-node
        removeGame(game.getGameId());
    }
}

/*! ************************************************************************************************/
/*! \brief Intrusive structures create and referencing
\param[in] game - to reference
***************************************************************************************************/
Blaze::GameManager::GbListGameIntrusiveNode* SortedGameList::createIntrusiveNode(GameSessionSearchSlave& game)
{
    // When a game is added through our RETE network we need to 
    // listen for updates.
    Blaze::GameManager::GbListGameIntrusiveNode* node = mSearchSlave.createGameInstrusiveNode(game, *this);

    mGameIntrusiveMap.insert(eastl::make_pair(game.getGameId(), node));

    game.addReference(*node);

    return node;
}

/*! ************************************************************************************************/
/*! \brief Intrusive structures cleanup and references removal
***************************************************************************************************/
void SortedGameList::destroyIntrusiveNode(Blaze::GameManager::GbListGameIntrusiveNode& node)
{
    GameManager::GameIntrusiveMap::iterator iter = mGameIntrusiveMap.find(node.getGameId());
    if (iter != mGameIntrusiveMap.end())
    {
        mGameIntrusiveMap.erase(iter);
    }

    mSearchSlave.deleteGameIntrusiveNode(&node);
}

/*! ************************************************************************************************/
/*! \brief if the initial search is complete then we are interested in this node
only if the list isn't at capacity or if we have found a better fit
score with this node, and, given its not snapshot (which never updates)
***************************************************************************************************/
bool SortedGameList::notifyOnTokenAdded(GameManager::Rete::ProductionInfo& pInfo) const
{
    return ((mListType != GameManager::GAME_BROWSER_LIST_SNAPSHOT) &&
        (mInitialSearchComplete && (!isListAtCapacity() || pInfo.fitScore > (**mWorstPInfoItr).fitScore)));
}

bool SortedGameList::notifyOnTokenRemoved(GameManager::Rete::ProductionInfo& pInfo)
{
    return ((mListType != GameManager::GAME_BROWSER_LIST_SNAPSHOT) &&
        !mGameIntrusiveMap.empty());
}

/*! ************************************************************************************************/
/*! \brief add/update a matching game to this list's containers
***************************************************************************************************/
void SortedGameList::processMatchingGame(const GameSessionSearchSlave &game, GameManager::FitScore fitScore, GameManager::Rete::ProductionInfo& pInfo)
{
    // deal with a matching game; we need to determine:
    //   1) is this a new match (first insertion) or an update of an existing game (still matching, but an updated GameManager::FitScore)
    //   2) does this game belong in the visible list

    // sanity check the state of the visible container
    EA_ASSERT_MSG( (mVisibleGameSessionContainer.size() <= mVisibleListCapacity), "visible map should never exceed its capacity");

    // if the game is already present in the visible lists (and the GameManager::FitScore changed), remove it from the list
    GameMatchContainer::iterator lookupIter = mVisibleGameSessionContainer.find(game.getGameId());
    if (lookupIter != mVisibleGameSessionContainer.end())
    {
        mSearchSlave.incrementMetric_VisibleGamesUpdatesByIdleEnd();
        GameManager::FitScore prevFitScore = lookupIter->second.mMatchingGameSessionMapIterator->first;
        if (prevFitScore == fitScore)
        {
            // just send an game updated notification so the client can get the changes
            //   (no GameManager::FitScore change, so game position is the same).
            markGameAsUpdated(game.getGameId(), fitScore, false /* newGame */);
            return;
        }

        // Game will get added back below because the list will no
        // longer be at capacity.  No need to mark the game
        // as removed, will get marked as updated.
        GameMatch* gbGameMatch = static_cast<GameMatch*>(lookupIter->second.mMatchingGameSessionMapIterator->second);
        mVisibleGameSessionContainer.erase(lookupIter);
        // we decrement this here since it will be re-incremented when we insertVisibleMatch.
        // this could also be a different pInfo, than what we matched.
        --gbGameMatch->pInfo->numActiveTokens;
        mSearchSlave.deleteGameMatch(gbGameMatch);
    }
    else
    {
        // Newly matched game
        mSearchSlave.incrementMetric_GameUpdatesByIdleEnd();
    }

    GameMatch* gbGameMatch = mSearchSlave.createGameMatch(fitScore, game.getGameId(), &pInfo);
    GameMatchContainer::Match gameMatch(fitScore, gbGameMatch);

    if (!isListAtCapacity())
    {
        // if the visible list wasn't full to begin with, just insert into it
        // TODO: Shall we check the current match game with the nextBestGame and insert the better one?
        insertVisibleMatch(gameMatch);
    }
    else if (fitScore <= mVisibleGameSessionContainer.getWorstMatchIter()->first)
    {
        // if trying to add a new worst-match, but was full capacity, abort
        TRACE_LOG("[SortedGameList].processMatchingGame List is Full (" << mVisibleListCapacity 
            << ") already for list id(" << mGameBrowserListId << "), aborting adding new matched game " << game.getGameId() 
            << " to invisible reserve");
        mSearchSlave.deleteGameMatch(gbGameMatch);
        return;
    }
    else if (!mVisibleGameSessionContainer.empty())
    {
        // the visible list is full (and not a 0 size list)
        // and we have a better match
        //  - remove our worst match
        //  - insert this match
        GameMatchContainer::MatchIter worstVisibleMatchIter = mVisibleGameSessionContainer.getWorstMatchIter();
        GameMatch* worstGameMatch = static_cast<GameMatch*>(worstVisibleMatchIter->second);

        // we can insert directly into the visible list, since the new/updated game is better than the
        //   worstVisible game
        TRACE_LOG("[SortedGameList].processMatchingGame List(" << mGameBrowserListId << ") removing worst match game("
            << worstGameMatch->gameId << ") and adding game(" << gbGameMatch->gameId << ").");
        // We mimic onTokenRemoved here, as we don't want onTokenRemoved to process the next matching game for us, we want to
        // insert ours.  May want to look into better ways of handling this (functioning out more of onTokenRemoved).
        GameManager::GameIntrusiveMap::iterator itr = mGameIntrusiveMap.find(worstGameMatch->gameId);
        if (itr != mGameIntrusiveMap.end())
        {
            GameManager::GbListGameIntrusiveNode &n = *itr->second;
            destroyIntrusiveNode(n);
        }
        else
        {
            TRACE_LOG("[SortedGameList].processMatchingGame list(" << mGameBrowserListId 
                << ") GameManager::FitScore not found for game(" << worstGameMatch->gameId << ")");
        }
        removeVisibleMatch(worstVisibleMatchIter);

        insertVisibleMatch(gameMatch);
    }
    else
    {
        WARN_LOG("[SortedGameList].processMatchmakingGame list(" << mGameBrowserListId << ") failed to insert game(" 
            << game.getGameId() << "), list capacity(" << mVisibleGameSessionContainer.size() << ").");
        mSearchSlave.deleteGameMatch(gbGameMatch);
    }

    EA_ASSERT(mVisibleGameSessionContainer.size() <= mVisibleListCapacity);
}

void SortedGameList::insertVisibleMatch(GameMatchContainer::Match& match)
{
    GameMatch& gbGameMatch = *static_cast<GameMatch*>(match.second);

    EA_ASSERT(mVisibleGameSessionContainer.size() < mVisibleListCapacity);

    mSearchSlave.incrementMetric_VisibleGamesUpdatesByIdleEnd();

    mVisibleGameSessionContainer.insert( match );
    markGameAsUpdated(gbGameMatch.gameId, match.first /*fitScore*/ , true /*newGame*/);


    EA_ASSERT(gbGameMatch.pInfo->numActiveTokens < gbGameMatch.pInfo->pNode->getTokenCount() );
    ++gbGameMatch.pInfo->numActiveTokens;

    // Search for the new worst pinfo iterator.  Search ends if we reach
    // the end of the list, or the GameManager::FitScore of the next pinfo is worse
    // than the token that was added.
    if (mWorstPInfoItr == mProductionInfoByFitscore.end()) mWorstPInfoItr = mProductionInfoByFitscore.begin();

    ProductionInfoByFitScore::iterator curItr = mWorstPInfoItr;
    ProductionInfoByFitScore::iterator end = mProductionInfoByFitscore.end();
    while (++curItr != end && (*gbGameMatch.pInfo).fitScore <= (**curItr).fitScore)
    {
        GameManager::Rete::ProductionInfo& pInfo = **curItr;
        if (pInfo.numActiveTokens != 0)
        {
            // Only adding 1 token so if we find a worst pInfo that has tokens
            // then it must be the one.
            mWorstPInfoItr = curItr;
            break;
        }
    }
}

void SortedGameList::removeVisibleMatch(const GameMatchContainer::MatchIter& itr)
{
    GameMatch* gbGameMatch = static_cast<GameMatch*>(itr->second);

    EA_ASSERT(!mVisibleGameSessionContainer.empty());
    EA_ASSERT(gbGameMatch->pInfo->numActiveTokens != 0);

    mSearchSlave.incrementMetric_VisibleGamesUpdatesByIdleEnd();
    mVisibleGameSessionContainer.erase(itr);

    markGameAsRemoved(gbGameMatch->gameId);
    --gbGameMatch->pInfo->numActiveTokens;

    mSearchSlave.deleteGameMatch(gbGameMatch);

    // May no longer be the worst pInfo
    // If this remove, removed the last token from the worst pNode then
    // search for the next worst pNode with tokens
    while (((**mWorstPInfoItr).numActiveTokens == 0) &&
        (mWorstPInfoItr != mProductionInfoByFitscore.begin()))
    {
        --mWorstPInfoItr;
    }
}

void SortedGameList::addNextBestGame()
{
    // After the remove the list is now below capacity, check to see if there are any games to add.
    ProductionInfoByFitScore::iterator curItr = mWorstPInfoItr;
    ProductionInfoByFitScore::iterator end = mProductionInfoByFitscore.end();
    while (curItr != end)
    {
        GameManager::Rete::ProductionInfo& pInfo = **curItr;
        // If this production info has more tokens than are active
        // find the next token and add it.
        if (pInfo.pNode->getTokenCount() > pInfo.numActiveTokens)
        {
            const GameManager::Rete::ProductionToken* token = nullptr;
            if (nextToken(pInfo, token))
            {
                onTokenAdded(*token, pInfo);
                return;
            }
            else
            {
                TRACE_LOG("[SortedGameList].addNextBestGame list id(" << mGameBrowserListId << ") found no new tokens");
            }
        }
        ++curItr;
    }
}

void SortedGameList::destroyGameInfo(GameMatchContainer::iterator visibleIter)
{
    GameMatch *gbGameMatch = static_cast<GameMatch*>(visibleIter->second.mMatchingGameSessionMapIterator->second);
    mSearchSlave.deleteGameMatch(gbGameMatch);
}

bool SortedGameList::checkListCapacity(GameManager::FitScore totalFitScore, const GameManager::Rete::ProductionToken &token)
{
    // GB_AUDIT: We may wish to move this above the checking for null game if we find that onTokenAdded is taking
    // too much time during stress testing.
    // If list is at both visible and invisible capacity abort
    if (isListAtCapacity() && (totalFitScore <= mVisibleGameSessionContainer.getWorstMatchIter()->first))
    {
        return false;
    }

    return true;
}

} // namespace Search
} // namespace Blaze
