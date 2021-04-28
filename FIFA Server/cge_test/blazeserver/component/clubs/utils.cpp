/*************************************************************************************************/
/*!
    \file   util.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// framework includes
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbutil.h"
#include "framework/database/query.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/usersessions/userinfo.h"
#include "framework/usersessions/usersession.h"
#include "framework/util/profanityfilter.h"
#include "framework/util/localization.h"

// clubs includes
#include "clubs/clubsslaveimpl.h"
#include "clubs/clubsdbconnector.h"

namespace Blaze
{
namespace Clubs
{

BlazeRpcError ClubsSlaveImpl::checkClubStrings(
    const char8_t *clubName, 
    const char8_t *clubNonUniqueName,
    const char8_t *clubDescription,
    const char8_t *clubPassword) const
{
    if (clubName != nullptr)
    {
        size_t nameLen = strlen(clubName);
        if (nameLen == 0)
        {
            return CLUBS_ERR_INVALID_CLUBNAME_EMPTY;
        }

        if (nameLen < OC_MIN_NAME_LENGTH || nameLen > OC_MAX_NAME_LENGTH)
        {
            return CLUBS_ERR_INVALID_CLUBNAME_SIZE;
        }

        // Check club name for profanity
        if (gProfanityFilter->isProfane(clubName))
        {
            TRACE_LOG("[checkClubStrings] club name contains profanity: " << clubName);
            return CLUBS_ERR_PROFANITY_FILTER;
        }
        
    }
    
    if (clubNonUniqueName != nullptr)
    {
        size_t nonUniqueNameLen = strlen(clubNonUniqueName);
        if (nonUniqueNameLen == 0)
        {
            return CLUBS_ERR_INVALID_NON_UNIQUE_NAME_EMPTY;
        }

        if (nonUniqueNameLen < OC_MIN_NON_UNIQUE_NAME_LENGTH || nonUniqueNameLen > OC_MAX_NON_UNIQUE_NAME_LENGTH)
        {
            return CLUBS_ERR_INVALID_NON_UNIQUE_NAME_SIZE;
        }

        // Check club non unique name for profanity
        if (gProfanityFilter->isProfane(clubNonUniqueName))
        {
            TRACE_LOG("[checkClubStrings] club non-unique name contains profanity: " << clubNonUniqueName);
            return CLUBS_ERR_PROFANITY_FILTER;
        }
    }
    
    if (clubDescription != nullptr)
    {
        // Check club description for profanity
        if (gProfanityFilter->isProfane(clubDescription))
        {
            TRACE_LOG("[checkClubStrings] club description contains profanity: " << clubDescription);
            return CLUBS_ERR_PROFANITY_FILTER;
        }
    }
    
    if (clubPassword != nullptr)
    {
        // Check club password for profanity
        if (gProfanityFilter->isProfane(clubPassword))
        {
            TRACE_LOG("[checkClubStrings] club password contains profanity: " << clubPassword);
            return CLUBS_ERR_INVALID_PASSWORD_PROFANITY;
        }
    }
  
    return ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::checkClubTags(const ClubTagList& tagList) const
{
    for (ClubTagList::const_iterator it = tagList.begin(); it != tagList.end(); it++)
    {
        const char8_t* tagText = (*it).c_str();
        if (*tagText == '\0')
            return CLUBS_ERR_INVALID_TAG_TEXT_EMPTY;
        else if (strlen(tagText) > OC_MAX_TAG_TEXT_LENGTH)
            return CLUBS_ERR_INVALID_TAG_TEXT_SIZE;
    }
    
    return ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::checkClubMetadata(const ClubSettings& clubSettings) const
{
	if (clubSettings.getMetaDataUnion().getActiveMember() == MetadataUnion::MEMBER_METADATASTRING)
	{
		char metadata[MAX_METADATASTRING_LEN];
		blaze_strnzcpy(metadata, clubSettings.getMetaDataUnion().getMetadataString(), sizeof(metadata));

		char8_t* stadiumNameSavePtr = nullptr;
		char8_t* stadiumName = blaze_strtok(metadata, ">", &stadiumNameSavePtr);
		while (stadiumName && blaze_strstr(stadiumName, "StadName") == nullptr)
		{
			stadiumName = blaze_strtok(nullptr, ">", &stadiumNameSavePtr);
		}

		if(stadiumName)
		{
			if(gProfanityFilter->isProfane(stadiumName) != 0)
			{
				ERR("[checkClubMetadata:%p] club metadata contains profanity: %s", this, clubSettings.getMetaDataUnion().getMetadataString());
				return CLUBS_ERR_PROFANITY_FILTER;
			}
		} 
	}

	return ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::filterClubPasswords(ClubList& resultList)
{
    if (!resultList.empty())
    {
        // only authorized users can access club password
        BlazeId requestorId = INVALID_BLAZE_ID;
        if (gCurrentUserSession != nullptr)
        {
            requestorId = gCurrentUserSession->getBlazeId();
        }

        if (UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true) == false)
        {
            
            BlazeRpcError error = Blaze::ERR_OK;

            ClubsDbConnector dbc(this); 
            if (dbc.acquireDbConnection(true) == nullptr)
                return ERR_SYSTEM;
                
            // not club admin, then check user role and club domain
            for (ClubList::iterator it = resultList.begin(); it != resultList.end(); it++)
            {
                if ((*it)->getClubSettings().getPassword()[0] != '\0')
                {
                    (*it)->getClubSettings().setHasPassword(true);

                    MembershipStatus mshipStatus = CLUBS_MEMBER;
                    MemberOnlineStatus onlStatus;
                    TimeValue memberSince;
                    error = getMemberStatusInClub(
                        (*it)->getClubId(), 
                        requestorId, 
                        dbc.getDb(), 
                        mshipStatus, 
                        onlStatus,
                        memberSince);

                    if (error != Blaze::ERR_OK && error != CLUBS_ERR_USER_NOT_MEMBER)
                    {
                        dbc.releaseConnection();
                        return error;
                    }
                    
                    // if user is not member or below GM(domain doesn't allow member to retrieve password), hide club password.
                    if (error == CLUBS_ERR_USER_NOT_MEMBER)
                    {
                        (*it)->getClubSettings().setPassword("");
                    }
                    else if (mshipStatus < CLUBS_GM)
                    {
                        const ClubDomain *domain = nullptr;
                        error = getClubDomainForClub((*it)->getClubId(), dbc.getDb(), domain);
                        if (error != Blaze::ERR_OK)
                        {
                            dbc.releaseConnection();
                            return error;
                        }
                        if (!domain->getAllowMemberToRetrievePassword())
                        {
                            (*it)->getClubSettings().setPassword("");
                        }
                    }
                }
                else
                {
                    (*it)->getClubSettings().setHasPassword(false);
                }
            }

            dbc.releaseConnection();
        }
        else
        {
            for (ClubList::iterator it = resultList.begin(); it != resultList.end(); it++)
            {
                if ((*it)->getClubSettings().getPassword()[0] != '\0')
                {
                    (*it)->getClubSettings().setHasPassword(true);
                }
                else
                {
                    (*it)->getClubSettings().setHasPassword(false);
                }
            }
        }
    }

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::insertNewsAndDeleteOldest(ClubsDatabase& clubsDb, ClubId clubId, BlazeId contentCreator, const ClubNews &news)
{
    BlazeRpcError result = Blaze::ERR_OK;

    const ClubDomain *domain = nullptr;

    result = getClubDomainForClub(clubId, clubsDb, domain);
    if (result != Blaze::ERR_OK)
    {
        return result;
    }

    result = clubsDb.insertNews(clubId, contentCreator, news);
    if (result == Blaze::ERR_OK)
    {
        uint16_t newsCount;
        if (clubsDb.countNews(clubId, newsCount) == Blaze::ERR_OK)
        {
            if (newsCount > domain->getMaxNewsItemsPerClub())
            {
                result = clubsDb.deleteOldestNews(clubId);
            }
        }
        
        result = clubsDb.updateClubLastActive(clubId);
    }

    return result;
}

BlazeRpcError ClubsSlaveImpl::getNewsForClubs(
        const ClubIdList& requestorClubIdList, 
        const NewsTypeList& typeFilters,
        const TimeSortType sortType,
        const uint32_t maxResultCount,
        const uint32_t offSet,
        const uint32_t locale,
        ClubLocalizedNewsListMap& clubLocalizedNewsListMap,
        uint16_t& totalPages)
{
    if (typeFilters.empty())
        return Blaze::CLUBS_ERR_MISSING_NEWS_TYPE_FILTER;
                
    ClubsDbConnector dbc(this); 
    if (dbc.acquireDbConnection(true) == nullptr)
        return Blaze::ERR_SYSTEM;

    BlazeRpcError result = Blaze::ERR_OK;

    // club admin and all the members are allowed to get news, 
    // but only admin and GM are allowed to get hidden news.
    eastl::map<ClubId, bool> getHiddenNewsMap;
    BlazeId requestorId = INVALID_BLAZE_ID;
    if (gCurrentUserSession != nullptr)
    {
        requestorId = gCurrentUserSession->getBlazeId();
    }
    ClubIdList requestorlocalClubIdList;
    requestorClubIdList.copyInto(requestorlocalClubIdList);

    bool isAdmin = false;
    bool hasCheckIfIsAdmin = false;

    // check membership status for current user
    MembershipStatus mship = CLUBS_MEMBER;
    MemberOnlineStatus onlStatus;
    TimeValue memberSince;

    ClubIdList::iterator iter = requestorlocalClubIdList.begin();

    for (; iter != requestorlocalClubIdList.end(); )
    {
        result = getMemberStatusInClub(
            *iter,
            requestorId, 
            dbc.getDb(), 
            mship, 
            onlStatus,
            memberSince);

        if (isAdmin)
            break;
        
        if (result != Blaze::ERR_OK && result != Blaze::CLUBS_ERR_USER_NOT_MEMBER)
        {
            if (!hasCheckIfIsAdmin)
            {
                isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);

                hasCheckIfIsAdmin = true;
            }
            
            if (!isAdmin)
            {
                iter = requestorlocalClubIdList.erase(iter);
                
                TRACE_LOG("[getNewsForClubs] user [" << requestorId << "] does not have admin view for club ["
                    << *iter << "] and is not allowed to get this club's hidden news");

                continue;
            }
        }
        else if (mship >= CLUBS_GM)
        {
            getHiddenNewsMap[*iter] = true;
        }
        else
        {
            if (!hasCheckIfIsAdmin)
            {
                isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);

                hasCheckIfIsAdmin = true;
            }

            if (!isAdmin)
            {
                getHiddenNewsMap[*iter] = false;

                TRACE_LOG("[getNewsForClubs] user [" << requestorId << "] does not have admin view for club ["
                    << *iter << "] and is not allowed to get this club's hidden news");
            }
        }

        iter++;
    }

        
    if (isAdmin)
    {
        ClubIdList::iterator it = requestorlocalClubIdList.begin();
        ClubIdList::iterator iterEnd = requestorlocalClubIdList.end();

        for (; it != iterEnd; it++)
        {
            getHiddenNewsMap[*it] = true;
        }
    }

    if (requestorlocalClubIdList.empty())
        return result;
     
    eastl::list<ClubServerNewsPtr> clubNews;
    
    TimeValue start = TimeValue::getTimeOfDay();
    bool getAssociateNews;

    result =
        dbc.getDb().getNews(
            requestorlocalClubIdList, 
            typeFilters, 
            sortType, 
            maxResultCount, 
            offSet, 
            clubNews,
            getAssociateNews);
    
    // get the total number of news
    uint16_t newsTotalCount = 0;
    dbc.getDb().countNewsByTypeForMultipleClubs(requestorlocalClubIdList, newsTotalCount, typeFilters);

    dbc.releaseConnection();
            
    uint64_t duration = (TimeValue::getTimeOfDay() - start).getMillis();
    if (duration > mPerfGetNewsTimeMsHighWatermark.get())
        mPerfGetNewsTimeMsHighWatermark.set(duration);
    
    if (result != (BlazeRpcError)ERR_OK)
    {
        ERR_LOG("[getNewsForClubs] Database error (" << result << ")");
        return result;
    }

    ClubIdList::iterator iterLocal = requestorlocalClubIdList.begin();
    ClubIdList::iterator iterLocalEnd = requestorlocalClubIdList.end();

    for (; iterLocal != iterLocalEnd; iterLocal++)
    {
        bool isHiddenNews = false;
        ClubId clubId = *iterLocal;
        ClubLocalizedNewsList *clubLocalizedNewsList = BLAZE_NEW ClubLocalizedNewsList();
        
        // format the clubLocalizedNewsList from the db result.
        result = transferClubServerNewsToLocalizedNews(clubId, clubNews, locale, isHiddenNews, getHiddenNewsMap[clubId], *clubLocalizedNewsList, getAssociateNews);

        if (result == Blaze::ERR_OK)
        {
            clubLocalizedNewsListMap[clubId] = clubLocalizedNewsList;
        }
        
        if (result == Blaze::ERR_OK && isAdmin && isHiddenNews)
        {
            INFO_LOG("[getNewsForClubs].execute: User [" << requestorId << "] got hidden news in  the club [" 
                << clubId << "], , had permisssion!");
        }
    }

    if (newsTotalCount > 0 && maxResultCount > 0 )
    {
        if (newsTotalCount % maxResultCount != 0)
            totalPages = (uint16_t)(newsTotalCount / maxResultCount) + 1;
        else
            totalPages = (uint16_t)(newsTotalCount / maxResultCount);
    }
    
    return result;
}

BlazeRpcError ClubsSlaveImpl::transferClubServerNewsToLocalizedNews(
        const ClubId requestorClubId,
        const eastl::list<ClubServerNewsPtr>& clubServerNews,
        const uint32_t locale,
        bool& isHiddenNews,
        bool getHiddenNews,
        ClubLocalizedNewsList& clubLocalizedNewsList,
        const bool& getAssociateNews)
{
    Blaze::BlazeRpcError result = Blaze::ERR_OK;

    // localize news
    eastl::list<ClubServerNewsPtr>::const_iterator it = clubServerNews.begin();

    Blaze::BlazeIdVector blzIds;
    
    for (; it != clubServerNews.end(); it++)
    {
        if ((*it)->getContentCreator() != 0)
            blzIds.push_back((*it)->getContentCreator());
    }
    
    Blaze::BlazeIdToUserInfoMap idToInfo;
    if (gUserSessionManager->lookupUserInfoByBlazeIds(blzIds, idToInfo) != Blaze::ERR_OK)
    {
        WARN_LOG("[transferClubServerNewsToLocalizedNews] Failed to lookup users when fetching news for club " << requestorClubId);
    }
    
    it = clubServerNews.begin();
    for (; it != clubServerNews.end(); it++)
    {
        ClubServerNews serverNews;
        (*it)->copyInto(serverNews);

        if (serverNews.getType() != CLUBS_ASSOCIATE_NEWS && requestorClubId != serverNews.getClubId())
            continue;

        if ( serverNews.getType() == CLUBS_ASSOCIATE_NEWS && requestorClubId != serverNews.getClubId() && requestorClubId != serverNews.getAssociateClubId())
            continue;
               
        if (serverNews.getFlags().getCLUBS_NEWS_HIDDEN_BY_TOS_VIOLATION())
            continue;

        if (!isHiddenNews && serverNews.getFlags().getCLUBS_NEWS_HIDDEN_BY_GM())
            isHiddenNews = true;

        if (!getHiddenNews && serverNews.getFlags().getCLUBS_NEWS_HIDDEN_BY_GM())
            continue;
        
        char8_t news[OC_MAX_NEWS_ITEM_LENGTH + 1];
        
        if (*serverNews.getStringId() != '\0')
        {
            // news needs to be localized
            char8_t newsParams[OC_MAX_NEWS_PARAM_LEN];
            char8_t *params[OC_MAX_NEWS_PARAMS + 1];

            blaze_strnzcpy(newsParams, serverNews.getParamList(), OC_MAX_NEWS_PARAM_LEN);
    
            int c = 0;
            params[0] = newsParams;
            unsigned int realCount = 1;
            for (unsigned int count = 1; count <= OC_MAX_NEWS_PARAMS; count++)
            {
                params[count] = nullptr;
                while (newsParams[c] != ',' && newsParams[c] != '\0') c++;
                if (newsParams[c] == ',') 
                {
                    params[count] = newsParams + c + 1;                    
                    newsParams[c] = '\0';
                    c++;
                    realCount++;
                }
            }
            
            if (realCount == OC_MAX_NEWS_PARAMS)
                result = (BlazeRpcError) CLUBS_ERR_TOO_MANY_PARAMETERS;
            
            eastl::string str = gLocalization->localize(
                serverNews.getStringId(),
                locale);
            
            for (unsigned int i = 0; i < realCount; i++)
            {
                char8_t* param = params[i];
                char8_t personaName[OC_MAX_NEWS_PARAM_LEN];
                // check for special handling of blaze ids
                if (serverNews.getType() == CLUBS_SERVER_GENERATED_NEWS && param[0] == '@')
                {
                    // convert the parameter to a blaze id
                    BlazeId id = 0;
                    sscanf(param + 1, "%" SCNd64, &id);
                    // and lookup the user name
                    UserInfoPtr info = nullptr;
                    BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByBlazeId(id, info);
                    if (lookupErr == Blaze::ERR_OK)
                    {
                        blaze_strnzcpy(personaName, info->getPersonaName(), sizeof(personaName));
                    }
                    else
                    {
                        blaze_strnzcpy(personaName, "UNKNOWN", sizeof(personaName));
                    }
                    param = personaName;
                }

                blaze_snzprintf(news, OC_MAX_NEWS_ITEM_LENGTH, "<%d>", i);
                eastl::string::size_type n;
                do
                {
                    n = str.find(news);
                    if (n != eastl::string::npos)
                        str.replace(n, strlen(news), param);
                }
                while (n != eastl::string::npos);
            }

            blaze_strnzcpy(news, str.c_str(), OC_MAX_NEWS_ITEM_LENGTH);
        }
        else
        {
            blaze_strnzcpy(news, serverNews.getText(), OC_MAX_NEWS_ITEM_LENGTH);
        }
        
        ClubLocalizedNews *clubLocNews = clubLocalizedNewsList.pull_back();
        clubLocNews->setAssociateClubId(serverNews.getAssociateClubId());
        clubLocNews->getUser().setBlazeId(serverNews.getContentCreator());
        clubLocNews->setText(news);
        clubLocNews->setTimestamp(serverNews.getTimestamp());
        clubLocNews->setType(serverNews.getType());
        clubLocNews->getFlags().setBits(serverNews.getFlags().getBits());
        clubLocNews->setNewsId(EA::TDF::ObjectId(ENTITY_TYPE_CLUBNEWS, serverNews.getNewsId()));
        
        if (serverNews.getContentCreator() != 0)
        {
            Blaze::BlazeIdToUserInfoMap::const_iterator itInfo = idToInfo.find(serverNews.getContentCreator());
            
            if (itInfo == idToInfo.end())
            {
                WARN_LOG("[transferClubServerNewsToLocalizedNews] Failed to lookup user (" << serverNews.getContentCreator() << ")");
            }
            else
            {
                UserInfo::filloutUserCoreIdentification(*itInfo->second, clubLocNews->getUser());
            }
        }
        else
        {
            clubLocNews->getUser().setName("SYSTEM");
        }
    }

    
    return result;
}

BlazeRpcError ClubsSlaveImpl::getPetitionsForClubs(
        const ClubIdList& requestorClubIdList, 
        const PetitionsType petitionsType,
        const TimeSortType sortType,
        ClubMessageListMap& clubMessageListMap)
{
    BlazeId requestorId = INVALID_BLAZE_ID;
    if (gCurrentUserSession != nullptr)
    {
        requestorId = gCurrentUserSession->getBlazeId();
    }

    ClubsDbConnector dbc(this); 
    if (dbc.acquireDbConnection(true) == nullptr)
        return Blaze::ERR_SYSTEM;
    
    BlazeRpcError error= Blaze::ERR_OK;
    bool isAdmin = false;
    bool hasCheckIfIsAdmin = false;
    ClubIdList requestorlocalClubIdList;
    requestorClubIdList.copyInto(requestorlocalClubIdList);

    ClubMessageList clubMessageListFromDB;

    const bool localClubIdListEmpty = requestorlocalClubIdList.empty();

    switch(petitionsType)
    {

    case CLUBS_PETITION_SENT_BY_ME:
        {
            error = dbc.getDb().getClubMessagesByUserSent(requestorId, CLUBS_PETITION,
                sortType, clubMessageListFromDB);
            break;
        }
    case CLUBS_PETITION_SENT_TO_CLUB:
        {
            if (localClubIdListEmpty)
            {
                TRACE_LOG("[getPetitionsForClubs] empty club Ids not allowed on CLUBS_PETITION_SENT_TO_CLUB.");
                return CLUBS_ERR_INVALID_ARGUMENT;
            }

            // club admin and club members can get club petitions
            MembershipStatus mshipStatus;
            MemberOnlineStatus onlSatus;
            TimeValue memberSince;

            ClubIdList::iterator iter = requestorlocalClubIdList.begin();
            for (; iter != requestorlocalClubIdList.end();)
            {
                error = getMemberStatusInClub(
                    *iter, 
                    requestorId, 
                    dbc.getDb(), 
                    mshipStatus, 
                    onlSatus,
                    memberSince);

                if (isAdmin)
                    break;

                if (error != Blaze::ERR_OK)
                {
                    if (!hasCheckIfIsAdmin)
                    {
                        isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);
                        hasCheckIfIsAdmin = true;
                    }
                    
                    if (!isAdmin)
                    {
                        if (error == Blaze::CLUBS_ERR_USER_NOT_MEMBER)
                        {
                            TRACE_LOG("[getPetitionsForClubs] user [" << requestorId << "] does not have admin view for club ["
                                << *iter << "] on CLUBS_PETITION_SENT_TO_CLUB and is not allowed this action");
                        }
                        else if (error != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
                        {
                            // not a common or expected case, so WARN ...
                            WARN_LOG("[getPetitionsForClubs] failed to get petition type [petitionType]: " << petitionsType
                                << " user [blazeid]: " << requestorId << " club [clubid]: " << *iter << ", error was " << ErrorHelp::getErrorName(error));
                        }

                        iter = requestorlocalClubIdList.erase(iter);
                    }
                }
                else
                {
                    iter++;
                }
            }

            // need to check again since all the clubs may have been removed from the list because the user isn't a club admin nor is he belonging to any of them
            if (!requestorlocalClubIdList.empty())
            {
                error = dbc.getDb().getClubMessagesByClubs(requestorlocalClubIdList, 
                    CLUBS_PETITION, sortType, clubMessageListFromDB);
            }

            break;
        }
    default:
        {
            error = Blaze::CLUBS_ERR_INVALID_ARGUMENT;
            break;
        }
    }
    dbc.releaseConnection();

    if (error != Blaze::ERR_OK)
    {
        return error;
    }
    
    ClubMessageList::iterator it = clubMessageListFromDB.begin();
    for(; it != clubMessageListFromDB.end(); it++)
    {
        ClubMessagePtr clubMsg = *it;

        UserInfoPtr userInfo = nullptr;
        error = gUserSessionManager->lookupUserInfoByBlazeId(clubMsg->getSender().getBlazeId(), userInfo);
        if (error == Blaze::ERR_OK && userInfo != nullptr)
        {
            UserInfo::filloutUserCoreIdentification(*userInfo, clubMsg->getSender());

            bool insert = false;
            if (petitionsType == CLUBS_PETITION_SENT_BY_ME)
            {
                // if clubIds aren't specified, retrieve all petitions sent by me for all clubs
                if (localClubIdListEmpty)
                {
                    insert = true;
                }                
                else
                {
                    // if clubIds are specified, only insert those that are from those clubs
                    for (ClubIdList::const_iterator itr = requestorlocalClubIdList.begin(); itr != requestorlocalClubIdList.end(); ++itr)
                    {
                        if (clubMsg->getClubId() == *itr)
                        {
                            insert = true;
                            break;
                        }
                    }
                }
            }
            else  //CLUBS_PETITION_SENT_TO_CLUB
            {
                insert = true;
            }

            if (insert)
            {
                if (clubMessageListMap.find(clubMsg->getClubId()) == clubMessageListMap.end())
                {
                    ClubMessageList *clubMessageList = BLAZE_NEW ClubMessageList;
                    clubMessageListMap[clubMsg->getClubId()] = clubMessageList;
                }

                clubMessageListMap[clubMsg->getClubId()]->push_back(clubMsg);
            }
        }
        else
        {
            WARN_LOG("[getPetitionsForClubs] error (" << ErrorHelp::getErrorName(error) << ") looking up user " << clubMsg->getSender().getBlazeId()
                     << " for club " << clubMsg->getClubId());
        }
    }

    return ERR_OK;
}


BlazeRpcError ClubsSlaveImpl::getClubTickerMessagesForClubs(const ClubIdList& requestorClubIdList,
                                            const uint32_t oldestTimestamp,
                                            const uint32_t locale,
                                            ClubTickerMessageListMap& clubTickerMessageListMap)
{
    TRACE_LOG("[getClubTickerMessagesForClubs] start");

    BlazeRpcError result = Blaze::ERR_OK;
    BlazeId requestorId = INVALID_BLAZE_ID;
    if (gCurrentUserSession != nullptr)
    {
        requestorId = gCurrentUserSession->getBlazeId();
    }

    ClubsDbConnector dbc(this); 
    dbc.acquireDbConnection(true);

    ClubIdList requestorClubIdListLocal;
    bool rtorIsAdmin = false;
    bool hasCheckedAdmin = false;

    requestorClubIdList.copyInto(requestorClubIdListLocal);

    ClubIdList::iterator iter = requestorClubIdListLocal.begin();

    // club admin and members are allowed to fetch ticker messages for club
    for (; iter != requestorClubIdListLocal.end();)
    {
        MembershipStatus mshipStatus;
        MemberOnlineStatus onlSatus;
        TimeValue memberSince;
        ClubId requestorClubId = *iter;

        result = getMemberStatusInClub(
            requestorClubId, 
            requestorId, 
            dbc.getDb(), 
            mshipStatus, 
            onlSatus,
            memberSince);

        if (result != Blaze::ERR_OK)
        {
            if (!hasCheckedAdmin)
            {
                hasCheckedAdmin = true;
                rtorIsAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);
            }

            if (!rtorIsAdmin)
            {
                TRACE_LOG("[getClubTickerMessagesForClubs] user [" << requestorId
                    << "] does not have admin view for club [" << requestorClubId << "] and is not allowed this action");

                iter = requestorClubIdListLocal.erase(iter);
            }
            else
            {
                iter++;
            }
        }
        else
        {
            iter++;
        }

        if (rtorIsAdmin)
            break;
    }

    if (requestorClubIdListLocal.empty())
    {
        return CLUBS_ERR_NO_PRIVILEGE;
    }

    ClubsDatabase::TickerMessageList msgList;

    result = dbc.getDb().getTickerMsgs(
        requestorClubIdList,
        requestorId,
        oldestTimestamp,
        msgList);

    if (result != Blaze::ERR_OK)
    {
        ERR_LOG("[GetClubTickerMessagesCommand] failed to get ticker messages for user [blazeid]: " 
            << requestorId << " error is " << result);
        dbc.releaseConnection();
        return result;
    }

    for (ClubsDatabase::TickerMessageList::const_iterator it = msgList.begin();
        it != msgList.end(); it++)
    {
        ClubId clubId = it->mClubId;

        if (clubTickerMessageListMap.find(clubId) == clubTickerMessageListMap.end())
        {
            ClubTickerMessageList *newList = BLAZE_NEW ClubTickerMessageList();
            clubTickerMessageListMap[clubId] = newList;
        }

        ClubTickerMessage *msg = clubTickerMessageListMap[clubId]->pull_back();
        msg->setMetadata(it->mMetadata.c_str());
        msg->setTimestamp(it->mTimestamp);

        eastl::string localizedText;
        localizeTickerMessage(
            it->mText.c_str(), 
            it->mParams.c_str(), 
            locale, 
            localizedText);

        msg->setText(localizedText.c_str());
    }

    return result;
}

BlazeRpcError ClubsSlaveImpl::countMessagesForClubs(const ClubIdList& requestorClubIdList, const MessageType messageType, ClubMessageCountMap &messageCountMap)
{
    ClubsDbConnector dbc(this); 
    if (dbc.acquireDbConnection(true) == nullptr)
        return ERR_SYSTEM;

    BlazeRpcError error = Blaze::ERR_OK;
    ClubIdList localClubIdList;
    requestorClubIdList.copyInto(localClubIdList);

    BlazeId requestorId = INVALID_BLAZE_ID;
    if (gCurrentUserSession != nullptr)
    {
        requestorId = gCurrentUserSession->getBlazeId();
    }

    bool isAdmin = false;
    bool hasCheckedIsAdmin = false;

    ClubIdList::iterator iter = localClubIdList.begin();
    while (!isAdmin && iter != localClubIdList.end())
    {
        // club admin and club members can count messages
        MembershipStatus mshipStatus;
        MemberOnlineStatus onlSatus;
        TimeValue memberSince;
        ClubId clubId = *iter;

        error = getMemberStatusInClub(
                                        clubId, 
                                        requestorId, 
                                        dbc.getDb(), 
                                        mshipStatus, 
                                        onlSatus,
                                        memberSince
                                    );

        if (error != Blaze::ERR_OK)
        {
            if (!hasCheckedIsAdmin)
            {
                hasCheckedIsAdmin = true;
                isAdmin = UserSession::isCurrentContextAuthorized(Blaze::Authorization::PERMISSION_CLUB_ADMINISTRATOR_VIEW, true);
            }

            if (!isAdmin)
            {
                if (error == Blaze::CLUBS_ERR_USER_NOT_MEMBER)
                {
                    TRACE_LOG("[countMessagesForClubs] user [" << requestorId
                        << "] does not have admin view for club [" << clubId << "] and is not allowed this action");
                }

                iter = localClubIdList.erase(iter);
            }
        }
        else
        {
            iter++;
        }
    }

    if (localClubIdList.empty())
        return error;

    error = dbc.getDb().countMessages(localClubIdList, messageType, messageCountMap);
    dbc.releaseConnection();

    if (error != Blaze::ERR_OK)           
    {
        if (error != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
        {
            ERR_LOG("[countMessagesForClubs] Database error (" << ErrorHelp::getErrorName(error) << ")");
        }
        return error;
    }

    return error;
}

} // Clubs
} // Blaze
