/*! ************************************************************************************************/
/*!
    \file   teamcompositionscommoninfo.h

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEMANAGER_TEAM_COMPOSITIONS_COMMON_INFO_H
#define BLAZE_GAMEMANAGER_TEAM_COMPOSITIONS_COMMON_INFO_H

#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/matchmaker/matchmakingutil.h"

namespace Blaze
{
namespace GameManager
{

    inline uint16_t calcNumOfGroupsInGroupSizeCountMaps(const GroupSizeCountMapByTeamList& groupSizeCountMapByTeamList)
    {
        size_t result = 0;
        for (size_t tIndex = 0; tIndex < groupSizeCountMapByTeamList.size(); ++tIndex)
        {
            for (GroupSizeCountMap::const_iterator itr = groupSizeCountMapByTeamList[tIndex]->begin(); itr != groupSizeCountMapByTeamList[tIndex]->end(); ++itr)
                result += itr->second;
        }
        return (uint16_t)result;
    }

namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief A team's composition. A vector of group sizes that can make up a team. (Non-group solo sessions are group size 1).
    ***************************************************************************************************/
    typedef eastl::vector<uint16_t> TeamComposition;

    /*! ************************************************************************************************/
    /*! \brief list of team compositions
    ***************************************************************************************************/
    typedef eastl::vector<TeamComposition> TeamCompositionVector;

    inline const char8_t* teamCompositionToLogStr(const TeamComposition& composition, eastl::string& buf)
    {
        buf.append("{");
        for (size_t j = 0; j < composition.size(); ++j)
            buf.append_sprintf("%u%s", composition[j], ((j+1 < composition.size())? "," : ""));
        return buf.append("}").c_str();
    }

    inline const char8_t* teamCompositionVectorToLogStr(const TeamCompositionVector& tcVector, eastl::string& buf, bool isForGameTeamCompositions = true)
    {
        const char8_t* delimiter = (isForGameTeamCompositions? "vs" : "\n\t");//delimit by whether for a 'gtc' or simple list of 'tc's.
        for (size_t i = 0; i < tcVector.size(); ++i)
        {
            eastl::string grpSizeBuf;
            buf.append_sprintf("%s%s", teamCompositionToLogStr(tcVector[i], grpSizeBuf), ((i+1 < tcVector.size())? delimiter:""));
        }
        return buf.c_str();
    }

    /*! ************************************************************************************************/
    /*! \brief GameTeamCompositions id for a tuple of team compositions for the potential teams in a game (unique per rule).
    ***************************************************************************************************/
    typedef int GameTeamCompositionsId;
    const GameTeamCompositionsId INVALID_GAME_TEAM_COMPOSITONS_ID = -1;

    /*! ************************************************************************************************/
    /*! \brief Holds info about a tuple of team compositions for the potential teams in a game.
    ***************************************************************************************************/
    struct GameTeamCompositionsInfo
    {
        GameTeamCompositionsInfo() : mGameTeamCompositionsId(INVALID_GAME_TEAM_COMPOSITONS_ID), mCachedGroupSizeCountsByTeam(nullptr), mCachedFitPercent(FIT_PERCENT_NO_MATCH) {}
        GameTeamCompositionsId mGameTeamCompositionsId; //unique per-rule id for this game team compositions info
        TeamCompositionVector mTeamCompositionVector;   //the tuple of team compositions for the potential teams in a game. Each index corresponds to its respective team's TeamIndex
        const GroupSizeCountMapByTeamList* mCachedGroupSizeCountsByTeam;//note: as a tdf, must be ptr (not plain member) to allow copying and sorting eastl lists of this container
        float mCachedFitPercent;
        bool mIsExactMatch;

        const GroupSizeCountMapByTeamList* getCachedGroupSizeCountsByTeam() const 
        {
            if ((mCachedGroupSizeCountsByTeam == nullptr) || mCachedGroupSizeCountsByTeam->empty())
                WARN_LOG("[GameTeamCompositionsInfo].getGroupSizeCountsByTeamList internal error: group sizes count data not initialized.");
            return mCachedGroupSizeCountsByTeam;
        }
        uint16_t getNumOfGroups() const { return ((getCachedGroupSizeCountsByTeam() == nullptr)? 0 : calcNumOfGroupsInGroupSizeCountMaps(*getCachedGroupSizeCountsByTeam())); }

        const char8_t* toFullLogStr() const { return (!mLogStr.empty()? mLogStr.c_str() : mLogStr.append_sprintf("GtcId=%" PRIi64 ",fitPct=%1.2f,teamcomps=%s,groupSizeCountsCached=%s", mGameTeamCompositionsId, mCachedFitPercent, toCompositionsLogStr(), (getCachedGroupSizeCountsByTeam() != nullptr && !getCachedGroupSizeCountsByTeam()->empty()? "yes":"no(ERROR)")).c_str()); }
        const char8_t* toCompositionsLogStr() const { return (!mCompositionsLogStr.empty()? mCompositionsLogStr.c_str() : teamCompositionVectorToLogStr(mTeamCompositionVector, mCompositionsLogStr)); }
    private:
        mutable eastl::string mLogStr;//lazy init
        mutable eastl::string mCompositionsLogStr;
    };

    /*! ************************************************************************************************/
    /*! \brief vector of possible game's team's compositions (note: see sorting comparitor in team composition rule definition)
    ***************************************************************************************************/
    typedef eastl::vector<GameTeamCompositionsInfo> GameTeamCompositionsInfoVector;

    /*! \brief Return whether both team composition's vector of group sizes are identical. */
    inline bool areTeamCompositionsEqual(const TeamComposition& a, const TeamComposition& b)
    {     
        const size_t groupCountA = a.size();
        const size_t groupCountB = b.size();
        if (groupCountA != groupCountB)
            return false;

        // pre: group sizes, in each team composition sorted non-ascending
        for (size_t i = 0; i < groupCountA; ++i)
        {
            if (a[i] != b[i])
                return false;
        }
        return true;
    }
    /*! \brief Return whether both game team composition info's vector of team compositions, are identical */
    inline bool areGameTeamCompositionsEqual(const TeamCompositionVector& a, const TeamCompositionVector& b)
    {
        const size_t teamCountA = a.size();
        const size_t teamCountB = b.size();
        if (teamCountA != teamCountB)
            return false;

        for (size_t teamInd = 0; teamInd < teamCountA; ++teamInd)
        {
            if (!areTeamCompositionsEqual(a[teamInd], b[teamInd]))
                return false;
        }
        return true;
    }
    /*! \brief Return whether all team compositions in both game team compositions infos, are identical to each other, i.e. 'exact match' */
    inline bool areAllTeamCompositionsEqual(const GameTeamCompositionsInfo& gtcInfo)
    {
        if (gtcInfo.mTeamCompositionVector.size() < 2)
            return true;

        // compare all to the first one
        const TeamComposition& tc1 = (gtcInfo.mTeamCompositionVector)[0];
        if (tc1.empty())
        {
            ERR_LOG("[teamcompositionscommoninfo].areAllTeamCompositionsEqual internal error, internal team compositions possible values not yet initialized or empty for GameTeamComposition(" << gtcInfo.toFullLogStr() << "). Unable to process team compositions.");
            return false;
        }
        for (size_t i = 1; i < gtcInfo.mTeamCompositionVector.size(); ++i)
        {
            const TeamComposition& tci = (gtcInfo.mTeamCompositionVector)[i];
            if (tci.empty())
            {
                ERR_LOG("[teamcompositionscommoninfo].areAllTeamCompositionsEqual internal error, internal team compositions possible values not yet initialized or empty for GameTeamComposition(" << gtcInfo.toFullLogStr() << "). Unable to process team compositions.");
                return false;
            }
            if (!areTeamCompositionsEqual(tc1, tci))
                return false;
        }
        return true;
    }

} // namespace Matchmaker

    /////////// GroupSizeCountMap Helpers

    inline void incrementGroupSizeCountInMap(uint16_t groupSize, GroupSizeCountMap& groupSizeCountMap, uint16_t increment)
    {
        GroupSizeCountMap::iterator itr = groupSizeCountMap.find(groupSize);
        if (itr != groupSizeCountMap.end())
            itr->second += increment;
        else
            groupSizeCountMap[groupSize] = increment;
        SPAM_LOG("[teamcompositionscommoninfo].incrementGroupSizeCountInMap Incremented count of group size(" << groupSize << ") from pre-existing count " << ((groupSizeCountMap[groupSize] > increment)? groupSizeCountMap[groupSize] - increment : 0) << " to " << groupSizeCountMap[groupSize] << ".");
    }
    inline uint16_t findGroupSizeCountInMap(uint16_t groupSize, const GroupSizeCountMap& groupSizeCountMap)
    {
        GroupSizeCountMap::const_iterator itr = groupSizeCountMap.find(groupSize);
        return ((itr != groupSizeCountMap.end())? itr->second : 0);
    }
    inline const char8_t* groupSizeCountMapToLogStr(const GroupSizeCountMap& groupSizeCountMap, eastl::string& buf)
    {
        for (GroupSizeCountMap::const_iterator itr = groupSizeCountMap.begin(); itr != groupSizeCountMap.end(); ++itr)
            buf.append_sprintf("{size=%u,count=%u}", itr->first, itr->second);
        return (groupSizeCountMap.empty()? buf.append("<None>").c_str() : buf.c_str());
    }

    inline bool hasGroupSizeInGroupSizeCountMaps(uint16_t groupSize, const GroupSizeCountMapByTeamList& groupSizeCountMapByTeamList) 
    {
        for (size_t tIndex = 0; tIndex < groupSizeCountMapByTeamList.size(); ++tIndex)
        {
            if (groupSizeCountMapByTeamList[tIndex]->find(groupSize) != groupSizeCountMapByTeamList[tIndex]->end())
                return true;
        }
        return false;
    }

    inline const char8_t* groupSizeCountMapByTeamListToLogStr(const GroupSizeCountMapByTeamList& groupSizeCountMapByTeamList, eastl::string& buf)
    {
        buf.append_sprintf("totalGroupsOverall=%u, totalTeams=%u, groupSizeCounts: ", calcNumOfGroupsInGroupSizeCountMaps(groupSizeCountMapByTeamList), groupSizeCountMapByTeamList.size());
        for (size_t i = 0; i < groupSizeCountMapByTeamList.size(); ++i)
        {
            buf.append_sprintf("Team%" PRIsize "", i);
            groupSizeCountMapToLogStr(*groupSizeCountMapByTeamList[i], buf);
            if (i+1 < groupSizeCountMapByTeamList.size())
                buf.append(" vs ");
        }
        return buf.c_str();
    }

} // namespace GameManager
} // namespace Blaze

#endif //BLAZE_GAMEMANAGER_TEAM_COMPOSITIONS_COMMON_INFO_H
