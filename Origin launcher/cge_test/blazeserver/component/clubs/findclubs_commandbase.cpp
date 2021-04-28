/*************************************************************************************************/
/*!
    \file   findclubs_commandbase.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"

// clubs includes
#include "findclubs_commandbase.h"

namespace Blaze
{
namespace Clubs
{

FindClubsCommandBase::FindClubsCommandBase(ClubsSlaveImpl* componentImpl, const char8_t* cmdName)
    : ClubsCommandBase(componentImpl),
      mCmdName(cmdName)
{
}

BlazeRpcError FindClubsCommandBase::findCachedClubs(
    const FindClubsRequest& request,
    const MemberOnlineStatusList* memberOnlineStatusFilter,
    const uint32_t memberOnlineStatusSum,
    ClubList& resultList)
{
    uint32_t totalCount = 0;
    const ClubsSlaveImpl::ClubDataCache& clubDataCache = mComponent->getClubDataCache();

    for (ClubsSlaveImpl::ClubDataCache::const_iterator dataItor = clubDataCache.begin(); dataItor != clubDataCache.end(); ++dataItor)
    {
        // filter by club id
        if (!request.getClubFilterList().empty())
        {
            ClubIdList::const_iterator idItor = request.getClubFilterList().begin();
            for(; idItor != request.getClubFilterList().end(); idItor++)
            {
                if (*idItor == dataItor->first)
                    break;
            }
            if (idItor == request.getClubFilterList().end())
                continue;
        }

        const CachedClubData* clubData = dataItor->second;
        const char8_t* clubName = clubData->getName();
        TRACE_LOG("[" << mCmdName << "].findCachedClubs: Looking at cached club '" << clubName << "'");

        // filter by domain id
        if (!request.getAnyDomain())
        {
            if (request.getClubDomainId() != clubData->getClubDomainId())
                continue;
        }

        // filter by club name
        if (*(request.getName()) != '\0')
        {
            const char8_t* match = blaze_strnistr(clubName, request.getName(),strlen(request.getName()));
            if (match == nullptr)
                continue;
        }

        //filter by club language
        if (*(request.getLanguage()) != '\0')
        {
            if (strcmp(request.getLanguage(), clubData->getClubSettings().getLanguage()) != 0)
                continue;
        }

        // filter by club team id
        if (!request.getAnyTeamId())
        {
            if (request.getTeamId() != clubData->getClubSettings().getTeamId())
                continue;
        }

        //filter by club nonuniquename
        if (*(request.getNonUniqueName()) != '\0')
        {
            if (strcmp(request.getNonUniqueName(), clubData->getClubSettings().getNonUniqueName()) != 0)
                continue;
        }

        //filter by acceptance flags
        if (request.getAcceptanceMask().getBits() != 0)
        {
            if ((clubData->getClubSettings().getAcceptanceFlags().getBits() ^ request.getAcceptanceFlags().getBits()) & request.getAcceptanceMask().getBits())
                continue;
        }

        //filter by season level
        if (request.getSeasonLevel() != 0)
        {
            if (request.getSeasonLevel() != clubData->getClubSettings().getSeasonLevel())
                continue;
        }

        //filter by club region
        if (request.getRegion() != 0)
        {
            if (request.getRegion() != clubData->getClubSettings().getRegion())
                continue;
        }

        //filter by club password
        if (request.getPasswordOption() != CLUB_PASSWORD_IGNORE)
        {
            if (request.getPasswordOption() == CLUB_PASSWORD_NOT_PROTECTED)
            {
                if (clubData->getClubSettings().getPassword()[0] != '\0')
                    continue;
            }
            else if (request.getPasswordOption() == CLUB_PASSWORD_PROTECTED)
            {
                if (clubData->getClubSettings().getPassword()[0] == '\0')
                    continue;
            }
        }

        //filter by join acceptance
        if (request.getJoinAcceptance() != CLUB_ACCEPTS_UNSPECIFIED)
        {
            if (request.getJoinAcceptance() == CLUB_ACCEPTS_NONE)
            {
                if(clubData->getClubSettings().getJoinAcceptance() != CLUB_ACCEPT_NONE)
                    continue;
            }
            else if(request.getJoinAcceptance() == CLUB_ACCEPTS_ALL)
            {
                if(clubData->getClubSettings().getJoinAcceptance() != CLUB_ACCEPT_ALL)
                    continue;
            }
        }

        //filter by petition acceptance
        if (request.getPetitionAcceptance() != CLUB_ACCEPTS_UNSPECIFIED)
        {
            if (request.getPetitionAcceptance() == CLUB_ACCEPTS_NONE)
            {
                if(clubData->getClubSettings().getPetitionAcceptance() != CLUB_ACCEPT_NONE)
                    continue;
            }
            else if(request.getPetitionAcceptance() == CLUB_ACCEPTS_ALL)
            {
                if(clubData->getClubSettings().getPetitionAcceptance() != CLUB_ACCEPT_ALL)
                    continue;
            }
        }

        bool isContinue = true;
        MemberOnlineStatusCountsMap onlineStatusCounts;
        mComponent->getClubOnlineStatusCounts(dataItor->first, onlineStatusCounts);

        if (memberOnlineStatusFilter == nullptr)
        {
            //filter by member online status counts, condition is "AND" between all the status in minMemberOnlineStatusCounts

            MemberOnlineStatusCountsMap::const_iterator requestStatusIt = request.getMinMemberOnlineStatusCounts().begin();
            for (; requestStatusIt != request.getMinMemberOnlineStatusCounts().end(); ++requestStatusIt)
            {
                if ((*requestStatusIt).second > 0)
                {
                    if (onlineStatusCounts[requestStatusIt->first] < requestStatusIt->second)
                        break;
                }
            }
            if (requestStatusIt == request.getMinMemberOnlineStatusCounts().end())
                isContinue = false;
        }
        else
        {
            //filter by member online status sum
            uint32_t statusSum = 0;
            for (MemberOnlineStatusList::const_iterator requestStatusIt = memberOnlineStatusFilter->begin(); requestStatusIt != memberOnlineStatusFilter->end(); ++requestStatusIt)
            {
                statusSum += onlineStatusCounts[*requestStatusIt];
            }
            if (statusSum >= memberOnlineStatusSum)
                isContinue = false;
        }
        if (isContinue)
            continue;

        // limit results
        ++totalCount;
        if (totalCount <= request.getMaxResultCount())
        {
            Club *club = resultList.pull_back();
            club->setClubId(dataItor->first);
            club->setClubDomainId(clubData->getClubDomainId());
            club->setName(clubName);
            clubData->getClubSettings().copyInto(club->getClubSettings());
            mComponent->getClubOnlineStatusCounts(club->getClubId(), club->getClubInfo().getMemberOnlineStatusCounts());

            // jump out the top loop
            if (totalCount == request.getMaxResultCount())
                break;
        }
    }

    return Blaze::ERR_OK;
}

BlazeRpcError FindClubsCommandBase::findClubsUtil(
    const FindClubsRequest& request,
    const MemberOnlineStatusList* memberOnlineStatusFilter,
    const uint32_t memberOnlineStatusSum,
    ClubList& resultList,
    uint32_t& totalCount,
    uint32_t* sequenceID,
    uint32_t seqNum)
{
    if (sequenceID == nullptr)
    {
        if (request.getMaxResultCount() > OC_MAX_ITEMS_PER_FETCH)
            return CLUBS_ERR_TOO_MANY_ITEMS_PER_FETCH_REQUESTED;
    }
    if (request.getMaxResultCount() == 0)
        return ERR_OK;

    BlazeRpcError result = Blaze::ERR_OK;
    TimeValue start = TimeValue::getTimeOfDay();
    bool findCached = false;

    if (memberOnlineStatusFilter == nullptr)
    {
        // check if client passed up minMemberOnlineStatusCounts in the search requirements
        // and make sure client actually included valid values in the map
        for(MemberOnlineStatusCountsMap::const_iterator itr = request.getMinMemberOnlineStatusCounts().begin(); itr != request.getMinMemberOnlineStatusCounts().end(); ++itr)
        {
            if ((*itr).second > 0)
            {
                findCached = true;
                break;
            }
        }
    }
    else
    {
        // check if client passed up memberOnlineStatusFilter and memberOnlineStatusSum in the search requirements
        if (!memberOnlineStatusFilter->empty() && memberOnlineStatusSum > 0)
        {
            findCached = true;
        }
    }

    if (findCached)
    {
        result = FindClubsCommandBase::findCachedClubs(
            request,
            memberOnlineStatusFilter,
            memberOnlineStatusSum,
            resultList);
        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[" << mCmdName << "] Find cached clubs error (" << result << ")");
        }
    }
    else
    {
        if (request.getTagSearchOperation() != CLUB_TAG_SEARCH_IGNORE)
        {
            result = mComponent->checkClubTags(request.getTagList());
            if (result != Blaze::ERR_OK)
                return result;
        }

        ClubsDbConnector dbc(mComponent);
        if (dbc.acquireDbConnection(true) == nullptr)
            return ERR_SYSTEM;

        result = dbc.getDb().findClubs(
            request.getAnyDomain(),
            request.getClubDomainId(),
            request.getName(), 
            request.getLanguage(), 
            request.getAnyTeamId(), 
            request.getTeamId(), 
            request.getNonUniqueName(), 
            request.getAcceptanceMask(),
            request.getAcceptanceFlags(), 
            request.getSeasonLevel(),
            request.getRegion(),
            request.getMinMemberCount(), 
            request.getMaxMemberCount(), 
            request.getClubFilterList(),
            request.getMemberFilterList(),
            0,
            nullptr,
            request.getLastGameTimeOffset() > 0 ? (uint32_t)(start.getSec() - request.getLastGameTimeOffset()) : 0,
            request.getMaxResultCount(),
            request.getOffset(), 
            request.getSkipMetadata() != 0,
            request.getTagSearchOperation(),
            request.getTagList(),
            request.getPasswordOption(),
            request.getPetitionAcceptance(),
            request.getJoinAcceptance(),
            resultList,
            request.getSkipCalcDbRows() ? nullptr : &totalCount,
            request.getClubsOrder(),
            request.getOrderMode());

        if (result == Blaze::ERR_OK && request.getIncludeClubTags())
        {
            result = dbc.getDb().getTagsForClubs(resultList);
        }
        dbc.releaseConnection();

        if (result != Blaze::ERR_OK)
        {
            ERR_LOG("[" << mCmdName << "] Database error (" << result << ")");
        }
    }

    if (sequenceID == nullptr)
    {
        uint64_t duration = (TimeValue::getTimeOfDay() - start).getMillis();
        if (duration > mComponent->mPerfFindClubsTimeMsHighWatermark.get())
            mComponent->mPerfFindClubsTimeMsHighWatermark.set(duration);
    }

    if (result == Blaze::ERR_OK)
    {
        result = mComponent->filterClubPasswords(resultList);
    }

    if (result == Blaze::ERR_OK)
    {
        if (sequenceID == nullptr)
        {
            if (!findCached)
            {
                for (ClubList::iterator it = resultList.begin(); it != resultList.end(); ++it)
                {
                    mComponent->getClubOnlineStatusCounts((*it)->getClubId(), (*it)->getClubInfo().getMemberOnlineStatusCounts());
                }
            }
        }
        else
        {
            *sequenceID = mComponent->getNextAsyncSequenceId();
            for (ClubList::iterator it = resultList.begin(); it != resultList.end(); ++it)
            {
                if (!findCached)
                {
                    mComponent->getClubOnlineStatusCounts((*it)->getClubId(), (*it)->getClubInfo().getMemberOnlineStatusCounts());
                }
                // fire the results
                FindClubsAsyncResult resultNotif;
                resultNotif.setSequenceID(*sequenceID);
                (*it)->copyInto(resultNotif.getClub());
                mComponent->sendFindClubsAsyncNotificationToUserSession(gCurrentLocalUserSession, &resultNotif, false, seqNum);
            }
        }
    }

    return result;
}

} // Clubs
} // Blaze
