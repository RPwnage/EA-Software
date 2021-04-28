/*! ************************************************************************************************/
/*!
    \file   creategamefinalizationteamcompositionsinfo.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationteamcompositionsinfo.h"
#include "gamemanager/matchmaker/creategamefinalization/creategamefinalizationsettings.h"

namespace Blaze
{

namespace GameManager
{

namespace Matchmaker
{

    TeamCompositionFilledInfo::TeamCompositionFilledInfo()
        : mParent(nullptr), mTeamComposition(nullptr)
    {}

    void TeamCompositionFilledInfo::initialize(const CreateGameFinalizationGameTeamCompositionsInfo& p,
        const TeamComposition& composition)
    {
        mParent = &p;
        mTeamComposition = &composition;
        mGroupSizeIsFilledPairVector.clear();
        mGroupSizeIsFilledPairVector.reserve(composition.size());
        for (size_t i = 0; i < composition.size(); ++i)
            mGroupSizeIsFilledPairVector.push_back(eastl::make_pair(composition[i], false));
    }

    // return next group size to fill, or UINT16_MAX if none unfilled
    uint16_t TeamCompositionFilledInfo::getNextUnfilledGroupSize() const
    {
        // Note: Right now given number of items in a team composition probably won't exceed 5 or 6 these scans, however
        // if in future they're larger, for performance, we may want to index the vector with a 'mNextUnfilledGroupSize' instead (assuming we always fill the parts in conseq order)
        for (size_t i = 0; i < mGroupSizeIsFilledPairVector.size(); ++i)
        {
            if (mGroupSizeIsFilledPairVector[i].second == false)
                return mGroupSizeIsFilledPairVector[i].first;
        }
        return UINT16_MAX;
    }

    bool TeamCompositionFilledInfo::hasUnfilledGroupSize(uint16_t groupSize) const
    {
        for (size_t i = 0; i < mGroupSizeIsFilledPairVector.size(); ++i)
        {
            if ((mGroupSizeIsFilledPairVector[i].first == groupSize) && (mGroupSizeIsFilledPairVector[i].second == false))
            {
                return true;
            }
        }
        TRACE_LOG("[TeamCompositionTriedInfo].hasUnfilledGroupSize already filled the team composition " << toLogStr(true) << ".");
        return false;
    }

    bool TeamCompositionFilledInfo::markGroupSizeFilled(uint16_t groupSizeToMarkFilled)
    {
        for (size_t i = 0; i < mGroupSizeIsFilledPairVector.size(); ++i)
        {
            if ((mGroupSizeIsFilledPairVector[i].first == groupSizeToMarkFilled) && (mGroupSizeIsFilledPairVector[i].second == false))
            {
                mGroupSizeIsFilledPairVector[i].second = true;
                TRACE_LOG("[TeamCompositionTriedInfo].markGroupSizeFilled marked a group of size " << groupSizeToMarkFilled 
                    << " filled in this composition " << toLogStr(true) << ". Composition is " << (isFull()? "full." : "not yet full."));
                return true;
            }
        }
        TRACE_LOG("[TeamCompositionTriedInfo].markGroupSizeFilled specified group of size " << groupSizeToMarkFilled 
            << " to mark filled is not present or was already filled in this composition " << toLogStr(true) << ". No op.");
        return false;
    }

    void TeamCompositionFilledInfo::clearGroupSizeFilled(uint16_t groupSizeToMarkFilled)
    {
        for (size_t i = 0; i < mGroupSizeIsFilledPairVector.size(); ++i)
        {
            if ((mGroupSizeIsFilledPairVector[i].first == groupSizeToMarkFilled) && (mGroupSizeIsFilledPairVector[i].second == true))
            {
                mGroupSizeIsFilledPairVector[i].second = false;
                TRACE_LOG("[TeamCompositionTriedInfo].clearGroupSizeFilled un-marked a group of size " << groupSizeToMarkFilled 
                    << " from being filled in this composition " << toLogStr(true) << ".");
                return;
            }
        }
        ERR_LOG("[TeamCompositionTriedInfo].clearGroupSizeFilled specified group of size " << groupSizeToMarkFilled 
            << " to unmark filled is not present or was unfilled in this composition " << toLogStr(true) << ". No op.");
    }

    // logging helper
    const char8_t* TeamCompositionFilledInfo::toLogStr(bool doPrintFilledKey /*= false*/) const
    {
        mLogStrBuf.clear();
        mLogStrBuf.append_sprintf("%s{", (doPrintFilledKey? "(key:x=filled,o=unfilled)":""));
        for (size_t i = 0; i < mGroupSizeIsFilledPairVector.size(); ++i)
        {
            mLogStrBuf.append_sprintf("%u%s", mGroupSizeIsFilledPairVector[i].first, (mGroupSizeIsFilledPairVector[i].second? "x":"o"));
            if (i+1 < mGroupSizeIsFilledPairVector.size())
                mLogStrBuf.append(",");
        }
        return mLogStrBuf.append("}").c_str();
    }


    CreateGameFinalizationGameTeamCompositionsInfo::CreateGameFinalizationGameTeamCompositionsInfo()
        : mCurrentGameTeamCompositionsInfo(nullptr)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief Set up the finalization for attempt at another game team compositions set.

        \param[in] gameTeamCompositionsInfo The Game Team Compositions tuple to attempt.
        \param[in,out] createGameFinalizationTeamInfoVector assigns TeamCompositions to teams in this vector.

        \return whether succeeds assigning out teams to compositions to attempt.
    *************************************************************************************************/
    bool CreateGameFinalizationGameTeamCompositionsInfo::initialize(const CreateGameFinalizationSettings& p,
        const GameTeamCompositionsInfo& gameTeamCompositionsInfo, const MatchmakingSession& pullingSession,
        CreateGameFinalizationTeamInfoVector& createGameFinalizationTeamInfoVector)
    {        
        if (!validateStatePreInitialize(p, gameTeamCompositionsInfo, pullingSession, createGameFinalizationTeamInfoVector))
            return false;

        // reset all old results
        reset(createGameFinalizationTeamInfoVector);
        
        if (mOwningSessionLogStr.empty())
            mOwningSessionLogStr.sprintf("(sessId=%" PRIu64 ",groupSize=%u)", pullingSession.getMMSessionId(), pullingSession.getPlayerCount());
        mCurrentGameTeamCompositionsInfo = &gameTeamCompositionsInfo;
        const size_t teamCount = createGameFinalizationTeamInfoVector.size();
        const size_t teamCompositionCount = gameTeamCompositionsInfo.mTeamCompositionVector.size();
       
        // set up team composition tried infos and assign teams to them
        for (TeamIndex teamIndex = 0; teamIndex < teamCount; ++teamIndex)
        {
            // Get the team composition to fill. Note: if there's more (see configuration settings) teams than there are TeamCompositions, the left over get random assignments
            if (teamIndex >= teamCompositionCount)
            {
                ERR_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].initialize possible internal error: the attempted GameTeamCompositions for finalization had fewer teams(" 
                    << teamCompositionCount << ") than finalization setting's number of teams(" << teamCount << "). The team(index=" << teamIndex 
                    << ") cannot be assigned to a composition. Cannot initialize with " << gameTeamCompositionsInfo.toFullLogStr() << ".");
                return false;
            }

            // add new team composition filled info
            mTeamCompositionFilledInfoList.resize(mTeamCompositionFilledInfoList.size() + 1);//note: nodes non-POD, avoid push_back_uninitialized
            TeamCompositionFilledInfo& nextTcFilledInfo = mTeamCompositionFilledInfoList.back();

            nextTcFilledInfo.initialize(*this, gameTeamCompositionsInfo.mTeamCompositionVector[teamIndex]);
            createGameFinalizationTeamInfoVector[teamIndex].setTeamCompositionToFill(&nextTcFilledInfo);
        }

        // mark pre-existing member's spot in its team as filled. Ensure its on the right team and can fit
        for (size_t i = 0; i < p.getTotalSessionsCount(); ++i)
        {
            const MatchmakingSession* session = p.getMatchingSessionList()[i];
            const TeamIdSizeList* teamIdSizeList = session->getCriteria().getTeamIdSizeListFromRule();
            if (teamIdSizeList == nullptr)
            {
                ERR_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].initialize internal error: session(id=" << session->getMMSessionId() 
                    << ") missing team join information.");
                return false;
            }

            TeamIdSizeList::const_iterator curTeamId = teamIdSizeList->begin();
            TeamIdSizeList::const_iterator endTeamId = teamIdSizeList->end();
            for (; curTeamId != endTeamId; ++curTeamId)
            {
                TeamId teamId = curTeamId->first;
                uint16_t playerCount = curTeamId->second;
                TeamIndex teamIndex = p.getTeamIndexForMMSession(*session, teamId);
                if (!createGameFinalizationTeamInfoVector[teamIndex].markTeamCompositionToFillGroupSizeFilled(playerCount))
                {
                    // If here, the pulling session might be already on the wrong team. (Only the game team compositions permutation
                    // which mirrors this one, but for which the pulling session is on the correct team, should be attempted).
                    eastl::string buf;
                    TRACE_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].initialize cannot attempt this game team compositions " << gameTeamCompositionsInfo.toCompositionsLogStr() 
                        << ", as the already-added session " << session->getMMSessionId() << ", was assigned to teamIndex " << teamIndex << " which cannot be assigned to required team composition " 
                        <<  teamCompositionToLogStr(gameTeamCompositionsInfo.mTeamCompositionVector[teamIndex], buf) << " (because of incompatibility with the session's team group size " << playerCount << ").");
                    return false;
                }
            }
        }

        TRACE_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].initialize initialized game team compositions finalization info " << toLogStr() << ".");
        return true;
    }

    void CreateGameFinalizationGameTeamCompositionsInfo::reset(CreateGameFinalizationTeamInfoVector& createGameFinalizationTeamInfoVector)
    {
        mTeamCompositionFilledInfoList.clear();
        for (TeamIndex teamIndex = 0; teamIndex < createGameFinalizationTeamInfoVector.size(); ++teamIndex)
        {
            createGameFinalizationTeamInfoVector[teamIndex].setTeamCompositionToFill(nullptr);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return whether we are able to do the new initialize call
    *************************************************************************************************/
    bool CreateGameFinalizationGameTeamCompositionsInfo::validateStatePreInitialize(const CreateGameFinalizationSettings& p,
        const GameTeamCompositionsInfo& gameTeamCompositionsInfo, const MatchmakingSession& pullingSession,
        CreateGameFinalizationTeamInfoVector& createGameFinalizationTeamInfoVector) const
    {
        if (!pullingSession.getCriteria().hasTeamCompositionRuleEnabled())
        {
            WARN_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].validateStatePreInitialize possible internal error: pulling session(" << pullingSession.getMMSessionId() 
                << ") did not have team compositions enabled, but we're attempting to initialize team compositions finalization settings.");
            return false;
        }

        // validate the teams can actually fit the attempting Game Team Compositions
        const size_t teamCount = createGameFinalizationTeamInfoVector.size();
        const size_t teamCompositionCount = gameTeamCompositionsInfo.mTeamCompositionVector.size();
        if (teamCompositionCount > teamCount)
        { 
            WARN_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].validateStatePreInitialize possible internal error: the attempted game team compositions for finalization had more teams(" 
                << teamCompositionCount << ") than finalization setting's actual number of teams(" << teamCount << "). Game team compositions: " << gameTeamCompositionsInfo.toFullLogStr() << ".");
            return false;
        }
        // Validate pre-existing members and group sizes.
        if (pullingSession.getCriteria().hasTeamCompositionRuleEnabled())
        {
            const int32_t expectedMemberCount = 1;
            if (p.getTotalSessionsCount() - expectedMemberCount >= 1)
            {
                ERR_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].validateStatePreInitialize internal error: unexpected excess number of members already assigned"
                    " to teams before team compositions assigned to teams. Expected only " << expectedMemberCount << ", but there were " << p.getTotalSessionsCount() << ".");
                return false;
            }
        }
        // validate all sessions in the finalization already have valid teams
        bool foundPuller = false;
        for (size_t i = 0; i < p.getTotalSessionsCount(); ++i)
        {
            const MatchmakingSession* session = p.getMatchingSessionList()[i];
            const TeamIdSizeList* teamIdSizeList = session->getCriteria().getTeamIdSizeListFromRule();
            if (teamIdSizeList == nullptr)
            {
                ERR_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].validateStatePreInitialize internal error: session(id=" << session->getMMSessionId() 
                    << ") missing team join information.");
                return false;
            }

            TeamIdSizeList::const_iterator curTeamId = teamIdSizeList->begin();
            TeamIdSizeList::const_iterator endTeamId = teamIdSizeList->end();
            for (; curTeamId != endTeamId; ++curTeamId)
            {
                TeamId teamId = curTeamId->first;
                TeamIndex teamIndex = p.getTeamIndexForMMSession(*session, teamId);
                if ((teamIndex == UNSPECIFIED_TEAM_INDEX) || (teamIndex >= teamCount))
                {
                    ERR_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].validateStatePreInitialize internal error: session(id=" << session->getMMSessionId() 
                        << ") was not assigned to a valid team index in the finalization. Its current teamIndex is " << teamIndex << ", the max teamIndex is " << (teamCount - 1) << ".");
                    return false;
                }
            }
            foundPuller = (foundPuller || (session->getMMSessionId() == pullingSession.getMMSessionId()));
        }
        // validate pulling session is in the finalization
        if (!foundPuller)
        {
            ERR_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].validateStatePreInitialize internal error:"
                " pulling session was not assigned to a valid team in the finalization yet. Expected pulling session to be in the finalization already.");
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief return true if all the game's team compositions have been full-filled (in which case sessions may finalize)
    *************************************************************************************************/
    bool CreateGameFinalizationGameTeamCompositionsInfo::isFullfilled() const
    {
        for (TeamCompositionTriedInfoList::const_iterator itr = mTeamCompositionFilledInfoList.begin(); itr != mTeamCompositionFilledInfoList.end(); ++itr)
        {
            if (!itr->isFull())
            {
                TRACE_LOG("[CreateGameFinalizationGameTeamCompositionsInfo].isFullfilled: NO_MATCH. Found an unfilled group size '" 
                    << itr->getNextUnfilledGroupSize() << "' in team composition '" << itr->toLogStr(true) << "'. Current game team compositions: " << toLogStr() << ".");
                return false;
            }
        }
        return true;
    }

    const char8_t* CreateGameFinalizationGameTeamCompositionsInfo::toLogStr() const
    { 
        mLogStr.clear();
        mLogStr.append(mOwningSessionLogStr);
        if (mCurrentGameTeamCompositionsInfo == nullptr)
        {
            return mLogStr.append("[No explicit GameTeamCompositions specified]").c_str();
        }
        mLogStr.append_sprintf("[Id=%" PRIi64 ", comps=", getCurrentGameTeamCompositionsId());
        for (TeamCompositionTriedInfoList::const_iterator itr = mTeamCompositionFilledInfoList.begin(); itr != mTeamCompositionFilledInfoList.end(); ++itr)
        {
            const bool isFirst = (itr == mTeamCompositionFilledInfoList.begin());
            mLogStr.append_sprintf("%s%s", (isFirst? "":" vs "), itr->toLogStr(isFirst));
        }
        return mLogStr.append("]").c_str();
    }
    
} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
