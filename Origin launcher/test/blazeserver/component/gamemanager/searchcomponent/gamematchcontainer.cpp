/*! ************************************************************************************************/
/*!
    \file gamesessioncontainer.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/searchcomponent/gamematchcontainer.h"

namespace Blaze
{

namespace Search
{
    GameMatchContainer::GameMatchContainer() : 
    mMatchingGameSessionMap(BlazeStlAllocator("GameSessionContainer::mMatchingGameSessionMap", DEFAULT_BLAZE_MEMGROUP)),
    mMatchingGameSessionMapInv(BlazeStlAllocator("GameSessionContainer::mMatchingGameSessionMapInv", DEFAULT_BLAZE_MEMGROUP)),
    mLastTopRank(0),
    mLastTopRankedIter(mMatchingGameSessionMap.end())
    {
   
    }


    void GameMatchContainer::erase(iterator iter)
    {
        doErase(iter->second.mMatchingGameSessionMapIterator);
        mMatchingGameSessionMapInv.erase(iter);
    }

    GameMatchContainer::MatchIter GameMatchContainer::insert(Match &match)
    {
        MatchingGameSessionInfo info;
        info.mMatchingGameSessionMapIterator = mMatchingGameSessionMap.insert(match);
        mMatchingGameSessionMapInv[match.second->gameId] = info;
        
        setTopRankLimitAfterInsertion(info.mMatchingGameSessionMapIterator);
        return info.mMatchingGameSessionMapIterator;
    }


    GameMatchContainer::MatchIter GameMatchContainer::insert(ConstMatch &match)
    {
        MatchingGameSessionInfo info;
        info.mMatchingGameSessionMapIterator = mMatchingGameSessionMap.insert(match);
        mMatchingGameSessionMapInv[match.second->gameId] = info;

        setTopRankLimitAfterInsertion(info.mMatchingGameSessionMapIterator);
        return info.mMatchingGameSessionMapIterator;
    }

    GameManager::FitScore GameMatchContainer::getWorstFitScore()
    {
        if (mMatchingGameSessionMap.empty())
            return 0;

        return mMatchingGameSessionMap.begin()->first;
    }

    void GameMatchContainer::setLastTopRank(size_t rank) 
    { 
        // this can only be set when the container is empty
        EA_ASSERT( mMatchingGameSessionMap.empty() ); 
        mLastTopRank = rank; 
    }

    bool GameMatchContainer::isTopRanked(MatchIter matchIter)
    {
        if (mLastTopRank == 0 
            || mLastTopRankedIter == mMatchingGameSessionMap.end() 
            || matchIter->first > mLastTopRankedIter->first 
            || matchIter == mLastTopRankedIter)
        {
            return true;
        }

        if (matchIter->first < mLastTopRankedIter->first)
            return false;

        // if passed in iterator comes after last top ranked iterator
        // then it's top ranked
        MatchIter it = mLastTopRankedIter;
        while (it != mMatchingGameSessionMap.end() && matchIter->first == it->first)
        {
            if (matchIter == it)
                return true;

            it++;
        }

        return false;
    }

    void GameMatchContainer::setTopRankLimitAfterInsertion(MatchIter match)
    {
        if (mLastTopRank > 0) 
        {
            if (mLastTopRankedIter == mMatchingGameSessionMap.end())
            {
                mLastTopRankedIter = match;
            }
            else if (mMatchingGameSessionMap.size() <= mLastTopRank)
            {
                mLastTopRankedIter = mMatchingGameSessionMap.begin();
            }
            else if (isTopRanked(match))
            {
                ++mLastTopRankedIter;
                if (mLastTopRankedIter == mMatchingGameSessionMap.end())
                    --mLastTopRankedIter;
            }
        }
    }

    void GameMatchContainer::doErase(MatchIter matchIter)
    {
        bool erasedIsLastRanked = false;

        if (mLastTopRank > 0)
        {
            if (matchIter == mLastTopRankedIter)
            {
                erasedIsLastRanked = true;
            }
            else if (isTopRanked(matchIter) && mMatchingGameSessionMap.size() > mLastTopRank)
            {
                --mLastTopRankedIter;   
            }
        }

        MatchIter it = mMatchingGameSessionMap.erase(matchIter);

        if (erasedIsLastRanked)
        {
            mLastTopRankedIter = it;
            if (mLastTopRankedIter != mMatchingGameSessionMap.begin())
                --mLastTopRankedIter;
        }
    }

} // namespace Search
} // namespace Blaze
