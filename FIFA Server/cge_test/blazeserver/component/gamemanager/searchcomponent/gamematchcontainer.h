/*! ************************************************************************************************/
/*!
\file gamesessioncontainer.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_SEARCH_GAMESESSIONCONTAINER_H
#define BLAZE_GAMEMANAGER_SEARCH_GAMESESSIONCONTAINER_H

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/tdf/matchmaker_types.h"
#include "EASTL/hash_map.h"
#include "EASTL/map.h"

namespace Blaze
{

namespace GameManager
{
    namespace Rete
    {
        struct ProductionInfo;
    }
}

namespace Search
{
    struct GameMatch
    {
        GameMatch(GameManager::FitScore fs, GameManager::GameId _gameId, GameManager::Rete::ProductionInfo* pi)
            : gameId(_gameId), fitScore(fs), pInfo(pi) {}
        virtual ~GameMatch() {}

        GameManager::GameId gameId;
        GameManager::FitScore fitScore;
        GameManager::Rete::ProductionInfo* pInfo;
    };

    class GameMatchContainer
    {
        NON_COPYABLE(GameMatchContainer);

    public:
        typedef eastl::multimap<GameManager::FitScore, GameMatch*> MatchingGameSessionMap;
        struct MatchingGameSessionInfo
        {
            MatchingGameSessionMap::iterator mMatchingGameSessionMapIterator;
        };
        typedef eastl::hash_map<GameManager::GameId, MatchingGameSessionInfo> MatchingGameSessionMapInv;
        typedef eastl::pair<GameManager::FitScore, GameMatch*> Match;
        typedef eastl::pair<const GameManager::FitScore, GameMatch*> ConstMatch;
        typedef MatchingGameSessionMap::iterator MatchIter;
        typedef MatchingGameSessionMapInv::iterator iterator;
        typedef MatchingGameSessionMapInv::const_iterator const_iterator;

    public:
        GameMatchContainer();
        virtual ~GameMatchContainer() {}

        /*! ************************************************************************************************/
        /*! \brief returns a match iterator to the best match in the list
            \return an iterator to the best match in the list
        ***************************************************************************************************/
        MatchIter getBestMatchIter()
        {
            MatchingGameSessionMap::reverse_iterator highestMatchedGameIter = mMatchingGameSessionMap.rbegin();
            //We need to return a forward iterator since you cannot erase a reverse_iterator. We must
            // decrement the forward iterator returned by ::base() since ::base() points to the
            // position after the position pointed to by the reverse iterator.
            return --highestMatchedGameIter.base();
        }

        /*! ************************************************************************************************/
        /*! \brief returns a match iterator to the worst match in the list
            \return an iterator to the worst match in the list
        ***************************************************************************************************/
        MatchIter getWorstMatchIter()
        {
            return mMatchingGameSessionMap.begin();
        }

        /*! ************************************************************************************************/
        /*! \brief erases the match referenced by a match iterator
            \param[in] matchIter the position at which to erase 
        ***************************************************************************************************/
        void erase(MatchIter matchIter)
        {
            mMatchingGameSessionMapInv.erase(matchIter->second->gameId);
            doErase(matchIter);
        }

        /*! ************************************************************************************************/
        /*! \brief returns an iterator corresponding to a game
            \return an iterator corresponding to a game
        ***************************************************************************************************/
        iterator find(GameManager::GameId gameId) { return mMatchingGameSessionMapInv.find(gameId); }
        const_iterator find(GameManager::GameId gameId) const { return mMatchingGameSessionMapInv.find(gameId); }
     
        /*! ************************************************************************************************/
        /*! \brief returns an iterator pointing to the start of the container
            \return an iterator pointing to the start of the container
        ***************************************************************************************************/
        iterator begin() { return mMatchingGameSessionMapInv.begin(); }
        const_iterator begin() const { return mMatchingGameSessionMapInv.begin(); }

        /*! ************************************************************************************************/
        /*! \brief returns an iterator pointing to the end of the container
            \return an iterator pointing to the end of the container
        ***************************************************************************************************/
        iterator end() { return mMatchingGameSessionMapInv.end(); }
        const_iterator end() const { return mMatchingGameSessionMapInv.end(); }

        /*! ************************************************************************************************/
        /*! \brief erases the match referenced by a supplied iterator
            \param[in] iter the position at which to erase
        ***************************************************************************************************/
        void erase(iterator iter);

        /*! ************************************************************************************************/
        /*! \brief inserts a new match into container
            \param[in] the match to insert
        ***************************************************************************************************/
        MatchIter insert(Match &match);

        /*! ************************************************************************************************/
        /*! \brief inserts a new match into container
            \param[in] the match to insert
        ***************************************************************************************************/
        MatchIter insert(ConstMatch &match);

        /*! ************************************************************************************************/
        /*! \brief returns the size of the container
            \return the size of the container
        ***************************************************************************************************/
        size_t size() const
        {
            return mMatchingGameSessionMap.size();
        }

        /*! ************************************************************************************************/
        /*! \brief returns true if the container is empty
            \return true if the container is empty
        ***************************************************************************************************/
        bool empty() const
        {
            return mMatchingGameSessionMap.empty();
        }
        
        bool contains(GameManager::GameId gameId) const { return mMatchingGameSessionMapInv.find(gameId) != mMatchingGameSessionMapInv.end(); }

        /*! ************************************************************************************************/
        /*! \brief returns fit score of the worst match
            \return fit score
        ***************************************************************************************************/
        GameManager::FitScore getWorstFitScore();

        /*! ************************************************************************************************/
        /*! \brief sets the lowest rank for the top ranked games
        ***************************************************************************************************/
        void setLastTopRank(size_t rank);

        size_t getLastTopRank() { return mLastTopRank; }

        MatchIter getLastTopRankedMatch() { return mLastTopRankedIter; }

        // does this match belong to top N ranked games in the container?
        bool isTopRanked(MatchIter matchIter);

    private:
        void setTopRankLimitAfterInsertion(MatchIter match);
        void doErase(MatchIter matchIter);


    // Members
    private:
        MatchingGameSessionMap mMatchingGameSessionMap;
        MatchingGameSessionMapInv mMatchingGameSessionMapInv;
        size_t mLastTopRank;
        MatchIter mLastTopRankedIter;
    };

} // namespace Search
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER__SEARCH_GAMESESSIONCONTAINER_H
