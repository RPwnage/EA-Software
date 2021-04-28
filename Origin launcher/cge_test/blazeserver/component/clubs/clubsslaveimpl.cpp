/*************************************************************************************************/
/*!
    \file   clubsslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ClubsSlaveImpl

    Clubs Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/database/dbutil.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/replication/replicator.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/rpc/oauthslave.h"
#include "framework/tdf/oauth.h"

// clubs includes
#include "clubs/clubsslaveimpl.h"
#include "clubs/clubsdbconnector.h"

#include "stats/statsconfig.h"
#include "stats/statsslaveimpl.h"

#include "EASTL/set.h"

#include "xblprivacyconfigs/tdf/xblprivacyconfigs.h"
#include "xblprivacyconfigs/rpc/xblprivacyconfigsslave.h"

namespace Blaze
{

namespace Metrics
{
namespace Tag
{
TagInfo<Clubs::ClubDomain>* club_domain = BLAZE_NEW TagInfo<Clubs::ClubDomain>("club_domain", [](const Clubs::ClubDomain& value, Metrics::TagValue&) { return value.getDomainName(); });
}
}


namespace Clubs
{

ClubsTransactionContext::ClubsTransactionContext(const char* description, ClubsSlaveImpl &component)
    : TransactionContext(description)
{
    mDbConnPtr = gDbScheduler->getConnPtr(component.getDbId());

    if (mDbConnPtr != nullptr)
    {
        // this database connection is now transaction context bound, *not* fiber bound
        mDbConnPtr->clearOwningFiber();
        if (mDbConnPtr->startTxn() != Blaze::ERR_OK)
        {
            mDbConnPtr = DbConnPtr();
        }
    }
}

ClubsTransactionContext::~ClubsTransactionContext()
{
}

BlazeRpcError ClubsTransactionContext::commit()
{
    return mDbConnPtr->commit(); 
}

BlazeRpcError ClubsTransactionContext::rollback()
{
    return mDbConnPtr->rollback();
}

BlazeRpcError ClubsSlaveImpl::createTransactionContext(uint64_t customData, TransactionContext*& result)
{

    result = nullptr;

    ClubsTransactionContext *context 
        = BLAZE_NEW_NAMED("ClubsTransactionContext") ClubsTransactionContext("ClubsTransactionContext", *this);

    if (context->getDatabaseConnection() == nullptr)
    {
        delete context;
        return ERR_SYSTEM;
    }

    result = static_cast<TransactionContext*>(context);
    return ERR_OK;
}

// static
const char8_t* ClubsSlaveImpl::SEASON_ROLLOVER_TASKNAME = "SEASON_ROLLOVER_TASK";
const char8_t* ClubsSlaveImpl::INACTIVE_CLUB_PURGE_TASKNAME = "INACTIVE_CLUB_PURGE_TASK";
const char8_t* ClubsSlaveImpl::FETCH_COMPONENTINFO_TASKNAME = "FETCH_COMPONENTINFO_TASKNAME";
const char8_t* ClubsSlaveImpl::FETCH_COMPONENTINFO_TASKNAME_ONETIME = "FETCH_COMPONENTINFO_TASKNAME_ONETIME";

static const eastl::string sStadNameElementLeading = "<StadName value=";
static const eastl::string sStadNameElementEnding = "/>";
static const eastl::string sResetStadName = "\"Stadium\"";

ClubsSlave* ClubsSlave::createImpl()
{
    return BLAZE_NEW_NAMED("ClubsSlaveImpl") ClubsSlaveImpl();
}

ClubsSlaveImpl::ClubsSlaveImpl() :
    mPerfGetNewsTimeMsHighWatermark(getMetricsCollection(), "gaugeGetNewsTimeMsHighWatermark"),
    mPerfFindClubsTimeMsHighWatermark(getMetricsCollection(), "gaugeFindClubsTimeMsHighWatermark"),
    mPerfClubsCreated(getMetricsCollection(), "clubsCreated", Metrics::Tag::club_domain),
    mPerfMessages(getMetricsCollection(), "messagesExchanged"),
    mPerfClubGamesFinished(getMetricsCollection(), "clubGamesFinished"),
    mPerfAdminActions(getMetricsCollection(), "adminActions"),
    mPerfJoinsLeaves(getMetricsCollection(), "membersJoinedLeft"),
    mPerfMemberLogins(getMetricsCollection(), "memberLogins"),
    mPerfMemberLogouts(getMetricsCollection(), "memberLogouts"),
    mPerfOnlineClubs(getMetricsCollection(), "onlineClubs", [this]() { return mOnlineStatusCountsForClub.size(); }),
    mPerfOnlineMembers(getMetricsCollection(), "onlineMembers", [this]() { return mOnlineStatusForUserPerClubCache.size(); }),
    mPerfInactiveClubsDeleted(getMetricsCollection(), "purgedClubs", Metrics::Tag::club_domain),
    mPerfUsersInClubs(getMetricsCollection(), "clubMemberships", Metrics::Tag::club_domain),
    mPerfClubs(getMetricsCollection(), "clubs", Metrics::Tag::club_domain),
    mAsyncSequenceId(0),
    mFirstSnapshot(false),
    mIsUsingClubKeyscopedStats(false),
    mIsUsingClubKeyscopedStatsSet(false),
    mOnlineStatusByClubIdByBlazeIdFieldMap(RedisId("Clubs", "mOnlineStatusByClubIdByBlazeIdFieldMap")),
    mScheduler(nullptr),
    mInactiveClubPurgeTaskId(INVALID_TASK_ID),
    mUpdateComponentInfoTaskId(INVALID_TASK_ID),
    mStartEndOfSeasonTimerId(INVALID_TIMER_ID),
    mUpdateUserJobQueue("ClubsSlaveImpl::mUpdateUserJobQueue")
{
    mUpdateUserJobQueue.setMaximumWorkerFibers(FiberJobQueue::MEDIUM_WORKER_LIMIT);

    mIdentityProvider = BLAZE_NEW ClubsIdentityProvider(this);
    gIdentityManager->registerProvider(COMPONENT_ID, *mIdentityProvider);
    gPetitionableContentManager->registerProvider(COMPONENT_ID, *this);


    // register itself to census data manager
    gCensusDataManager->registerProvider(COMPONENT_ID, *this);
}

ClubsSlaveImpl::~ClubsSlaveImpl()
{
    mClubDomains.clear();

    delete mIdentityProvider;

    for (OnlineStatusCountsForClub::iterator itr = mOnlineStatusCountsForClub.begin();
         itr != mOnlineStatusCountsForClub.end();
         ++itr)
    {
        delete itr->second;
    }
    mOnlineStatusCountsForClub.clear();

    for (OnlineStatusForUserPerClubCache::iterator itr = mOnlineStatusForUserPerClubCache.begin();
         itr != mOnlineStatusForUserPerClubCache.end();
         ++itr)
    {
        delete itr->second;
    }
    mOnlineStatusForUserPerClubCache.clear();

    mClubDataCache.clear();
    mClubMemberInfoCache.clear();
}

void ClubsSlaveImpl::checkConfigUInt16(const int16_t configVal, const char8_t *name, ConfigureValidationErrors& validationErrors) const
{
    if (configVal == -1)
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].checkConfigUInt16(): missing configuration parameter <uint16> %s.", name);
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
}

void ClubsSlaveImpl::checkConfigUInt32(const int32_t configVal, const char8_t *name, ConfigureValidationErrors& validationErrors) const
{
    if (configVal == -1)
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].checkConfigUInt32(): missing configuration parameter <uint32> %s.", name);
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
}

void ClubsSlaveImpl::checkConfigString(const char8_t *configVal, const char8_t *name, ConfigureValidationErrors& validationErrors) const
{
    if (*configVal == '\0')
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].checkConfigString(): missing configuration parameter <string> %s.", name);
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
}

bool ClubsSlaveImpl::isUsingClubKeyscopedStats()
{
    if (mIsUsingClubKeyscopedStatsSet)
        return mIsUsingClubKeyscopedStats;

    Stats::StatsSlaveImpl* stats = 
        static_cast<Stats::StatsSlaveImpl*>
        (gController->getComponent(Stats::StatsSlave::COMPONENT_ID));    
    if (stats != nullptr)
    {
        const Stats::KeyScopesMap& keyScopeMap = stats->getConfigData()->getKeyScopeMap();
        mIsUsingClubKeyscopedStats = (keyScopeMap.find( "club" ) != keyScopeMap.end());

        mIsUsingClubKeyscopedStatsSet = true;
        return mIsUsingClubKeyscopedStats;
    }

    return false;
}

/*************************************************************************************************/
/*!
    \brief getClubsConfigValues
    get configuration file values into memory structures for faster access
    \param[in]  - none
    \return - true on success
*/
/*************************************************************************************************/
void ClubsSlaveImpl::getClubsConfigValues()
{
    const ClubsConfig &config = getConfig();
    const ClubDomainList &domainList = config.getDomains();
    ClubDomainList::const_iterator it = domainList.begin();
    ClubDomainList::const_iterator itend = domainList.end();
    for (int cnt =1; it != itend; ++it, ++cnt)
    {
        uint32_t domainId = static_cast<uint32_t>((*it)->getClubDomainId());

        mClubDomains[static_cast<ClubDomainId>(domainId)] = *it;
    }
}
/************************************************************************************************/
bool ClubsSlaveImpl::onValidateConfig(ClubsConfig& config, const ClubsConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    checkConfigUInt16(config.getDivisionSize(), "DivisionSize", validationErrors);
    checkConfigUInt16(config.getMaxEvents(), "MaxEvents", validationErrors);
    checkConfigUInt16((uint16_t)config.getPurgeClubHour(), "PurgeClubHour", validationErrors);
    checkConfigUInt16(config.getMaxRivalsPerClub(), "MaxRivalsPerClub", validationErrors);
    checkConfigUInt32(config.getTickerMessageExpiryDay(), "TickerMessageExpiryDay", validationErrors);

    // validate domains
    const ClubDomainList &domainList = config.getDomains();
    if(domainList.size() == 0)
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): missing configuration parameter Domains.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        ClubDomainList::const_iterator it = domainList.begin();
        ClubDomainList::const_iterator itend = domainList.end();
        for (int cnt =1; it != itend; ++it, ++cnt)
        {
            size_t errorSum = validationErrors.getErrorMessages().size();

            checkConfigUInt32((*it)->getClubDomainId(), "ClubDomainId", validationErrors);
            checkConfigString((*it)->getDomainName(), "DomainName", validationErrors);
            checkConfigUInt32((*it)->getMaxMembersPerClub(), "maxMembersPerClub", validationErrors);
            checkConfigUInt16((*it)->getMaxGMsPerClub(), "maxGMsPerClub", validationErrors);
            checkConfigUInt16((*it)->getMaxMembershipsPerUser(), "maxMembershipsPerUser", validationErrors);
            checkConfigUInt16((*it)->getMaxInvitationsPerUserOrClub(), "MaxInvitationsPerUserOrClub", validationErrors);
            checkConfigUInt16((*it)->getMaxInactiveDaysPerClub(), "MaxInactiveDaysPerClub", validationErrors);
            checkConfigUInt16((*it)->getMaxNewsItemsPerClub(), "MaxNewsItemsPerClub", validationErrors);

            if (errorSum < validationErrors.getErrorMessages().size())
            {
                eastl::string msg;
                msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): Check %d element of Domain sequence.", cnt);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }

            uint32_t domainId = static_cast<uint32_t>((*it)->getClubDomainId());
            for (ClubDomainList::const_iterator domainIt = domainList.begin(); domainIt != it; ++domainIt)
            {
                if (domainId == static_cast<uint32_t>((*domainIt)->getClubDomainId()))
                {
                    eastl::string msg;
                    msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): duplicate club domain id %d"
                        " at %d element of Domains.", domainId, cnt);
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                    break;
                }
            }

            if ((*it)->getMaxMembershipsPerUser() > 1 && (*it)->getClubJumpingInterval().getSec() > 0)
            {
                eastl::string msg;
                msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): clubJumpingInterval of club domain %d should not be bigger than 0, because the multi-memberships is supported.", domainId);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
        }
    }

    // validate record settings
    const RecordSettingItemList &rsil = config.getRecordSettings();
    RecordSettingItemList::const_iterator rsit = rsil.begin();
    RecordSettingItemList::const_iterator rsendit = rsil.end();
    for (int cnt = 1; rsit != rsendit; ++rsit, ++cnt)
    {
        size_t errorSum = validationErrors.getErrorMessages().size();

        checkConfigUInt32((*rsit)->getId(), "Id", validationErrors);
        checkConfigString((*rsit)->getName(), "Name", validationErrors);
        checkConfigString((*rsit)->getDescription(), "Description", validationErrors);
        checkConfigString((*rsit)->getStatType(), "StatType", validationErrors);
        checkConfigString((*rsit)->getFormat(), "Format", validationErrors);

        if (blaze_strcmp((*rsit)->getStatType(), RECORD_STAT_TYPE_INT) != 0 &&
            blaze_strcmp((*rsit)->getStatType(), RECORD_STAT_TYPE_FLOAT) != 0)
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): bad stat type"
                " declared in RecordSettings"
                " at %d element of RecordSettings.",
                cnt);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }

        if (errorSum < validationErrors.getErrorMessages().size())
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): Check %d element of RecordSettings sequence.", cnt);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    // validate event log strings
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_CREATE_CLUB, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_JOIN_CLUB, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_LEAVE_CLUB, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_REMOVED_FROM_CLUB, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_AWARD_TROPHY, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_GM_PROMOTED, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_GM_DEMOTED, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_OWNERSHIP_TRANSFERRED, validationErrors);
    checkEventMappingString(config.getEventStrings(), LOG_EVENT_FORCED_CLUB_NAME_CHANGE, validationErrors);
	checkEventMappingString(config.getEventStrings(), LOG_EVENT_MAX_CLUB_NAME_CHANGES, validationErrors);
    
    // validate season settings
    const SeasonSettingItem &seasonMap = config.getSeasonSettings();
    if (seasonMap.getStatCategory()[0] == '\0')
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): statCategory definition missing from SeasonSettings section of config.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    const RolloverItem &rolloverMap = seasonMap.getRollover();
    // If all rollover settings are -1, it means that rollover settings are removed from the config file clubs.cfg,
    // in this case rollover settings needn't to be validated.
    if (!((rolloverMap.getMaxRetry()) == -1 &&
          (rolloverMap.getMinute() == -1) &&
          (rolloverMap.getHour() == -1) &&
          (rolloverMap.getDay() == -1) &&
          (rolloverMap.getPeriod() == -1) &&
          (rolloverMap.getTimeout() == -1)))
    {
        int32_t seasonStatsCategoryPeriod = rolloverMap.getPeriod();
        if (seasonStatsCategoryPeriod < 0 || seasonStatsCategoryPeriod > CLUBS_PERIOD_MONTHLY)
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): invalid period definition from rollover map section of SeasonSettings section of config.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    if ((rolloverMap.getMinute() < 0) ||
        (rolloverMap.getMinute() > 59) ||
        (rolloverMap.getHour() < 0) ||
        (rolloverMap.getHour() > 23) ||
        (rolloverMap.getDay() < 0) ||
        (rolloverMap.getDay() > 31) ||
        (rolloverMap.getPeriod() < 0) ||
        (rolloverMap.getPeriod() > 2))
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): invalid parameters for season rollover");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        if ((rolloverMap.getPeriod() == CLUBS_PERIOD_WEEKLY) &&
            (rolloverMap.getDay() > 6))
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): invalid day parameter for weekly rollover");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        if ((rolloverMap.getPeriod() == CLUBS_PERIOD_MONTHLY) &&
            (rolloverMap.getDay() < 1))
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): invalid day parameter for monthly rollover");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    // validate awards settings
    const AwardSettingList &awardList = config.getAwardSettings();
    AwardSettingList::const_iterator it = awardList.begin();
    AwardSettingList::const_iterator itend = awardList.end();

    for (int cnt = 1; it != itend; ++it, ++cnt)
    {
        size_t errorSum = validationErrors.getErrorMessages().size();

        checkConfigUInt32((*it)->getAwardChecksum(), "AwardChecksum", validationErrors);
        checkConfigUInt32((*it)->getAwardId(), "AwardId", validationErrors);
        checkConfigString((*it)->getAwardName(), "AwardName", validationErrors);
        checkConfigString((*it)->getAwardURL(), "AwardURL", validationErrors);
        
        if (errorSum < validationErrors.getErrorMessages().size())
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].onValidateConfig(): Check %d element of AwardSetting sequence.", cnt);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }

    validateClubUpdateStats(config, validationErrors);

    return validationErrors.getErrorMessages().empty();
}


void ClubsSlaveImpl::validateClubUpdateStats(ClubsConfig& config, ConfigureValidationErrors& validationErrors) const
{
    BlazeRpcError waitResult = Blaze::ERR_OK;
    Stats::StatsSlave* statsComponent = getStatsSlave(&waitResult);
    if (waitResult != ERR_OK || statsComponent == nullptr)
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): could not obtain Stats component.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
        return;
    }

    // get categories from stat config.
    Blaze::Stats::StatCategoryList statCategoriesRsp;
    
    //Put this in a super user context
    {
        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
        waitResult = statsComponent->getStatCategoryList(statCategoriesRsp);
    }
    
    if (waitResult != ERR_OK)
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): could not obtain Stats category list: %s", ErrorHelp::getErrorName(waitResult));
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
        return;
    }

    //check the ClubStatsUpdates stat categories exist and column name is specified
    for(ClubStatsUpdates::const_iterator updatesIter = config.getClubStatsUpdates().begin(); updatesIter != config.getClubStatsUpdates().end(); ++updatesIter)
    {
        const char8_t* wipeSet = (*updatesIter)->getWipeSet();

        //check if the wipeSet is legal
        if (wipeSet == nullptr || wipeSet[0] == '\0')
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): wipeSet could not be nullptr.");
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
        }

        for(ClubStatsCategoryList::const_iterator categoriesIter = (*updatesIter)->getCategories().begin(); categoriesIter != (*updatesIter)->getCategories().end(); ++categoriesIter)
        {
            const char8_t* categoryName = (*categoriesIter)->getName();
            const char8_t* keySopeName = (*categoriesIter)->getClubKeyscopeName();

            //check if the categoryName is legal
            if (categoryName == nullptr || categoryName[0] == '\0')
            {
                eastl::string msg;
                msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): categoryName could not be nullptr.");
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return;
            }

            //check the categoryName is specified in stats config and entitytype is club.
            const Blaze::Stats::StatCategorySummary* statCategory = findInStatCategoryList(categoryName, statCategoriesRsp.getCategories());
            bool correctCategoryName = false;
            if (statCategory != nullptr)
            {
                if (keySopeName != nullptr && keySopeName[0] != '\0')
                {
                    const Blaze::Stats::StatCategorySummary::StringScopeNameList& scopeNames = statCategory->getKeyScopes();
        
                    //check the stats is club member stats
                    if (!findInStatKeyScopeList(keySopeName, scopeNames))
                    {
                        eastl::string msg;
                        msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): categoryName %s does not have this keyScopeName %s.", categoryName, keySopeName);
                        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                        str.set(msg.c_str());
                        return;
                    }

                    //get category's stats from stats config.
                    Blaze::Stats::GetStatDescsRequest statDescsReq;
                    Blaze::Stats::StatDescs statDescsRsp;
                    statDescsReq.setCategory(categoryName);
                                        
                    {
                        UserSession::SuperUserPermissionAutoPtr autoPtr(true);
                        waitResult = statsComponent->getStatDescs(statDescsReq, statDescsRsp);
                    }                    
                    
                    if (ERR_OK != waitResult)
                    {
                        eastl::string msg;
                        msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): could not obtain Stats category(%s) stats list: %s", categoryName, ErrorHelp::getErrorName(waitResult));
                        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                        str.set(msg.c_str());
                        return;
                    }

                    for (Clubs::StatsList::const_iterator statsIter = (*categoriesIter)->getStats().begin(); statsIter != (*categoriesIter)->getStats().end(); ++statsIter)
                    {
                        //check the column is specified in stats config.
                        const Blaze::Stats::StatDescSummary* statSumm = findInStatDescList(statsIter->c_str(), statDescsRsp.getStatDescs());
                        if (nullptr == statSumm)
                        {
                            eastl::string msg;
                            msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): stats name %s is not specified in stats config.", statsIter->c_str());
                            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                            str.set(msg.c_str());
                            return;
                        }
                        else if (statSumm->getDerived())
                        {
                            eastl::string msg;
                            msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): stats name %s should be a base stats not derived stats.", statsIter->c_str());
                            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                            str.set(msg.c_str());
                            return;
                        }
                    }
                    correctCategoryName = true;
                }
                else if (statCategory->getEntityType() == Blaze::Clubs::ENTITY_TYPE_CLUB)
                {
                    correctCategoryName = true;
                }
            }

            if (!correctCategoryName)
            {
                eastl::string msg;
                msg.sprintf("[ClubsSlaveImpl].validateClubUpdateStats(): categoryName %s doesn't exist in club stats.", categoryName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                return;
            }
        }
    }
}

bool ClubsSlaveImpl::onConfigure()
{
    TRACE_LOG("[ClubsSlaveImpl].onConfigure(): configuring component.");

    const ClubsConfig &config = getConfig();

    getClubsConfigValues();

    ClubsDatabase::initialize(getDbId());

    mDbAttempts = config.getFindClubsSettings().getDbAttempts();
    mFetchSize = config.getFindClubsSettings().getFetchSize();

    mScheduler = static_cast<TaskSchedulerSlaveImpl*>
        (gController->getComponent(TaskSchedulerSlave::COMPONENT_ID, true, true));
    if (mScheduler != nullptr)
    {
        mScheduler->registerHandler(COMPONENT_ID, this);
    }

    mSeasonRollover.setComponent(this);
    const SeasonSettingItem &seasonSettings = config.getSeasonSettings();
    // If all rollover settings are -1, it means that rollover settings are removed from the config file clubs.cfg,
    // in this case rollover timer needn't to be set.
    if (!((seasonSettings.getRollover().getMaxRetry()) == -1 &&
          (seasonSettings.getRollover().getMinute() == -1) &&
          (seasonSettings.getRollover().getHour() == -1) &&
          (seasonSettings.getRollover().getDay() == -1) &&
          (seasonSettings.getRollover().getPeriod() == -1) &&
          (seasonSettings.getRollover().getTimeout() == -1)))
    {
        mSeasonRollover.parseRolloverData(seasonSettings);
        mSeasonRollover.setRolloverTimer();
    }

    // schedule any stale clubs purging
    uint32_t year, month, day, hour, minute, second, millis;
    TimeValue::getGmTimeComponents(TimeValue::getTimeOfDay(),
        &year, &month, &day, &hour, &minute, &second, &millis);

    int32_t purgeHour = config.getPurgeClubHour();
    //the "PurgeClubHour" configure the default is "-1", if the user can't setting it, we change it to zero.
    if (purgeHour == -1)
        purgeHour = 0;
    
    TimeValue nextTv(year, month, day, purgeHour, 0, 0);
    if (hour >= static_cast<uint32_t>(purgeHour))
    {
        // we've passed today's time, so schedule for tomorrow's
        nextTv += (int64_t) 86400 * 1000000;
#if 0
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
                    86400000000Ui64;
#else
                    86400000000ULL;
#endif
#endif
    }

    uint32_t purgeTime = static_cast<uint32_t>(nextTv.getSec());
    uint32_t purgePeriod = static_cast<uint32_t>(86400); // 1 day in seconds

    // We can fire and forget here as the TaskScheduler will sort out multiple slaves attempting to schedule the same task
    if (mScheduler != nullptr)
    {
        mScheduler->createTask(INACTIVE_CLUB_PURGE_TASKNAME, COMPONENT_ID, nullptr, purgeTime, 0, purgePeriod);

        bool runOneTimeTask = true;
        if (mUpdateComponentInfoTaskId != INVALID_TASK_ID)
        {
            const ScheduledTaskInfo* taskInfo = mScheduler->getTask(mUpdateComponentInfoTaskId);
            uint32_t startTime = taskInfo->getStart();
            const uint32_t updateThreshold = 5;
            // census data will update in less than 5 seconds, so don't bother running the one time task
            if (startTime - TimeValue::getTimeOfDay().getSec() < updateThreshold)
                runOneTimeTask = false;
        }
        else
        {
            // Schedule the recurring task since it doesn't yet exist
            // Since we're populating the census data immediately via the one-time task below, delay running the recurring task
            // until the first scheduled recurrence
            mScheduler->createTask(FETCH_COMPONENTINFO_TASKNAME, COMPONENT_ID, nullptr, 
                static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec() + config.getRefreshCensusInterval().getSec()), 0, static_cast<uint32_t>(config.getRefreshCensusInterval().getSec()));
        }

        if (runOneTimeTask)
        {
            BlazeRpcError err = mScheduler->runOneTimeTaskAndBlock(FETCH_COMPONENTINFO_TASKNAME_ONETIME, COMPONENT_ID,
                TaskSchedulerSlaveImpl::RunOneTimeTaskCb(this, &ClubsSlaveImpl::doTakeComponentInfoSnapshot));
            if (err != ERR_OK)
            {
                WARN_LOG("[ClubsSlaveImpl].updateComponentInfoOneTimeTask: failed to run component info update task ("
                    << FETCH_COMPONENTINFO_TASKNAME_ONETIME << ").");
            }
        }

    }

    // Complete the state change to CONFIGURED
    return true;
}

bool ClubsSlaveImpl::onResolve()
{
    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->addLocalUserSessionSubscriber(*this);
    gUserSessionManager->registerExtendedDataProvider(COMPONENT_ID, *this);
    gUserSetManager->registerProvider(COMPONENT_ID, *this);
    gController->registerDrainStatusCheckHandler(Blaze::Clubs::ClubsSlave::COMPONENT_INFO.name, *this);

    return true;
}

void ClubsSlaveImpl::onShutdown()
{
    if (mStartEndOfSeasonTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mStartEndOfSeasonTimerId);

    mUpdateUserJobQueue.join();

    if (gUserSessionMaster != nullptr)
        gUserSessionMaster->removeLocalUserSessionSubscriber(*this);

    gIdentityManager->deregisterProvider(COMPONENT_ID);
    gPetitionableContentManager->deregisterProvider(COMPONENT_ID);
    gUserSessionManager->deregisterExtendedDataProvider(COMPONENT_ID);
    gUserSetManager->deregisterProvider(COMPONENT_ID);
    gController->deregisterDrainStatusCheckHandler(Clubs::ClubsSlave::COMPONENT_INFO.name);
    // remove itself from census data manager
    gCensusDataManager->unregisterProvider(COMPONENT_ID);

    if (mScheduler != nullptr)
        mScheduler->deregisterHandler(COMPONENT_ID);
}

bool ClubsSlaveImpl::isDrainComplete()
{
    return (getPendingTransactionsCount() == 0);
}

void ClubsSlaveImpl::checkEventMappingString(const EventStringsMap& eventStringsMap, const char8_t* key, ConfigureValidationErrors& validationErrors) const
{
    EventStringsMap::const_iterator
        it = eventStringsMap.find(key),
        end = eventStringsMap.end();

    if (it == end)
    {
        eastl::string msg;
        msg.sprintf("[ClubsSlaveImpl].checkEventMappingString(): Unable to locate event string "
            "mapping for '%s'", key);
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    else
    {
        if (it->second[0] == '\0')
        {
            eastl::string msg;
            msg.sprintf("[ClubsSlaveImpl].checkEventMappingString(): Unable to locate event string "
                "mapping for '%s'", key);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
    }
}

const char8_t* ClubsSlaveImpl::getEventString(const char8_t* key) const
{
    EventStringsMap::const_iterator it = getConfig().getEventStrings().find(key);

    if (it == getConfig().getEventStrings().end())
        return nullptr;

    return it->second;
}

BlazeRpcError ClubsSlaveImpl::logEvent(ClubsDatabase* clubsDb, ClubId clubId, const char8_t* message,
        NewsParamType arg0type, const char8_t* arg0,
        NewsParamType arg1type, const char8_t* arg1,
        NewsParamType arg2type, const char8_t* arg2,
        NewsParamType arg3type, const char8_t* arg3)
{
    if (clubsDb == nullptr || message == nullptr)
        return ERR_SYSTEM;

    ClubsEvent clubEvent;
    clubEvent.clubId = clubId;
    blaze_strnzcpy(clubEvent.message, message, sizeof(clubEvent.message));
    clubEvent.numArgs = 0;
    if (arg0 != nullptr)
    {
        clubEvent.args[0].type = arg0type;
        blaze_strnzcpy(clubEvent.args[0].value, arg0, sizeof(clubEvent.args[0].value));
        ++clubEvent.numArgs;
        if (arg1 != nullptr)
        {
            clubEvent.args[1].type = arg1type;
            blaze_strnzcpy(clubEvent.args[1].value, arg1, sizeof(clubEvent.args[1].value));
            ++clubEvent.numArgs;
            if (arg2 != nullptr)
            {
                clubEvent.args[2].type = arg2type;
                blaze_strnzcpy(clubEvent.args[2].value, arg2,
                        sizeof(clubEvent.args[2].value));
                ++clubEvent.numArgs;
                if (arg3 != nullptr)
                {
                    clubEvent.args[3].type = arg3type;
                    blaze_strnzcpy(clubEvent.args[3].value, arg3,
                            sizeof(clubEvent.args[3].value));
                    ++clubEvent.numArgs;
                }
            }
        }
    }

    BlazeRpcError errorCode = logEventAsNews(clubsDb, &clubEvent);
    if (errorCode != Blaze::ERR_OK)
    {
        TRACE_LOG("[ClubsSlaveImpl].logEvent(): Error logging events for Online Clubs!");
    }
    
    return errorCode;
    
}

const ClubDomain* ClubsSlaveImpl::getClubDomain(ClubDomainId domainId) const
{
    ClubDomainsById::const_iterator
        domainItr = mClubDomains.find(domainId),
        domainEnd = mClubDomains.end();

    if (domainItr != domainEnd)
    {
        return domainItr->second;
    }

    return nullptr;
}

void ClubsSlaveImpl::updateCachedClubInfo(uint64_t version, ClubId clubId, const Club& updatedClub)
{
    TRACE_LOG("[ClubsSlaveImpl].updateCachedClubInfo");

    // if clubId is invalid, there is nothing to do
    if (INVALID_CLUB_ID == clubId)
    {
        WARN_LOG("[ClubsSlaveImpl].updateCachedClubInfo: Invalid clubId provided");
        return;
    }

    UpdateCacheRequest req;
    req.setClubId(clubId);

    CachedClubData &updatedClubData = req.getUpdatedClubData();
    updatedClubData.setName(updatedClub.getName());
    updatedClubData.setClubDomainId(updatedClub.getClubDomainId());
    updatedClub.getClubSettings().copyInto(updatedClubData.getClubSettings());
    updatedClubData.setVersion(version);

    sendUpdateCacheNotificationToAllSlaves(&req);
}

void ClubsSlaveImpl::invalidateCachedClubInfo(ClubId clubId, bool removeIdentityProviderCacheEntry/* = false*/)
{
    TRACE_LOG("[ClubsSlaveImpl].invalidateCachedClubInfo");

    InvalidateCacheRequest req;
    req.setCacheId(CLUB_DATA);
    req.setClubId(clubId);
    req.setRemoveFromIdentityProviderCache(removeIdentityProviderCacheEntry);

    sendInvalidateCacheNotificationToAllSlaves(&req);
}

void ClubsSlaveImpl::invalidateCachedMemberInfo(BlazeId blazeId, ClubId clubId)
{
    TRACE_LOG("[ClubsSlaveImpl].invalidateCachedMemberInfo");

    InvalidateCacheRequest req;
    req.setCacheId(CLUB_MEMBER_DATA);
    req.setBlazeId(blazeId);
    req.setClubId(clubId);

    sendInvalidateCacheNotificationToAllSlaves(&req);
}

BlazeRpcError ClubsSlaveImpl::updateCachedMamberInfoOnTransferOwnership(BlazeId oldOwner, BlazeId newOwner, MembershipStatus exOwnersNewStatus, ClubId clubId)
{
    if (exOwnersNewStatus == CLUBS_OWNER)
        return CLUBS_ERR_INVALID_ARGUMENT;

    invalidateCachedMemberInfo(oldOwner, clubId);
    invalidateCachedMemberInfo(newOwner, clubId);

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::getClubDomainForClub(
    ClubId clubId, 
    ClubsDatabase &clubsDb,
    const ClubDomain* &clubDomain)
{
    BlazeRpcError result = Blaze::ERR_OK;

    ClubDomainId domainId;

    // fetch club's domainId from cache
    ClubDataCache::const_iterator itr = mClubDataCache.find(clubId);
    if (itr != mClubDataCache.end())
    {
        domainId = itr->second->getClubDomainId();
        clubDomain = getClubDomain(domainId);
        if (clubDomain == nullptr)
        {
            WARN_LOG("[ClubsSlaveImpl].getClubDomainForClub(): Club " << clubId << " has an invalid domain (in cache): " << domainId);
            result = Blaze::ERR_SYSTEM;
        }
    }
    else
    {
        Club club;
        uint64_t version = 0;
        result = clubsDb.getClub(clubId, &club, version);
        if (result == Blaze::ERR_OK)
        {
            domainId = club.getClubDomainId();
            clubDomain = getClubDomain(domainId);
            if (clubDomain == nullptr)
            {
                ERR_LOG("[ClubsSlaveImpl].getClubDomainForClub(): Club " << clubId << " has an invalid domain: " << domainId);
                result = Blaze::ERR_SYSTEM;
            }
            else // add the club to the cache
            {
                addClubDataToCache(clubId, domainId, club.getClubSettings(), club.getName(), version);
            }
        }
    }

    return result;
}

void ClubsSlaveImpl::removeIdentityCacheEntry(ClubId clubId)
{
    mIdentityProvider->removeCacheEntry(clubId);
}

BlazeRpcError ClubsSlaveImpl::checkPetitionAlreadySent(
    ClubsDatabase &clubsDb, 
    BlazeId requestorId,
    ClubId clubId)
{
    ClubMessageList existingMsgs;
    
    BlazeRpcError error;
    
    error = clubsDb.getClubMessagesByUserSent(
        requestorId, 
        CLUBS_PETITION, 
        CLUBS_SORT_TIME_ASC, existingMsgs);
        
    if (error == Blaze::ERR_OK)
    {
        for (ClubMessageList::const_iterator it = existingMsgs.begin(); it != existingMsgs.end(); ++it)
        {
            const ClubMessage *msg = *it;
            if (msg->getClubId() == clubId)
            {
                return static_cast<BlazeRpcError>(CLUBS_ERR_PETITION_ALREADY_SENT);
            }
        }
    }
    else
    {
        // database error
        return error;
    }
    
    return static_cast<BlazeRpcError>(ERR_OK);
}

BlazeRpcError ClubsSlaveImpl::checkInvitationAlreadySent(
    ClubsDatabase &clubsDb,
    BlazeId blazeId,
    ClubId clubId) const
{

    BlazeRpcError error;

    ClubMessageList existingMsgs;
    error = clubsDb.getClubMessagesByUserReceived(
        blazeId, 
        CLUBS_INVITATION, 
        CLUBS_SORT_TIME_ASC, 
        existingMsgs);

    if (error == Blaze::ERR_OK)
    {
        for (ClubMessageList::const_iterator it = existingMsgs.begin(); it != existingMsgs.end(); ++it)
        {
            const ClubMessage *msg = *it;
            if (msg->getClubId() == clubId)
            {
                return static_cast<BlazeRpcError>(CLUBS_ERR_INVITATION_ALREADY_SENT);
            }
        }
    }
    else
    {
        // database error
        return error;
    }
    
    return static_cast<BlazeRpcError>(ERR_OK);
}

BlazeRpcError ClubsSlaveImpl::updateMemberUserExtendedData(BlazeId blazeId, ClubId clubId, UpdateReason reason, MemberOnlineStatus status)
{
    ClubsDatabase clubsDb;
    DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
    clubsDb.setDbConn(conn);

    if (conn == nullptr)
        return ERR_SYSTEM;

    const ClubDomain* domain;
    BlazeRpcError err = getClubDomainForClub(clubId, clubsDb, domain);

    if (err == ERR_OK)
        err = updateMemberUserExtendedData(blazeId, clubId, domain->getClubDomainId(), reason, status);

    return err;
}
/*! \brief update club to user's extended data, or the UED's associated member status as needed */
BlazeRpcError ClubsSlaveImpl::updateMemberUserExtendedData(BlazeId blazeId, ClubId clubId, ClubDomainId domainId, UpdateReason reason, MemberOnlineStatus status)
{
    TRACE_LOG("[ClubsSlaveImpl].updateMemberUserExtendedData");
    BlazeRpcError error = ERR_OK;
    const ClubDomain* domain = getClubDomain(domainId);
    if (blazeId == 0 || clubId == INVALID_CLUB_ID || domain == nullptr)
    {
        FAIL_LOG("[ClubsSlaveImpl::updateMemberUserExtendedData]: Invalid args, with blazeId = " << 
            blazeId << " and clubId = " << clubId << " and domainId = " << domainId)
        return ERR_SYSTEM;
    }

    if (domain->getTrackMembershipInUED())
    {
        if (reason == UPDATE_REASON_CLUB_CREATED || reason == UPDATE_REASON_USER_JOINED_CLUB)
        {
            EA::TDF::ObjectId objId = EA::TDF::ObjectId(ENTITY_TYPE_CLUB, clubId);
            error = gUserSessionManager->insertBlazeObjectIdForUser(blazeId, objId);
        }

        if (error == ERR_OK)
        {
            // so we can update the online status count cache on each slave.
            updateOnlineStatusOnExtendedDataChange(clubId, blazeId, status);
        }
    }
    return error;
}

BlazeRpcError ClubsSlaveImpl::removeMemberUserExtendedData(BlazeId blazeId, ClubId clubId, UpdateReason reason)
{
    TRACE_LOG("[ClubsSlaveImpl].removeMemberUserExtendedData");

    BlazeRpcError error = ERR_OK;
    if (blazeId == 0 || clubId == INVALID_CLUB_ID)
    {
        FAIL_LOG("[ClubsSlaveImpl::removeMemberUserExtendedData]: Invalid args, with blazeId = " << 
            blazeId << " and clubId = " << clubId)
        return ERR_SYSTEM;
    }

    EA::TDF::ObjectId objId = EA::TDF::ObjectId(ENTITY_TYPE_CLUB, clubId);
    error = gUserSessionManager->removeBlazeObjectIdForUser(blazeId, objId);
    if (error == ERR_OK)
    {
        // so we can update the online status count cache on each slave.
        updateOnlineStatusOnExtendedDataChange(clubId, blazeId, CLUBS_MEMBER_OFFLINE);
    }

    return error;
}


void ClubsSlaveImpl::updateOnlineStatusOnExtendedDataChange(ClubId clubId, BlazeId blazeId, MemberOnlineStatus status)
{
    // We usually don't want any existing status to be overwritten by CLUBS_MEMBER_ONLINE_NON_INTERACTIVE. If a user
    // is already online and then creates another session (e.g. via WAL), then his existing status shouldn't change.
    // (The one exception is logging out - this may destroy the user's primary session, and if he still has one or more
    // non-primary sessions, then his new status should be non-interactive.)
    // Note that this logic also prevents users from manually setting their status to CLUBS_MEMBER_ONLINE_NON_INTERACTIVE
    // via the updateMemberOnlineStatus RPC.
    bool changedLocalOnlineStatusCache = updateLocalOnlineStatusAndCounts(clubId, blazeId, status, false /*allowOverwriteWithNoninteractive*/);
    bool refreshLocalOnlineStatusCache = false;
    bool confirmNoninteractive = false;
    if (!changedLocalOnlineStatusCache)
    {
        // There may be an in-flight UpdateMemberOnlineStatus notification from another instance. In this case, our local cache would be
        // out of sync with redis, and we'd need to apply and broadcast this latest update even though we haven't made any changes locally. [GOS-30029]
        // We should also refresh our local cache after updating redis, in case the in-flight UpdateMemberOnlineStatus notification arrives while we're
        // blocked in a redis call (before the redis update is applied).
        MemberOnlineStatusValue redisStatus = CLUBS_MEMBER_OFFLINE;
        RedisError rc = mOnlineStatusByClubIdByBlazeIdFieldMap.find(blazeId, clubId, redisStatus);
        if (rc != REDIS_ERR_OK && rc != REDIS_ERR_NOT_FOUND)
        {
            ERR_LOG("[ClubsSlaveImpl].updateOnlineStatusOnExtendedDataChange: failed to look up status for user(" << blazeId << ") and club("
                << clubId << "): RedisError: " << RedisErrorHelper::getName(rc) << "; forcing (possibly redundant) update to " << MemberOnlineStatusToString(status));
            refreshLocalOnlineStatusCache = true;
            confirmNoninteractive = true;
        }
        else if (static_cast<MemberOnlineStatus>(redisStatus) != status)
        {
            // Don't allow updates from CLUBS_MEMBER_ONLINE_INTERACTIVE to CLUBS_MEMBER_ONLINE_NON_INTERACTIVE
            if (static_cast<MemberOnlineStatus>(redisStatus) != CLUBS_MEMBER_ONLINE_INTERACTIVE || status != CLUBS_MEMBER_ONLINE_NON_INTERACTIVE)
            {
                WARN_LOG("[ClubsSlaveImpl].updateOnlineStatusOnExtendedDataChange: user(" << blazeId << ") and club(" << clubId << ") has unexpected status "
                    << MemberOnlineStatusToString(static_cast<MemberOnlineStatus>(redisStatus)) << " in redis; forcing update to " << MemberOnlineStatusToString(status));
                refreshLocalOnlineStatusCache = true;
            }
        }
    }
    if (changedLocalOnlineStatusCache || refreshLocalOnlineStatusCache)
    {
        NotifyUpdateMemberOnlineStatus req;
        req.getClubIds().push_back(clubId);
        req.setUserLoggedOff(false);
        req.setBlazeId(blazeId);

        broadcastOnlineStatusChange(req, status, refreshLocalOnlineStatusCache, confirmNoninteractive);
    }
}

void ClubsSlaveImpl::onLocalUserSessionLogin(const UserSessionMaster& userSession)
{
    TRACE_LOG("[ClubsSlaveImpl].onLocalUserSessionLogin for local session(" << userSession.getUserSessionId() << ").");

    // We pass in a copy of the usersession as UserSessionData as UserSessions are non-copyable.
    // We need to copy here because the usersession may be deleted before the fiber call can execute
    mUpdateUserJobQueue.queueFiberJob(this, &ClubsSlaveImpl::doOnLocalUserSessionLogin, gUserSessionMaster->getLocalSession(userSession.getUserSessionId()), "ClubsSlaveImpl::doLocalOnUserSessionLogin");
}

void ClubsSlaveImpl::doOnLocalUserSessionLogin(UserSessionMasterPtr userSession)
{
    const ObjectIdList &list = userSession->getExtendedData().getBlazeObjectIdList();
    ClubIdList clubIds;

    for (ObjectIdList::const_iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it).type == ENTITY_TYPE_CLUB)
        {
            const ClubId clubId = it->toObjectId<ClubId>();
            if (clubId != INVALID_CLUB_ID)
                clubIds.push_back(clubId);
        }
    }

    if (clubIds.empty())
        return;

    mPerfMemberLogins.increment();

    // IMPORTANT: updateMemberUserExtendedData() is a blocking call, extended data can mutate during this; therefore, we must operate on cached club ids
    for (ClubIdList::const_iterator it = clubIds.begin(), end = clubIds.end(); it != end; ++it)
    {
        const ClubId clubId = *it;
        BlazeRpcError result = updateMemberUserExtendedData(userSession->getBlazeId(), clubId, UPDATE_REASON_USER_SESSION_CREATED, getOnlineStatus(userSession->getBlazeId()));
        TRACE_LOG("[ClubsSlaveImpl].doLocalOnUserSessionLogin: Updating UED for session(" << userSession->getUserSessionId() << "), result=" << ErrorHelp::getErrorName(result) );
    }

    ClubsDatabase clubsDb;
    DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
    clubsDb.setDbConn(conn);

    if (conn == nullptr)
    {
        WARN_LOG("[ClubsSlaveImpl].doLocalOnUserSessionLogin: Failed to get db conn, session(" << userSession->getUserSessionId() << ")");
        return;
    }

    BlazeRpcError result = addClubsDataToCache(&clubsDb, clubIds);
    if (result == ERR_OK)
    {
        result = onMemberOnline(&clubsDb, clubIds, userSession->getBlazeId());
        if (result != ERR_OK)
        {
            WARN_LOG("[ClubsSlaveImpl].doLocalOnUserSessionLogin: onMemberOnline failed for session(" << userSession->getUserSessionId() << "), result=" << ErrorHelp::getErrorName(result) );
        }
    }
    else
    {
        WARN_LOG("[ClubsSlaveImpl].doLocalOnUserSessionLogin: addClubsDataToCache failed for session(" << userSession->getUserSessionId() << "), result=" << ErrorHelp::getErrorName(result) );
    }
}

void ClubsSlaveImpl::onLocalUserSessionLogout(const UserSessionMaster& userSession)
{
    TRACE_LOG("[ClubsSlaveImpl].onLocalUserSessionLogout for session(" << userSession.getUserSessionId() << ").");

    // is user a club member?
    const ObjectIdList& list = userSession.getExtendedData().getBlazeObjectIdList();

    bool isClubMember = false;

    for (ObjectIdList::const_iterator it = list.begin(); it != list.end(); ++it)
    {
        if ((*it).type == ENTITY_TYPE_CLUB)
        {
            const ClubId clubId = it->toObjectId<ClubId>();
            if (clubId != INVALID_CLUB_ID)
            {
                isClubMember = true;
                // unsubscribe from club ticker messages
                if (userSession.isPrimarySession())
                    setClubTickerSubscription(clubId, userSession.getBlazeId(), false);
            }
        }
    }

    if (isClubMember)
    {
        mPerfMemberLogouts.increment();
    }

    mUpdateUserJobQueue.queueFiberJob(this, &ClubsSlaveImpl::doOnLocalUserSessionLogout, userSession.getBlazeId(), userSession.getUserSessionId(), "ClubsSlaveImpl::doOnLocalUserSessionLogout");
}

void ClubsSlaveImpl::doOnLocalUserSessionLogout(BlazeId blazeId, UserSessionId sessionId)
{
    // Find all clubs we have cached off for this logged out user
    OnlineStatusForUserPerClubCache::iterator userItr = mOnlineStatusForUserPerClubCache.find(blazeId);
    if (userItr != mOnlineStatusForUserPerClubCache.end())
    {
        TRACE_LOG("[ClubsSlaveImpl].doOnLocalUserSessionLogout for session(" << sessionId << ") & blaze id (" << blazeId << ").");

        UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(blazeId);
        UserSessionIdList sessionIds;
        gUserSessionManager->getSessionIds(blazeId, sessionIds);

        bool hasPrimarySession = (primarySessionId != INVALID_USER_SESSION_ID) && (primarySessionId != sessionId);
        bool hasSessions = sessionIds.size() > 1 || (sessionIds.size() == 1 && sessionIds.back() != sessionId);

        NotifyUpdateMemberOnlineStatus request;
        OnlineStatusInClub::const_iterator clubItr = userItr->second->begin();
        while (clubItr != userItr->second->end())
        {
            ClubId clubId = clubItr->first;
            MemberOnlineStatus oldStatus = clubItr->second;
            MemberOnlineStatus newStatus = oldStatus;

            if (hasSessions && !hasPrimarySession)
            {
                // If we've logged out the primary session, but there are other sessions tied to this user (e.g., WAL),
                // then we need to update the status to NON_INTERACTIVE
                newStatus = CLUBS_MEMBER_ONLINE_NON_INTERACTIVE;
            }
            else if (!hasSessions)
            {
                newStatus = CLUBS_MEMBER_OFFLINE;

                InvalidateCacheRequest req;
                req.setCacheId(CLUB_MEMBER_DATA);
                req.setBlazeId(blazeId);
                req.setClubId(clubId);
                onInvalidateCacheNotification(req, nullptr /*associatedUserSession - don't care*/);
            }

            bool deleteEntry = false;
            if (newStatus != oldStatus)
            {
                if (updateLocalOnlineStatusAndCounts(clubId, blazeId, newStatus, true /*allowOverwriteWithNoninteractive*/, false /*deleteOnlineStatusInClub*/))
                {
                    request.getClubIds().push_back(clubId);
                    if (newStatus == CLUBS_MEMBER_OFFLINE)
                        deleteEntry = true;
                }
            }

            if (deleteEntry)
                clubItr = userItr->second->erase(clubItr);
            else
                ++clubItr;
        }

        // If this user is no longer online in any clubs, delete his entry from the OnlineStatusForUserPerClub map
        if (userItr->second->empty())
        {
            delete userItr->second;
            mOnlineStatusForUserPerClubCache.erase(userItr);
        }

        // We've updated our local maps; now update Redis and notify the other slaves
        if (!request.getClubIds().empty())
        {
            request.setBlazeId(blazeId);
            request.setUserLoggedOff(!hasSessions);

            broadcastOnlineStatusChange(request, hasSessions ? CLUBS_MEMBER_ONLINE_NON_INTERACTIVE : CLUBS_MEMBER_OFFLINE);
        }
    }
}

// extended user data provider implementation
BlazeRpcError ClubsSlaveImpl::onLoadExtendedData(
    const UserInfoData &data, 
    UserSessionExtendedData &extendedData)
{
    BlazeId blazeId = data.getId();
    TRACE_LOG("[Clubs].onLoadExtendedData user ID: " << blazeId);
    bool dbIssuesUseDefaultUED = gUserSessionManager->getConfig().getAllowLoginOnUEDLookupError();

    if (blazeId == INVALID_BLAZE_ID)
    {
        // Given invalid blaze id would not look up anything from clubs db can early out here
        return ERR_OK;
    }

    ClubsDatabase clubsDb;
    DbConnPtr conn = gDbScheduler->getReadConnPtr(getDbId());
    if (conn == nullptr)
    {
        if (dbIssuesUseDefaultUED)
            return ERR_OK;
        else
            return ERR_SYSTEM;
    }

    clubsDb.setDbConn(conn);

    ClubId clubId = INVALID_CLUB_ID;
    BlazeRpcError err = ERR_OK;

    for (ClubDomainsById::const_iterator it = mClubDomains.begin(); it != mClubDomains.end(); ++it)
    {
        if (!it->second->getTrackMembershipInUED())
            continue;

        ClubDomainId domainId = it->first;

        ClubMembershipList clubMembershipList;
        if (clubsDb.getClubMembershipsInDomain(domainId, blazeId, clubMembershipList) == Blaze::ERR_OK)
        {
            for (ClubMembershipList::const_iterator mit = clubMembershipList.begin();
                 mit != clubMembershipList.end();
                 ++mit)
            {
                clubId = (*mit)->getClubId();

                //pack extended data
                EA::TDF::ObjectId objId = EA::TDF::ObjectId(ENTITY_TYPE_CLUB, clubId);
                extendedData.getBlazeObjectIdList().push_back(objId);
            }
        }
    }

    return err;
}

void ClubsSlaveImpl::onInvalidateCacheNotification(const Blaze::Clubs::InvalidateCacheRequest& data, UserSession *associatedUserSession)
{
    TRACE_LOG("[ClubsSlaveImpl].onInvalidateCacheNotification");

    // Get cache ID from request data
    if (CLUB_DATA == data.getCacheId())
    {
        ClubDataCache::iterator itr = mClubDataCache.find(data.getClubId());
        if (itr != mClubDataCache.end())
        {
            mClubDataCache.erase(itr);
        }

        if (data.getRemoveFromIdentityProviderCache())
        {
            removeIdentityCacheEntry(data.getClubId());
        }
    }
    else if (CLUB_MEMBER_DATA == data.getCacheId())
    {
        ClubMemberInfoCache::iterator itr = mClubMemberInfoCache.find(data.getClubId());
        if (itr != mClubMemberInfoCache.end())
        {
            BlazeIdToMemberInfoMap::iterator memberItr = itr->second.find(data.getBlazeId());
            if (memberItr != itr->second.end())
            {
                itr->second.erase(memberItr);
            }

            if (itr->second.empty())
            {
                mClubMemberInfoCache.erase(itr);
            }
        }
    }
}

void ClubsSlaveImpl::onUpdateCacheNotification(const Blaze::Clubs::UpdateCacheRequest& data, UserSession *associatedUserSession)
{
    TRACE_LOG("[ClubsSlaveImpl].onUpdateCacheNotification");

    ClubDataCache::iterator itr = mClubDataCache.find(data.getClubId());
    if (itr != mClubDataCache.end())
    {
        if (data.getUpdatedClubData().getVersion() > itr->second->getVersion())
            data.getUpdatedClubData().copyInto(*itr->second);
    }
    else
    {
        TRACE_LOG("[ClubsSlaveImpl].onUpdateCacheNotification: Adding club " << data.getUpdatedClubData().getName() << " to cache.")
        CachedClubData* cachedData = BLAZE_NEW CachedClubData;
        data.getUpdatedClubData().copyInto(*cachedData);
        mClubDataCache[data.getClubId()] = cachedData;
    }
}

void ClubsSlaveImpl::onUpdateClubsComponentInfo(const ClubsComponentInfo& data, UserSession *associatedUserSession)
{
    TRACE_LOG("[ClubsSlaveImpl].onUpdateClubsComponentInfo");

    data.copyInto(mClubsCompInfo);

    mPerfClubs.reset();
    for (ClubCensusDomainMap::const_iterator domainit = mClubsCompInfo.getClubsByDomain().begin(); domainit != mClubsCompInfo.getClubsByDomain().end(); ++domainit)
    {
        const ClubDomain *domain = getClubDomain((*domainit).first);
        if (domain != nullptr)
            mPerfClubs.set((*domainit).second, *domain);
    }

    mPerfUsersInClubs.reset();
    for (ClubCensusDomainMap::const_iterator domainit = mClubsCompInfo.getMembersByDomain().begin(); domainit != mClubsCompInfo.getMembersByDomain().end(); ++domainit)
    {
        const ClubDomain *domain = getClubDomain((*domainit).first);
        if (domain != nullptr)
            mPerfUsersInClubs.set((*domainit).second, *domain);
    }
}


void ClubsSlaveImpl::onUpdateOnlineStatusNotification(const Blaze::Clubs::NotifyUpdateMemberOnlineStatus& data, UserSession *associatedUserSession)
{
    TRACE_LOG("[ClubsSlaveImpl].onUpdateOnlineStatusNotification for user(" << data.getBlazeId() << ").");

     mUpdateUserJobQueue.queueFiberJob<ClubsSlaveImpl, const NotifyUpdateMemberOnlineStatusPtr, bool>(this, &ClubsSlaveImpl::handleOnlineStatusUpdate, data.clone(), data.getUserLoggedOff(), "ClubsSlaveImpl::handleOnlineStatusUpdate");
}

void ClubsSlaveImpl::handleOnlineStatusUpdate(const NotifyUpdateMemberOnlineStatusPtr request, bool invalidateClubMemberInfoCache)
{
    for (Clubs::ClubIdList::const_iterator itr = request->getClubIds().begin(); itr != request->getClubIds().end(); ++itr)
    {
        if (invalidateClubMemberInfoCache)
        {
            InvalidateCacheRequest req;
            req.setCacheId(CLUB_MEMBER_DATA);
            req.setBlazeId(request->getBlazeId());
            req.setClubId(*itr);
            onInvalidateCacheNotification(req, nullptr /*associatedUserSession - don't care*/);
        }

        MemberOnlineStatusValue status = CLUBS_MEMBER_OFFLINE;
        RedisError rc = mOnlineStatusByClubIdByBlazeIdFieldMap.find(request->getBlazeId(), *itr, status);
        if (rc == REDIS_ERR_OK || rc == REDIS_ERR_NOT_FOUND)
        {
            updateLocalOnlineStatusAndCounts(*itr, request->getBlazeId(), static_cast<MemberOnlineStatus>(status));
        }
        else
        {
            ERR_LOG("[ClubsSlaveImpl].onUpdateOnlineStatusNotification: failed to look up entry for user(" << request->getBlazeId() << ") and club("
                << *itr << "): RedisError: " << RedisErrorHelper::getName(rc));
        }
    }
}

void ClubsSlaveImpl::getClubOnlineStatusCounts(ClubId clubId, MemberOnlineStatusCountsMap &map)
{
    OnlineStatusCountsForClub::const_iterator itr = mOnlineStatusCountsForClub.find(clubId);
    for (MemberOnlineStatus status = CLUBS_MEMBER_ONLINE_INTERACTIVE; status < CLUBS_MEMBER_ONLINE_STATUS_COUNT; status = (MemberOnlineStatus)((uint32_t)status + 1))
    {
        if (itr != mOnlineStatusCountsForClub.end())
            map[status] = static_cast<uint16_t>((*itr->second)[status]);
        else
            map[status] = 0;
    }
}

void ClubsSlaveImpl::addClubDataToCache(
    ClubId clubId,
    ClubDomainId domainId,
    const ClubSettings& clubSettings,
    const ClubName& clubName,
    uint64_t version)
{
    if (clubId == INVALID_CLUB_ID)
    {
        FAIL_LOG("[ClubsSlaveImpl].addClubDataToCache: Invalid args, with clubId = " << clubId)
        return;
    }

    UpdateCacheRequest req;
    req.setClubId(clubId);

    CachedClubData &updatedClubData = req.getUpdatedClubData();
    updatedClubData.setName(clubName);
    updatedClubData.setClubDomainId(domainId);
    clubSettings.copyInto(updatedClubData.getClubSettings());
    updatedClubData.setVersion(version);

    sendUpdateCacheNotificationToAllSlaves(&req);
}

BlazeRpcError ClubsSlaveImpl::addClubsDataToCache(ClubsDatabase *clubsDb, const ClubIdList& clubIds)
{
    ClubList clubs;
    uint64_t* versions = nullptr;
    BlazeRpcError result = clubsDb->getClubs(clubIds, clubs, versions);
    if (result != ERR_OK)
        return result;

    uint32_t i = 0;
    for (ClubList::const_iterator begin = clubs.begin();
         begin != clubs.end();
         ++begin, ++i)
    {
        const Club& club = **begin;
        addClubDataToCache(club.getClubId(), club.getClubDomainId(), club.getClubSettings(), club.getName(), versions[i]);
    }

    if (versions != nullptr)
        delete [] versions;

    return ERR_OK;
}

void ClubsSlaveImpl::addClubMemberInfoToCache(BlazeId blazeId, ClubId clubId, MembershipStatus membershipStatus, uint32_t memberSince) const
{
    if (clubId == INVALID_CLUB_ID || blazeId == 0)
    {
        FAIL_LOG("[ClubsSlaveImpl::addCachedUserDataToRequest]: Invalid args, with blazeId = " << 
            blazeId << " and clubId = " << clubId)
        return;
    }

    // Get the membership map from the cache
    ClubMemberInfoCache::iterator itr = mClubMemberInfoCache.find(clubId);
    if (itr == mClubMemberInfoCache.end())
    {
        ClubMemberInfoCache::insert_return_type ret = mClubMemberInfoCache.insert(eastl::make_pair(clubId, BlazeIdToMemberInfoMap()));
        itr = ret.first;
    }

    BlazeIdToMemberInfoMap& memberMap = itr->second;
    BlazeIdToMemberInfoMap::iterator memberItr = memberMap.find(blazeId);
    if (memberItr == memberMap.end())
    {
        CachedMemberInfo* info = BLAZE_NEW CachedMemberInfo;
        info->setMembershipSinceTime(memberSince);
        info->setMembershipStatus(membershipStatus);
        memberMap[blazeId] = info;
    }
}

BlazeRpcError ClubsSlaveImpl::updateCachedDataOnNewUserAdded(
    ClubId clubId,
    const ClubName& clubName,
    ClubDomainId domainId,
    const ClubSettings& clubSettings,
    BlazeId blazeId, 
    MembershipStatus membershipStatus,
    uint32_t memberSince,
    UpdateReason reason)
{
    if (reason != UPDATE_REASON_CLUB_CREATED && reason != UPDATE_REASON_USER_JOINED_CLUB)
    {
        FAIL_LOG("[ClubsSlaveImpl::updateCachedDataOnNewUserAdded]: Invalid args, with reason = " << UpdateReasonToString(reason))
        return ERR_SYSTEM;
    }

    if (reason == UPDATE_REASON_CLUB_CREATED)
    {
        addClubDataToCache(clubId, domainId, clubSettings, clubName, 0);
    }

    addClubMemberInfoToCache(blazeId, clubId, membershipStatus, memberSince);

    return Blaze::ERR_OK;
}

BlazeRpcError ClubsSlaveImpl::awardClub(
        DbConnPtr& dbConn,
        ClubId clubId, 
        ClubAwardId awardId)
{
    BlazeRpcError result = ERR_OK;

    // dbConn is active connection on which transaction was started    
    // also, corresponding club data row in database is locked at 
    // this point 
    if (dbConn == nullptr)
        return static_cast<BlazeRpcError>(ERR_SYSTEM);

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    result = clubsDb.updateClubAward(clubId, awardId);
    result = (result == Blaze::ERR_OK) ? 
        clubsDb.updateClubAwardCount(clubId) : result;

    if (result == Blaze::ERR_OK)
    {
        result = onAwardUpdated(&clubsDb, clubId, awardId);
    }

    return result;
}

BlazeRpcError ClubsSlaveImpl::getMemberStatusInClub(
    const ClubId clubId,
    const BlazeId blazeId,
    ClubsDatabase &clubsDb,
    MembershipStatus &membershipStatus,
    MemberOnlineStatus &onlineStatus,
    TimeValue &memberSince)
{
    bool foundInCache = false;
    BlazeRpcError result = Blaze::ERR_OK;

    onlineStatus = CLUBS_MEMBER_OFFLINE;
    ClubMemberInfoCache::const_iterator itr = mClubMemberInfoCache.find(clubId);
    if (itr != mClubMemberInfoCache.end())
    {
        const BlazeIdToMemberInfoMap& memberMap = itr->second;
        BlazeIdToMemberInfoMap::const_iterator memberItr = memberMap.find(blazeId);
        if (memberItr != memberMap.end())
        {
            foundInCache = false;
            membershipStatus = memberItr->second->getMembershipStatus();
            memberSince.setSeconds((int64_t)memberItr->second->getMembershipSinceTime());

            OnlineStatusForUserPerClubCache::iterator statusItr = mOnlineStatusForUserPerClubCache.find(blazeId);
            if (statusItr != mOnlineStatusForUserPerClubCache.end())
            {
                OnlineStatusInClub *statusInClub = statusItr->second;
                OnlineStatusInClub::const_iterator onlStatusIter = statusInClub->find(clubId);
                if (onlStatusIter != statusInClub->end())
                {
                    onlineStatus = onlStatusIter->second;
                    foundInCache = true;
                }
            }
        }
    }


    // not in cache? then it must be in db
    if (!foundInCache)
    {
        TRACE_LOG("[getMemberStatusInClub] Cache miss, user " << blazeId << ", clubId " << clubId << ".");

        ClubMembership clubMembership;
        result = clubsDb.getMembershipInClub(clubId, blazeId, clubMembership);
        if (result == Blaze::ERR_OK)
        {
            membershipStatus = clubMembership.getClubMember().getMembershipStatus();
            memberSince.setSeconds((int64_t)clubMembership.getClubMember().getMembershipSinceTime());
            addClubMemberInfoToCache(blazeId, clubId, membershipStatus, clubMembership.getClubMember().getMembershipSinceTime());
        }
        else if(result == Blaze::CLUBS_ERR_USER_NOT_MEMBER)
        {
            if (!checkClubId(clubsDb.getDbConn(), clubId))
                result = Blaze::CLUBS_ERR_INVALID_CLUB_ID;
        }
    }

    return result;
}


BlazeRpcError ClubsSlaveImpl::getMemberStatusInClub(
    const ClubId clubId,
    const BlazeIdList& blazeIdList,
    ClubsDatabase &clubsDb,
    MembershipStatusMap &membershipStatusMap)
{
    BlazeRpcError result = Blaze::ERR_OK;
    BlazeIdList reqBlazeIds;
    blazeIdList.copyInto(reqBlazeIds);

    ClubMemberInfoCache::const_iterator itr = mClubMemberInfoCache.find(clubId);
    if (itr != mClubMemberInfoCache.end())
    {
        for (BlazeIdList::iterator i = reqBlazeIds.begin(); i != reqBlazeIds.end();)
        {
            BlazeId blazeId = *i;
            const BlazeIdToMemberInfoMap& memberMap = itr->second;
            BlazeIdToMemberInfoMap::const_iterator memberItr = memberMap.find(blazeId);
            if (memberItr != memberMap.end())
            {
                OnlineStatusForUserPerClubCache::iterator statusItr = mOnlineStatusForUserPerClubCache.find(blazeId);
                if (statusItr != mOnlineStatusForUserPerClubCache.end())
                {
                    OnlineStatusInClub *statusInClub = statusItr->second;
                    OnlineStatusInClub::const_iterator onlStatusIter = statusInClub->find(clubId);
                    if (onlStatusIter != statusInClub->end())
                    {
                        membershipStatusMap[blazeId] = onlStatusIter->second;
                    }
                    else
                    {
                        membershipStatusMap[blazeId] = CLUBS_MEMBER_OFFLINE;
                    }
                }
                i = reqBlazeIds.erase(i);
            }
            else
            {
                ++i;
            }
        }
    }

    // if there are still some user ids cannot be found in cache, go to the DB
    if (!reqBlazeIds.empty())
    {
        TRACE_LOG("[getMemberStatusInClub] Cache miss in clubId " << clubId << ".");

        result = clubsDb.getMembershipInClub(clubId, reqBlazeIds, membershipStatusMap);
        if(result == Blaze::CLUBS_ERR_USER_NOT_MEMBER)
        {
            if (!checkClubId(clubsDb.getDbConn(), clubId))
                result = Blaze::CLUBS_ERR_INVALID_CLUB_ID;
        }
    }

    return result;
}

BlazeRpcError ClubsSlaveImpl::getMemberStatusInDomain(
    ClubDomainId domainId,
    BlazeId blazeId,
    ClubsDatabase &clubsDb,
    ClubMemberStatusList &statusList,
    bool forceDbLookup)
{
    statusList.clear();

    bool foundInCache = false;
    BlazeRpcError result = Blaze::ERR_OK;

    // if user is online, try cache first unless we want to skip cache
    if (!forceDbLookup && gUserSessionManager->getPrimarySessionId(blazeId) != INVALID_USER_SESSION_ID)
    {
        const ClubDomain *domain = getClubDomain(domainId);
        if (domain == nullptr)
        {
            WARN_LOG("[ClubsSlaveImpl].getMemberStatusInDomain(): Invalid domain " << domainId);
            return Blaze::ERR_SYSTEM;
        }
        
        // if user is in cache, then all his memberships are cached if domain is
        // configured in such way
        if (domain->getTrackMembershipInUED())
        {
            foundInCache = true;
            MemberOnlineStatus onlineStatus = CLUBS_MEMBER_OFFLINE;

            OnlineStatusForUserPerClubCache::iterator statusItr = mOnlineStatusForUserPerClubCache.find(blazeId);
            if (statusItr != mOnlineStatusForUserPerClubCache.end())
            {
                OnlineStatusInClub *statusInClub = statusItr->second;
                for (OnlineStatusInClub::const_iterator onlStatusIter = statusInClub->begin(); onlStatusIter != statusInClub->end(); ++onlStatusIter)
                {
                    ClubId clubId = onlStatusIter->first;
                    onlineStatus = onlStatusIter->second;

                    result = getClubDomainForClub(clubId, clubsDb, domain);
                    if (result != Blaze::ERR_OK)
                    {
                        WARN_LOG("[ClubsSlaveImpl].getMemberStatusInDomain(): Error fetching domain for club " << clubId);
                        return result;
                    }

                    if (domain->getClubDomainId() == domainId)
                    {
                        // get membership info from cache
                        ClubMemberInfoCache::const_iterator itr = mClubMemberInfoCache.find(clubId);
                        if (itr != mClubMemberInfoCache.end())
                        {
                            const BlazeIdToMemberInfoMap& memberMap = itr->second;
                            BlazeIdToMemberInfoMap::const_iterator memberItr = memberMap.find(blazeId);
                            if (memberItr != memberMap.end())
                            {
                                MembershipStatus membershipStatus = memberItr->second->getMembershipStatus();
                                ClubMemberStatus status;
                                status.membershipStatus = membershipStatus;
                                status.onlineStatus = onlineStatus;
                                statusList[clubId] = status;
                            }
                        }
                    }
                }

            }

        }
    }

    // not in cache? then try the db
    if (!foundInCache)
    {
        ClubMembershipList clubMembershipList;
        result = clubsDb.getClubMembershipsInDomain(domainId, blazeId, clubMembershipList);
        if (result == Blaze::ERR_OK)
        {
            for (ClubMembershipList::iterator cItr = clubMembershipList.begin(); cItr != clubMembershipList.end(); ++cItr)
            {
                ClubMembership *clubMembership = *cItr;
                ClubMemberStatus status;
                status.membershipStatus = clubMembership->getClubMember().getMembershipStatus();
                status.onlineStatus = CLUBS_MEMBER_OFFLINE;
                statusList[clubMembership->getClubId()] = status;
            }
        }
    }

    return result;
}

BlazeRpcError ClubsSlaveImpl::lockClubs(Blaze::DbConnPtr& dbConn, ClubIdList *clubsToLock)
{
    BlazeRpcError err = Blaze::ERR_SYSTEM;

    if (dbConn == nullptr)
    {
        ERR_LOG("[lockClubs] Unable to get database connection.");
        return err;
    }

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    err = clubsDb.lockClubs(clubsToLock);

    if (err != Blaze::ERR_OK)
    {
        ERR_LOG("[lockClubs] Database error: " << ErrorHelp::getErrorName(err) << ".");
    }
        
    return err;
}

BlazeRpcError ClubsSlaveImpl::getClubsComponentSettings(ClubsComponentSettings &settings)
{
    settings.setClubDivisionSize(static_cast<uint16_t>(getConfig().getDivisionSize()));
    settings.setMaxEvents(static_cast<uint16_t>(getConfig().getMaxEvents()));
    settings.setPurgeHour(static_cast<uint16_t>(getConfig().getPurgeClubHour()));
    settings.setSeasonRolloverTime(static_cast<int32_t>(mSeasonRollover.getRollover()));
    settings.setSeasonStartTime(static_cast<int32_t>(mSeasonRollover.getSeasonStart()));
    settings.setMaxRivalsPerClub(static_cast<uint16_t>(getConfig().getMaxRivalsPerClub()));

    for (RecordSettingItemList::const_iterator rsilIt = getConfig().getRecordSettings().begin(); rsilIt != getConfig().getRecordSettings().end(); ++rsilIt)
    {
        RecordSettings *rcs = settings.getRecordSettings().pull_back();
        rcs->setRecordId(static_cast<uint32_t>((*rsilIt)->getId()));
        rcs->setRecordName((*rsilIt)->getName());
    }

    for (AwardSettingList::const_iterator asilIt = getConfig().getAwardSettings().begin(); asilIt != getConfig().getAwardSettings().end(); ++asilIt)
    {
        AwardSettings *aws = settings.getAwardSettings().pull_back();
        (*asilIt)->copyInto(*aws);
    }

    for (ClubDomainsById::const_iterator domainItr = mClubDomains.begin(); domainItr != mClubDomains.end(); ++domainItr)
    {
        const ClubDomain *domain = domainItr->second;
        settings.getDomainList().push_back(domain->clone());
    }

    return Blaze::ERR_OK;
}

void ClubsSlaveImpl::doTakeComponentInfoSnapshot(BlazeRpcError& error)
{
    error = takeComponentInfoSnapshot();
}

BlazeRpcError ClubsSlaveImpl::takeComponentInfoSnapshot()
{
    //get global state of clubs component
    ClubsDbConnector dbc(this);

    if (dbc.acquireDbConnection(true) == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].takeComponentInfoSnapshot: Could not obtain database connection.");
        return DB_ERR_SYSTEM;
    }

    // refresh current club count

    BlazeRpcError error = dbc.getDb().getClubsComponentInfo(mClubsCompInfo);
    dbc.releaseConnection();

    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].takeComponentInfoSnapshot: Failed to query database.");
        return error;
    }

    sendUpdateClubsComponentInfoToAllSlaves(&mClubsCompInfo);
    return ERR_OK;
}

void ClubsSlaveImpl::startEndOfSeason()
{
    mStartEndOfSeasonTimerId = INVALID_TIMER_ID;

    // Check the current season mStart against start time in DB
    // If previous season hasn't finished processing, DB start time
    // will be less than current season start time
    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
    if (dbConn == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].startEndOfSeason Unable to get database connection.");
        return;
    }

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    SeasonState currentState;
    int64_t seasonStartTime = 0;
    DbError error = clubsDb.getSeasonState(currentState, seasonStartTime);
    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].startEndOfSeason Unable to fetch season state.");
        return;
    }

    if (currentState == CLUBS_POST_SEASON)
    {
        if (mSeasonRollover.getSeasonStart() > seasonStartTime)
        {
            // Skip the end of season processing as likely the current processing is in progress or (less likely) failed
            WARN_LOG("[ClubsSlaveImpl].startEndOfSeason Current season start time (" << mSeasonRollover.getSeasonStart() << " is greater than expected (" << seasonStartTime
                << ". Skipping end of season processing.");
            return;
        }
    }

    if (mScheduler != nullptr)
    {
        // Since the task for each season's rollover must be uniquely identified, the task name will include the
        // start time; otherwise, runOneTimeTaskAndBlock() will prevent any end of season processing after the
        // first time.  At this point, the season rollover object has been updated and the new season start time
        // is actually the current time for rollover we'll be doing.
        struct tm tM;
        TimeValue::gmTime(&tM, (int64_t) mSeasonRollover.getSeasonStart());
        mSeasonRolloverTaskName.sprintf("%s_%d/%02d/%02d-%02d:%02d:%02d", SEASON_ROLLOVER_TASKNAME,
            tM.tm_year + 1900, tM.tm_mon + 1, tM.tm_mday, tM.tm_hour, tM.tm_min, tM.tm_sec);
        BlazeRpcError err = mScheduler->runOneTimeTaskAndBlock(mSeasonRolloverTaskName.c_str(), COMPONENT_ID,
            TaskSchedulerSlaveImpl::RunOneTimeTaskCb(this, &ClubsSlaveImpl::doStartEndOfSeason));
        if (err != ERR_OK)
        {
            WARN_LOG("[ClubsSlaveImpl].startEndOfSeason failed to run season rollover task ("
                << mSeasonRolloverTaskName.c_str() << ").");
        }
    }
}

void ClubsSlaveImpl::doStartEndOfSeason(BlazeRpcError& error)
{
    TRACE_LOG("[ClubsSlaveImpl:]].doStartEndOfSeason");

    error = Blaze::ERR_SYSTEM;
    Stats::StatsSlave* component = getStatsSlave(&error);
    if (component == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].doStartEndOfSeason Stats component not found!");
        return;
    }

    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
    SeasonState currentState;
    int64_t seasonStartTime = 0;

    if (dbConn == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].doStartEndOfSeason Unable to get database connection.");
        return;
    }

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    error = clubsDb.getSeasonState(currentState, seasonStartTime);
    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].doStartEndOfSeason Unable to fetch season state.");
        return;
    }
    
    // The expected state could be already in CLUBS_POST_SEASON if this is called
    // because the current season didn't finish processing (due to an error).
    // If that's the case, continue with the end of season processing.
    // The taskScheduler will prevent processing the same season multiple times in parallel, so we
    // should only get here if we didn't commit the end of season update
    if (currentState == CLUBS_IN_SEASON)
    {
        // transition to POST_SEASON
        error = clubsDb.updateSeasonState(CLUBS_POST_SEASON, mSeasonRollover.getSeasonStart()); // Update with current mStart time
        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[ClubsSlaveImpl].doStartEndOfSeason Unable to update season state.");
            return;
        }
    }

    BlazeRpcError dbErr = dbConn->startTxn();
    if (dbErr != DB_ERR_OK)
    {
        ERR_LOG("[[ClubsSlaveImpl].doStartEndOfSeason Unable to start txn. dbErr(" << getDbErrorString(dbErr) << ")");
        return;
    }
    error = clubsDb.lockSeason();
    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].doStartEndOfSeason Unable to lock season.");
        dbConn->rollback();
        return;
    }

    // read the season state again since it might have been
    // modified since our last read
    error = clubsDb.getSeasonState(currentState, seasonStartTime);
    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].doStartEndOfSeason Unable to fetch season state.");
        return;
    }

    if (currentState == CLUBS_POST_SEASON)
    {
        // actual end of season processing
        if (processEndOfSeason(component, dbConn))
        {
            // the season processing has finished so update DB and reset season state
            clubsDb.updateSeasonState(CLUBS_IN_SEASON, mSeasonRollover.getRollover()); // Update with current mRollover time (start time of the next season)
            dbErr = dbConn->commit();
            if (dbErr != DB_ERR_OK)
            {
                ERR_LOG("[ClubsSlaveImpl].doStartEndOfSeason Unable to commit txn. dbErr(" << getDbErrorString(dbErr) << ")");
            }
        }
        else
            dbConn->rollback();
    }
    else
    {
        // season processing was done while we were waiting for lock on the season
        dbConn->rollback();
    }
}

void ClubsSlaveImpl::purgeStaleObjectsFromDatabase()
{
    TRACE_LOG("[ClubsSlaveImpl].purgeStaleObjectsFromDatabase() - start!");

    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn == nullptr)
    {
        ERR_LOG("[ClubsSlaveImpl].purgeStaleObjectsFromDatabase() - failed to obtain connection");
        return;
    }

    uint32_t clubsBeforePurge;
    uint32_t diff;

    for (ClubDomainsById::const_iterator domainItr = mClubDomains.begin(); domainItr != mClubDomains.end(); ++domainItr)
    {
        const ClubDomain *domain = domainItr->second;

        clubsBeforePurge = mClubDataCache.size();
        if (domain->getMaxInactiveDaysPerClub() > 0)
        {
            purgeInactiveClubs(conn, domain->getClubDomainId(), domain->getMaxInactiveDaysPerClub());
            diff = mClubDataCache.size() - clubsBeforePurge;
            if(diff > 0)
                mPerfInactiveClubsDeleted.increment(diff, *domain);
        }
    }

    uint32_t tickerMessageExpiryDay = static_cast<uint32_t>(getConfig().getTickerMessageExpiryDay());
    if (tickerMessageExpiryDay > 0)
        purgeStaleTickerMessages(conn, tickerMessageExpiryDay);

    TRACE_LOG("[ClubsSlaveImpl].purgeStaleObjectsFromDatabase() - done!");
    // don't really care about any error
}

bool ClubsSlaveImpl::onConfigureReplication()
{
    return true;
}

bool ClubsSlaveImpl::onCompleteActivation()
{
    RedisError rc;
    // Fetch the BlazeIds of all online users who might be in a club. Because we only store MemberOnlineStatus data for
    // online users, this set should include all entries in Redis.
    BlazeIdSet blazeIds;
    gUserSessionManager->getOnlineClubsUsers(blazeIds);

    // Exit early if there are no users to look up
    if (blazeIds.empty())
        return true;

    eastl::vector<RedisResponsePtr> results;
    // getAllByKeys is deprecated: it cannot be executed reliably on Redis in cluster mode
    #pragma warning(suppress : 4996)
    rc = mOnlineStatusByClubIdByBlazeIdFieldMap.getAllByKeys(blazeIds, results, getConfig().getFetchMemberOnlineStatusPageSize());

    for (eastl::vector<RedisResponsePtr>::iterator itr = results.begin(); itr != results.end(); ++itr)
    {
        RedisResponsePtr resp = *itr;

        if (rc != REDIS_ERR_OK)
        {
            ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Unable to get MemberOnlineStatus for all users; RedisError: " << RedisErrorHelper::getName(rc));
            return false;
        }
        if (!resp->isArray())
        {
            ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Unable to get MemberOnlineStatus for all users; getAllByKeys returned unexpected response type (" << resp->getType() << ")");
            return false;
        }
        if (resp->getArraySize()%2 != 0)
        {
            ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Unable to get MemberOnlineStatus for all users; getAllByKeys returned odd number of results (" << resp->getArraySize() << 
                "). (Expected alternating BlazeId / club status map pairs)");
            return false;
        }

        for (uint32_t userIndex = 0; userIndex < resp->getArraySize(); userIndex+=2)
        {
            const RedisResponse& key = resp->getArrayElement(userIndex);
            if (!key.isString())
            {
                ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Error loading MemberOnlineStatus: unexpected key type (" << key.getType() << ")");
                return false;
            }
            const char8_t* end = key.getString() + key.getStringLen();
            const char8_t* begin = end;
            // strip out the CollectionKey prefix
            for ( ; begin != key.getString(); --begin)
            {
                if (*begin == REDIS_PAIR_DELIMITER)
                    break;
            }
            RedisDecoder rdecoder;
            BlazeId blazeId;
            if ( (begin == key.getString()) || (rdecoder.decodeValue(begin+1, end, blazeId) == nullptr) )
            {
                ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Error loading MemberOnlineStatus: failed to decode key: " << key.getString());
                return false;
            }

            const RedisResponse& statusByClubIdElement = resp->getArrayElement(userIndex+1);
            if (!statusByClubIdElement.isArray())
            {
                ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Error loading MemberOnlineStatus: got unexpected entry type (" << statusByClubIdElement.getType() << ") for user " << blazeId);
                return false;
            }
            if (statusByClubIdElement.getArraySize()%2 != 0)
            {
                ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Error loading MemberOnlineStatus: got odd number of entries (" << resp->getArraySize() << ") for user " << blazeId <<
                    "(Expected alternating ClubId/status pairs)");
                return false;
            }

            for (uint32_t clubIndex = 0; clubIndex < statusByClubIdElement.getArraySize(); ++clubIndex)
            {
                ClubId clubId;
                MemberOnlineStatusValue status;

                if (!statusByClubIdElement.extractArrayElement(clubIndex, clubId))
                {
                    ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Error loading MemberOnlineStatus: failed to decode ClubId for user " << blazeId);
                    return false;
                }
                if (!statusByClubIdElement.extractArrayElement(++clubIndex, status))
                {
                    ASSERT_LOG("[ClubsSlaveImpl].onCompleteActivation(): Error loading MemberOnlineStatus: failed to decode MemberOnlineStatus for user " << blazeId << " in club " << clubId);
                    return false;
                }

                updateLocalOnlineStatusAndCounts(clubId, blazeId, static_cast<MemberOnlineStatus>(status));
            }
        }
    }

    return true;
}

MemberOnlineStatus ClubsSlaveImpl::getMemberOnlineStatus(
    const BlazeId blazeId, const ClubId clubId)
{
    OnlineStatusForUserPerClubCache::iterator statusItr = mOnlineStatusForUserPerClubCache.find(blazeId);
    if (statusItr != mOnlineStatusForUserPerClubCache.end())
    {
        OnlineStatusInClub *statusInClub = statusItr->second;
        OnlineStatusInClub::const_iterator onlStatusIter = statusInClub->find(clubId);
        if (onlStatusIter != statusInClub->end())
        {
            return onlStatusIter->second;
        }
    }

    return CLUBS_MEMBER_OFFLINE;
}

void ClubsSlaveImpl::getMemberOnlineStatus(
    const BlazeIdList& blazeIdList,
    const ClubId clubId,
    MemberOnlineStatusMapForClub& statusMap)
{
    for (BlazeIdList::const_iterator i = blazeIdList.begin(); i != blazeIdList.end(); ++i)
    {
        BlazeId blazeId = *i;
        OnlineStatusForUserPerClubCache::iterator statusItr = mOnlineStatusForUserPerClubCache.find(blazeId);
        if (statusItr != mOnlineStatusForUserPerClubCache.end())
        {
            OnlineStatusInClub *statusInClub = statusItr->second;
            OnlineStatusInClub::const_iterator onlStatusIter = statusInClub->find(clubId);
            if (onlStatusIter != statusInClub->end())
            {
                statusMap[blazeId] = onlStatusIter->second;
            }
            else
            {
                statusMap[blazeId] = CLUBS_MEMBER_OFFLINE;
            }
        }
        else
        {
            statusMap[blazeId] = CLUBS_MEMBER_OFFLINE;
        }
    }
}

bool ClubsSlaveImpl::allowOnlineStatusOverride() const
{
    // On platforms like xone, the XRs required that caller should not be able to see the a club member as 'online' if blocked by that member. 
    // So we check if the calling user session is on xone
    if (gCurrentUserSession == nullptr)
    {
        WARN_LOG("[ClubsSlaveImpl].allowOnlineStatusOverride: This should only be called with a valid user session.");
        return false;
    }
    
    return ((gCurrentUserSession->getClientPlatform() == xone) || (gCurrentUserSession->getClientPlatform() == xbsx));
}

void ClubsSlaveImpl::overrideOnlineStatus(MemberOnlineStatusMapByExternalId& statusMap)
{
    if (gCurrentUserSession == nullptr || !allowOnlineStatusOverride())
    {
        return;
    }

    ExternalXblAccountId requestorExtId = gCurrentUserSession->getPlatformInfo().getExternalIds().getXblAccountId();
    if (requestorExtId == INVALID_XBL_ACCOUNT_ID)
    {
        return;
    }

    const BlazeId requestorId = gCurrentUserSession->getBlazeId();

    if (requestorId != INVALID_BLAZE_ID && statusMap.size() > 0)
    {
        Blaze::OAuth::GetUserXblTokenRequest req;
        Blaze::OAuth::GetUserXblTokenResponse rsp;

        BlazeRpcError err = ERR_SYSTEM;

        req.getRetrieveUsing().setPersonaId(requestorId);

        Blaze::OAuth::OAuthSlave *oAuthComponent = (Blaze::OAuth::OAuthSlave*) gController->getComponent(
            Blaze::OAuth::OAuthSlave::COMPONENT_ID, false, true);
        if (oAuthComponent == nullptr)
        {
            ERR_LOG("[ClubsSlaveImpl].overrideOnlineStatus: OAuthSlave was unavailable, overrideOnlineStatus failed.");
            return;
        }
        else
        {
            err = oAuthComponent->getUserXblToken(req, rsp);
            if (err != ERR_OK)
            {
                ERR_LOG("[ClubsSlaveImpl].overrideOnlineStatus: Failed to retrieve xbl auth token from service, for BlazeId id: " << requestorId << ", with error '" << ErrorHelp::getErrorName(err) <<"'.");
                return;
            }
        }

        XBLPrivacy::PermissionCheckBatchRequest permissionCheckReq;
        permissionCheckReq.setXuid(requestorExtId);
        permissionCheckReq.getPermissionCheckBatchRequestHeader().setAuthToken(rsp.getXblToken());
        permissionCheckReq.getPermissionCheckBatchRequestHeader().setServiceVersion("1"); 

        XBLPrivacy::PermissionCheckBatchRequestBody& body = permissionCheckReq.getPermissionCheckBatchRequestBody();
        body.getPermissions().push_back("ViewTargetPresence");


        for (MemberOnlineStatusMapByExternalId::const_iterator it = statusMap.begin(), itEnd = statusMap.end(); it != itEnd; ++it)
        {
            ExternalId otherExtId = it->first;
           
            if (otherExtId == INVALID_EXTERNAL_ID || otherExtId == requestorExtId)
                continue;
            
            eastl::string extIdStr;
            extIdStr.sprintf("%" PRId64, otherExtId);

            XBLPrivacy::User* user = body.getUsers().pull_back();
            user->setXuid(extIdStr.c_str());
        }

        if (body.getUsers().size() == 0)
        {
            TRACE_LOG("[ClubsSlaveImpl].overrideOnlineStatus: No user to check against. Return early.");
            return;
        }

        XBLPrivacy::PermissionCheckBatchResponse permissionCheckResponse;
        XBLPrivacy::XBLPrivacyConfigsSlave * xblPrivacyConfigsSlave = (Blaze::XBLPrivacy::XBLPrivacyConfigsSlave *)Blaze::gOutboundHttpService->getService(Blaze::XBLPrivacy::XBLPrivacyConfigsSlave::COMPONENT_INFO.name);
        if (xblPrivacyConfigsSlave == nullptr)
        {
            ERR_LOG("[ClubsSlaveImpl].overrideOnlineStatus: Failed to instantiate XBLPrivacyConfigsSlave object. ExternalId: " << requestorExtId << ".");
            return;
        }

        err = xblPrivacyConfigsSlave->permissionCheckBatch(permissionCheckReq, permissionCheckResponse);
        if (err == XBLPRIVACY_AUTHENTICATION_REQUIRED)
        {
            TRACE_LOG("[ClubsSlaveImpl].overrideOnlineStatus: Authentication with Microsoft failed, token may have expired. Forcing token refresh and retrying.");
            req.setForceRefresh(true);
            err = oAuthComponent->getUserXblToken(req, rsp);
            if (err == ERR_OK)
            {
                permissionCheckReq.getPermissionCheckBatchRequestHeader().setAuthToken(rsp.getXblToken());
                err = xblPrivacyConfigsSlave->permissionCheckBatch(permissionCheckReq, permissionCheckResponse);
            }
        }

        if (err != ERR_OK)
        {
            ERR_LOG("[ClubsSlaveImpl].overrideOnlineStatus: Failed to get permissions. externalId: " << requestorExtId << ", with error '" << ErrorHelp::getErrorName(err) << "'.");
            return;
        }

        for (XBLPrivacy::PermissionCheckBatchUserResponseList::const_iterator userRespIter = permissionCheckResponse.getResponses().begin(), userRespEnd = permissionCheckResponse.getResponses().end(); userRespIter != userRespEnd; ++userRespIter)
        {
            ExternalId userXuid = EA::StdC::AtoI64((*userRespIter)->getUser().getXuid());
            if ((*userRespIter)->getPermissions().size() > 0)  
            {
                // We should have only 1 entry
                XBLPrivacy::PermissionCheckResponseList::const_iterator respIter = (*userRespIter)->getPermissions().begin();
                if (!(*respIter)->getIsAllowed())
                {
                    statusMap[userXuid] = CLUBS_MEMBER_OFFLINE;
                }
            }
        }
    }
}

void ClubsSlaveImpl::overrideOnlineStatus(ClubMemberList& memberList)
{
    if (!allowOnlineStatusOverride())
        return;
   
    MemberOnlineStatusMapByExternalId statusMap;

    for (ClubMemberList::iterator it = memberList.begin(); it != memberList.end(); ++it)
    {
        ExternalId extId = getExternalIdFromPlatformInfo((*it)->getUser().getPlatformInfo());
        statusMap[extId] = (*it)->getOnlineStatus();
    }

    overrideOnlineStatus(statusMap);

    for (ClubMemberList::iterator it = memberList.begin(); it != memberList.end(); ++it)
    {
        ExternalId extId = getExternalIdFromPlatformInfo((*it)->getUser().getPlatformInfo());
        (*it)->setOnlineStatus(static_cast<MemberOnlineStatus>(statusMap[extId]));
    }
}

BlazeRpcError ClubsSlaveImpl::getClubMembers(
    DbConnPtr& dbConn, 
    ClubId clubId,
    uint32_t maxResultCount,
    uint32_t offset,
    MemberOrder orderType,
    OrderMode orderMode,
    MemberTypeFilter memberType,
    bool skipCalcDbRows,
    const char8_t* personaNamePattern,
    Clubs::GetMembersResponse &response)
{
    ClubMemberList &list = response.getClubMemberList();
    uint32_t totalCount = 0;

    if (personaNamePattern != nullptr && personaNamePattern[0] != '\0')
    {
        if (!getConfig().getEnableSearchByPersonaName())
        {
            return Blaze::CLUBS_ERR_FEATURE_NOT_ENABLED;
        }
    }
    else
    {
        personaNamePattern = nullptr;
    }    

    ClubsDatabase clubsDb;
    clubsDb.setDbConn(dbConn);

    BlazeRpcError error = clubsDb.getClubMembers(
        clubId,
        maxResultCount,
        offset,
        list,
        orderType,
        orderMode,
        memberType,
        personaNamePattern,
        skipCalcDbRows ? nullptr : &totalCount);


    if (error == Blaze::ERR_OK)
    {
        response.setTotalCount(totalCount);

        if (list.size() == 0)
        {
            // no results
            return error;
        }

        // resolve identities
        Blaze::BlazeIdVector blazeIds;
        Blaze::BlazeIdToUserInfoMap idMap;

        for (ClubMemberList::iterator it = list.begin(); it != list.end(); ++it)
            blazeIds.push_back((*it)->getUser().getBlazeId());

        error = gUserSessionManager->lookupUserInfoByBlazeIds(blazeIds, idMap, false);

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[getClubMembers] User identity provider returned an error (" << ErrorHelp::getErrorName(error) << ")");
            return error;
        }

        for (ClubMemberList::iterator it = list.begin(); it != list.end(); ++it)
        {
            Blaze::BlazeIdToUserInfoMap::const_iterator itm = idMap.find((*it)->getUser().getBlazeId());
            if (itm != idMap.end())
            {
                UserInfo::filloutUserCoreIdentification(*itm->second, (*it)->getUser());
            }
            // set online status
            (*it)->setOnlineStatus(getMemberOnlineStatus((*it)->getUser().getBlazeId(), clubId));
        }
    }
    else if (error != Blaze::CLUBS_ERR_INVALID_CLUB_ID)
    {
        ERR_LOG("[getClubMembers] Database error (" << ErrorHelp::getErrorName(error) << ")");
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief getStatusInfo

    Called by controller during getStatus RPC 

*/
/*************************************************************************************************/
void ClubsSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    ComponentStatus::InfoMap& map = status.getInfo();
    char8_t buffer[64];

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfFindClubsTimeMsHighWatermark.get());
    map[mPerfFindClubsTimeMsHighWatermark.getName().c_str()] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfGetNewsTimeMsHighWatermark.get());
    map[mPerfGetNewsTimeMsHighWatermark.getName().c_str()] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfMessages.getTotal());
    map["TotalMessagesExchanged"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfClubGamesFinished.getTotal());
    map["LastMinuteClubGamesFinished"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfAdminActions.getTotal());
    map["TotalAdminActions"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfJoinsLeaves.getTotal());
    map["TotalMembersJoinedLeft"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfMemberLogins.getTotal());
    map["TotalMemberLogins"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfMemberLogouts.getTotal());
    map["TotalMemberLogouts"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfOnlineClubs.get());
    map["TotalOnlineClubs"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfOnlineMembers.get());
    map["TotalOnlineMembers"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfClubsCreated.getTotal());
    map["TotalClubsCreated"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfInactiveClubsDeleted.getTotal());
    map["TotalPurgedClubs"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfUsersInClubs.get());
    map["TotalClubMemberships"] = buffer;

    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, mPerfClubs.get());
    map["TotalClubs"] = buffer;

    mPerfClubsCreated.iterate([this, &map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addDomainMetric(map, "TotalClubsCreated", tagList, value.getTotal());
        });

    mPerfInactiveClubsDeleted.iterate([this, &map](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addDomainMetric(map, "TotalPurgedClubs", tagList, value.getTotal());
        });

    mPerfUsersInClubs.iterate([this, &map](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) {
            addDomainMetric(map, "TotalClubMemberships", tagList, value.get());
        });

    mPerfClubs.iterate([this, &map](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) {
            addDomainMetric(map, "TotalClubs", tagList, value.get());
        });
}

void ClubsSlaveImpl::addDomainMetric(ComponentStatus::InfoMap& map, const char8_t* metricName, const Metrics::TagPairList& tagList, uint64_t value) const
{
    // Format the metric key, stripping whitespace from the domain name
    char name[256];
    name[sizeof(name)-1] = '\0';
    int32_t len = blaze_snzprintf(name, sizeof(name), "%s_", metricName);

    const char8_t* domainName = tagList[0].second.c_str();
    size_t idx = len;
    for(; (*domainName != '\0') && (idx < sizeof(name)); ++domainName)
    {
        if (!isspace(*domainName))
            name[idx++] = *domainName;
    }
    if (idx < sizeof(name))
        name[idx] = '\0';

    char8_t buffer[64];
    blaze_snzprintf(buffer, sizeof(buffer), "%" PRIu64, value);
    map[name] = buffer;
}

MemberOnlineStatus ClubsSlaveImpl::getOnlineStatus(BlazeId blazeId) const
{
    MemberOnlineStatus status = CLUBS_MEMBER_OFFLINE;

    if (gUserSessionManager->isUserOnline(blazeId))
    {
        UserSessionId primarySessionId = gUserSessionManager->getPrimarySessionId(blazeId);
        if (UserSession::isGameplayUser(primarySessionId))
        {
            status = CLUBS_MEMBER_ONLINE_INTERACTIVE;
        }
        else
        {
            status = CLUBS_MEMBER_ONLINE_NON_INTERACTIVE;
        }
    }
    return status;
}

/*! ********************************************************************************************************************************/
/*! \brief Update the local mOnlineStatusForUserPerClubCache and mOnlineStatusCountsForClub maps

    \param[in] clubId  The club in which a user's status has changed
    \param[in] blazeId  The user whose status has changed
    \param[in] status  The new status of the user identified by blazeId, in the club identified by clubId
    \param[in] allowOverwriteWithNoninteractive  Whether to allow a user's existing status (if he's not offline) to be overwritten by ONLINE_NON_INTERACTIVE
    \param[in] deleteOnlineStatusInClub  Whether to, when a user goes offline, delete his entry in the mOnlineStatusForUserPerClub map

    \return  true if the update changed any maps, false otherwise
***********************************************************************************************************************************/
bool ClubsSlaveImpl::updateLocalOnlineStatusAndCounts(ClubId clubId, BlazeId blazeId, MemberOnlineStatus status, bool allowOverwriteWithNoninteractive /*= true*/, bool deleteOnlineStatusInClub /*= true*/)
{
    TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts: blazeId(" << blazeId << "), clubId(" << clubId <<"), status(" << MemberOnlineStatusToString(status) << ")");
    bool statusUpdated = false;
    if (status == CLUBS_MEMBER_OFFLINE)
    {
        // User is offline; remove entry from local maps

        OnlineStatusForUserPerClubCache::iterator userItr = mOnlineStatusForUserPerClubCache.find(blazeId);
        if (userItr != mOnlineStatusForUserPerClubCache.end())
        {
            OnlineStatusInClub::iterator statusItr = userItr->second->find(clubId);
            if (statusItr != userItr->second->end())
            {
                statusUpdated = true;
                MemberOnlineStatus currentStatus = statusItr->second;

                TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts removing status count entry for blazeId (" << blazeId << ") in clubId(" << clubId << ").");

                // Remove this entry from the local map of BlazeId to ClubIds to MemberOnlineStatus
                if (deleteOnlineStatusInClub)
                {
                    userItr->second->erase(statusItr);
                    if (userItr->second->empty())
                    {
                        TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts removing last status count entry for blazeId (" << blazeId << ").");
                        delete userItr->second;
                        mOnlineStatusForUserPerClubCache.erase(userItr);
                    }
                }

                // Decrement the appropriate entry in the local map of ClubId to MemberOnlineStatuses to user count
                OnlineStatusCountsForClub::iterator clubItr = mOnlineStatusCountsForClub.find(clubId);
                if (clubItr != mOnlineStatusCountsForClub.end())
                {
                    if ((*clubItr->second)[CLUBS_TOTAL_STATUS_COUNT_INDEX] > 0)
                    {
                        --(*clubItr->second)[CLUBS_TOTAL_STATUS_COUNT_INDEX];
                        --(*clubItr->second)[currentStatus];
                    }

                    if ((*clubItr->second)[CLUBS_TOTAL_STATUS_COUNT_INDEX] == 0)
                    {
                        TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts removing status counts entry for clubId(" << clubId << ").");
                        delete clubItr->second;
                        mOnlineStatusCountsForClub.erase(clubItr);
                    }
                }
                else
                {
                    ERR_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts removed status count entry for blazeId (" << blazeId << ") in clubId(" << clubId <<
                        "), but did not find entry for this club in map of status counts.");
                }
            }
            else
            {
                TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts: Ignoring status update to OFFLINE for blazeId (" << blazeId << ") in clubId(" << clubId << 
                    "); did not find entry for this user and club.");
            }
        }
        else
        {
            TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts: Ignoring status update to OFFLINE for blazeId (" << blazeId << ") in clubId(" << clubId << 
                "); did not find entry for this user.");
        }
    }
    else
    {
        // User is not offline; add or update entry in local maps
        MemberOnlineStatus currentStatus = CLUBS_MEMBER_OFFLINE;

        // Add or update this entry in the local map of BlazeId to ClubIds to MemberOnlineStatus
        OnlineStatusForUserPerClubCache::iterator userItr = mOnlineStatusForUserPerClubCache.find(blazeId);
        if (userItr == mOnlineStatusForUserPerClubCache.end())
        {
            // We currently don't have a status for any club
            statusUpdated = true;

            OnlineStatusInClub* statusInClub = BLAZE_NEW OnlineStatusInClub();
            (*statusInClub)[clubId] = status;
            mOnlineStatusForUserPerClubCache[blazeId] = statusInClub;

            TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts added first new online status(" << MemberOnlineStatusToString(status) << " for user(" << blazeId << ") in club(" << clubId << ").");
        }
        else
        {
            OnlineStatusInClub::iterator statusItr = userItr->second->find(clubId);
            if (statusItr == userItr->second->end())
            {
                // We currently don't have a status for this club
                statusUpdated = true;
                OnlineStatusInClub* statusInClub = userItr->second;
                (*statusInClub)[clubId] = status;

                TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts added new online status(" << MemberOnlineStatusToString(status) << " for user(" << blazeId << ") in club(" << clubId << ").");
            }
            else
            {
                // We already have a status for this club
                currentStatus = statusItr->second;
                if (status != currentStatus)
                {
                    if (status == CLUBS_MEMBER_ONLINE_NON_INTERACTIVE && !allowOverwriteWithNoninteractive)
                    {
                        TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts: Ignoring status update to ONLINE_NON_INTERACTIVE for blazeId (" << blazeId << ") in clubId(" << clubId << ").");
                    }
                    else
                    {
                        statusUpdated = true;
                        statusItr->second = status;

                        TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts updated online status from " << MemberOnlineStatusToString(currentStatus) << " to " << MemberOnlineStatusToString(status) << " for user(" << blazeId << ") in club(" << clubId << ").");
                    }
                }
                else
                {
                    TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts: Ignoring status update to " << MemberOnlineStatusToString(status) << " for blazeId (" << blazeId << ") in clubId(" << clubId << 
                        "); update status matches current status.");
                }
            }
        }

        // Increment/decrement the appropriate entries in the local map of ClubId to MemberOnlineStatuses to user count
        if (statusUpdated)
        {
            OnlineStatusCountsForClub::iterator clubItr = mOnlineStatusCountsForClub.find(clubId);
            if (clubItr == mOnlineStatusCountsForClub.end())
            {
                if (currentStatus != CLUBS_MEMBER_OFFLINE)
                {
                    ERR_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts found no entry for club( " << clubId << ") in map of status counts, but user(" << blazeId << ") had status " << MemberOnlineStatusToString(currentStatus));
                    currentStatus = CLUBS_MEMBER_OFFLINE; // set currentStatus to OFFLINE so we don't decrement any entry in the freshly-inserted OnlineStatusCounts
                }
                TRACE_LOG("[ClubsSlaveImpl].updateLocalOnlineStatusAndCounts added new online status count entry for clubId(" << clubId << ").");
                clubItr = mOnlineStatusCountsForClub.insert(eastl::make_pair(clubId, BLAZE_NEW OnlineStatusCounts((uint32_t)CLUBS_MEMBER_ONLINE_STATUS_COUNT))).first;
            }

            if (currentStatus != CLUBS_MEMBER_OFFLINE)
                --(*clubItr->second)[currentStatus];   // decrement counts for our old status, if we had one
            else
                ++(*clubItr->second)[CLUBS_TOTAL_STATUS_COUNT_INDEX];  // this user was previously offline; increment the total count of online users

            ++(*clubItr->second)[status];  // increment counts for our new status
        }
    }
    return statusUpdated;
}

RedisError ClubsSlaveImpl::broadcastOnlineStatusChange(const NotifyUpdateMemberOnlineStatus& request, MemberOnlineStatus status, bool refreshLocalOnlineStatusCache /*= false*/, bool confirmNoninteractive /*= false*/)
{
    RedisError rc = REDIS_ERR_OK;
    if (request.getUserLoggedOff())
    {
        rc = mOnlineStatusByClubIdByBlazeIdFieldMap.erase(request.getBlazeId());
    }
    else if (status == CLUBS_MEMBER_OFFLINE)
    {
        rc = mOnlineStatusByClubIdByBlazeIdFieldMap.erase(request.getBlazeId(), request.getClubIds().asVector());
    }
    else
    {
        bool doUpdate = true;
        if (confirmNoninteractive && status == CLUBS_MEMBER_ONLINE_NON_INTERACTIVE)
        {
            // A user's status must not be updated from ONLINE_INTERACTIVE to ONLINE_NONINTERACTIVE unless he's logged out but still has one or more non-interactive sessions.
            // Thus there are only two allowable paths for such an update (this isn't one of them):
            //    (1) Direct from doOnUserSessionLogout (confirmNoninteractive would be false)
            //    (2) When updating the local cache to match redis after receiving an UpdateOnlineStatus notification
            MemberOnlineStatusValue redisStatus = CLUBS_MEMBER_OFFLINE;
            // confirmNoninteractive is only set to true from updateOnlineStatusOnExtendedDataChange, which always updates just one club
            ClubId clubId = *(request.getClubIds().asVector().cbegin());
            rc = mOnlineStatusByClubIdByBlazeIdFieldMap.find(request.getBlazeId(), clubId, redisStatus);
            doUpdate = (rc == REDIS_ERR_NOT_FOUND) || (rc == REDIS_ERR_OK && static_cast<MemberOnlineStatus>(redisStatus) != CLUBS_MEMBER_ONLINE_INTERACTIVE); 
        }

        if (doUpdate)
            rc = mOnlineStatusByClubIdByBlazeIdFieldMap.upsert(request.getBlazeId(), request.getClubIds().asVector(), status);
    }

    if (rc != REDIS_ERR_OK)
    {
        ERR_LOG("[ClubsSlaveImpl].updateAndBroadcastMemberOnlineStatus for " << (request.getUserLoggedOff() ? "logged off " : "") << "user(" << request.getBlazeId() << "): failed to update user status to "
            << MemberOnlineStatusToString(status) << "; Redis error: " << RedisErrorHelper::getName(rc));

        // If anything went wrong, we want to ensure that all local maps at least reflect what's currently in Redis. 
        // Since we always update Redis to match any change to our local maps, this prevents us from skipping
        // future Redis updates when our cache seems to already have them.
        handleOnlineStatusUpdate(request.clone(), false /*invalidateClubMemberInfoCache*/); // Our local ClubMemberInfoCache has already been cleared
    }
    else if (refreshLocalOnlineStatusCache)
    {
        // refreshLocalOnlineStatusCache will be true if we suspect that there is an in-flight UpdateMemberOnlineStatus notification, which may have
        // arrived while we were blocked in an earlier redis call. We don't send UpdateMemberOnlineStatus notifications to ourselves, so we call
        // handleOnlineStatusUpdate here to ensure that our local caches contain the online status that we've now set in redis.
        TRACE_LOG("[ClubsSlaveImpl].updateAndBroadcastMemberOnlineStatus for " << (request.getUserLoggedOff() ? "logged off " : "") << "user(" << request.getBlazeId() << "): refreshing local online status cache");
        handleOnlineStatusUpdate(request.clone(), false /*invalidateClubMemberInfoCache*/);
    }

    sendUpdateOnlineStatusNotificationToRemoteSlaves(&request);

    return rc;
}

/*! ************************************************************************************************/
/*! \brief return the local, or remote stats slave component
***************************************************************************************************/
Stats::StatsSlave* ClubsSlaveImpl::getStatsSlave(BlazeRpcError* waitResult) const
{
    // allow getting remote slaves (require local = false).
    BlazeRpcError err = ERR_OK;
    Component* statsComponent = gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false,
        true, &err);

    if (waitResult != nullptr)
        *waitResult = err;

    // pre: is type StatsSlave here.
    return static_cast<Stats::StatsSlave*>(statsComponent);
}

/*! \brief grab the StatCategorySummary from list */
const Blaze::Stats::StatCategorySummary* ClubsSlaveImpl::findInStatCategoryList(const char8_t* categoryName,
    const Blaze::Stats::StatCategoryList::StatCategorySummaryList& categories) const
{
    if ((categoryName == nullptr) || (categoryName[0] == '\0'))
        return nullptr;

    for (Blaze::Stats::StatCategoryList::StatCategorySummaryList::const_iterator iter = categories.begin(); iter != categories.end(); ++iter)
    {
        if (blaze_strcmp(categoryName, (*iter)->getName()) == 0)
            return *iter;
    }
    return nullptr;
}

/*! \brief return whether list has scope name */
bool ClubsSlaveImpl::findInStatKeyScopeList(const char8_t* keyScopeName,
    const Blaze::Stats::StatCategorySummary::StringScopeNameList& keyScopeList) const
{
    if ((keyScopeName == nullptr) || (keyScopeName[0] == '\0'))
        return false;

    for (Blaze::Stats::StatCategorySummary::StringScopeNameList::const_iterator iter = keyScopeList.begin(); iter != keyScopeList.end(); ++iter)
    {
        if (blaze_strcmp(keyScopeName, (*iter).c_str()) == 0)
            return true;
    }
    return false;
}

/*! \brief grab the StatDescSummary from list */
const Blaze::Stats::StatDescSummary* ClubsSlaveImpl::findInStatDescList(const char8_t* statName,
    const Blaze::Stats::StatDescs::StatDescSummaryList& statList) const
{
    if ((statName == nullptr) || (statName[0] == '\0'))
        return nullptr;

    for (Blaze::Stats::StatDescs::StatDescSummaryList::const_iterator iter = statList.begin(); iter != statList.end(); ++iter)
    {
        if (blaze_strcmp(statName, (*iter)->getName()) == 0)
            return *iter;
    }
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////// CensusDataProvider interface ///////////////////////////////////////////
Blaze::BlazeRpcError ClubsSlaveImpl::getCensusData(CensusDataByComponent& censusData)
{
    // All platforms use the same values because Clubs does not support Crossplay at all. 
    BlazeRpcError error = Blaze::ERR_OK;

    ClubsCensusData* clbCensusData = BLAZE_NEW ClubsCensusData();

    clbCensusData->setNumOfClubMembers((uint32_t)mPerfUsersInClubs.get());
    clbCensusData->setNumOfClubs((uint32_t)mPerfClubs.get());
    clbCensusData->setNumOfOnlineClubMembers((uint32_t)mPerfOnlineMembers.get());
    clbCensusData->setNumOfOnlineClubs((uint32_t)mPerfOnlineClubs.get());

    for (auto& domainItr : mClubDomains)
    {
        (clbCensusData->getNumOfClubMembersByDomain())[domainItr.first] = (uint32_t)mPerfUsersInClubs.get({ { Metrics::Tag::club_domain, domainItr.second->getDomainName() } });
        (clbCensusData->getNumOfClubsByDomain())[domainItr.first] = (uint32_t)mPerfClubs.get({ {Metrics::Tag::club_domain, domainItr.second->getDomainName() } });
    }

    if (censusData.find(getComponentId()) == censusData.end())
    {
        censusData[getComponentId()] = censusData.allocate_element();
    }
    censusData[getComponentId()]->set(clbCensusData);

    return error;
}

/*************************************************************************************************/
/* PetitionableContentProvider implementation */

BlazeRpcError ClubsSlaveImpl::fetchContent(const EA::TDF::ObjectId& bobjId, Collections::AttributeMap& attributeMap, eastl::string& url)
{
    if (bobjId.type != ENTITY_TYPE_CLUBNEWS)
        return Blaze::ERR_SYSTEM;

    url.clear();
        
    ClubsDbConnector dbConn(this);
    dbConn.acquireDbConnection(true);
    ClubServerNews news;
    ClubId clubId = INVALID_CLUB_ID;
    BlazeRpcError result = dbConn.getDb().getServerNews(bobjId.toObjectId<NewsId>(), clubId, news);

    if (result == Blaze::ERR_OK)
    {
        BlazeId id = news.getContentCreator();
        UserInfoPtr info;
        BlazeRpcError lookupError = gUserSessionManager->lookupUserInfoByBlazeId(id, info);

        attributeMap["Creator"] = (lookupError == Blaze::ERR_OK) ? info->getPersonaName() : "Unknown";
        attributeMap["ClubNewsText"] = news.getText();
    }
    
    dbConn.releaseConnection();
        
    return result;
}

BlazeRpcError ClubsSlaveImpl::showContent(const EA::TDF::ObjectId& bobjId, bool visible)
{    
    if (bobjId.type != ENTITY_TYPE_CLUBNEWS)
        return Blaze::ERR_SYSTEM;
    
    ClubsDbConnector dbConn(this);
    dbConn.acquireDbConnection(false);
    ClubServerNews news;

    // check if news item exists
    ClubId clubId = INVALID_CLUB_ID;
    BlazeRpcError result = dbConn.getDb().getServerNews(bobjId.toObjectId<NewsId>(), clubId, news);

    if (result == Blaze::ERR_OK)
    {

        ClubNewsFlags newsFlagsMask;
        newsFlagsMask.setCLUBS_NEWS_HIDDEN_BY_TOS_VIOLATION();
        ClubNewsFlags newsFlagsValue;
        
        if (!visible)
            newsFlagsValue.setCLUBS_NEWS_HIDDEN_BY_TOS_VIOLATION();
        
        result = dbConn.getDb().updateNewsItemFlags(bobjId.toObjectId<NewsId>(), newsFlagsMask, newsFlagsValue);
        // news exists so the error means it was already in requested state
        if (result == Blaze::CLUBS_ERR_NEWS_ITEM_NOT_FOUND)
            result = Blaze::ERR_OK;
    }    
    
    dbConn.releaseConnection();
    return result;
}

/////////////////  UserSetProvider interface functions  /////////////////////////////
BlazeRpcError ClubsSlaveImpl::getSessionIds(const EA::TDF::ObjectId& bobjId, UserSessionIdList& ids)
{
    BlazeRpcError result = ERR_SYSTEM;
    WARN_LOG("[ClubsSlaveImpl].getSessionIds - Not Implemented!");
    return result;
}

BlazeRpcError ClubsSlaveImpl::getUserBlazeIds(const EA::TDF::ObjectId& bobjId, BlazeIdList& ids)
{
    BlazeRpcError result = ERR_OK;

    const EA::TDF::ObjectType bobjType = bobjId.type;
    if (bobjType == ENTITY_TYPE_CLUB)
    {
        ClubId clubId = bobjId.toObjectId<ClubId>();
        ClubsDbConnector dbc(this);
        if (dbc.acquireDbConnection(true) != nullptr)
        {
            ClubMemberList memberList;
            result = dbc.getDb().getClubMembers(clubId, static_cast<uint32_t>(-1), 0, memberList);
            dbc.releaseConnection();

            if (result != ERR_OK)
            {
                WARN_LOG("[ClubsSlaveImpl].getUserBlazeIds: The indicated club (id - " << clubId << ") does not exist");
            }
            else
            {
                ids.reserve(memberList.size());
                for (ClubMemberList::const_iterator itr = memberList.begin(); itr != memberList.end(); ++itr)
                    ids.push_back((*itr)->getUser().getBlazeId());
            }
        }
        else
        {
            WARN_LOG("[ClubsSlaveImpl].getUserBlazeIds: The indicated club (id - " << clubId << ") does not exist");
            result = CLUBS_ERR_INVALID_CLUB_ID;
        }
    }
    else
    {
        WARN_LOG("[ClubsSlaveImpl].getUserBlazeIds: The indicated object (type - " << bobjType.toString().c_str() << ") is not a club");
        result = CLUBS_ERR_INVALID_ARGUMENT;
    }
    return result;
}

BlazeRpcError ClubsSlaveImpl::getUserIds(const EA::TDF::ObjectId& bobjId, UserIdentificationList& ids)
{
    BlazeRpcError result = ERR_OK;

    const EA::TDF::ObjectType bobjType = bobjId.type;
    if (bobjType == ENTITY_TYPE_CLUB)
    {
        ClubId clubId = bobjId.toObjectId<ClubId>();
        ClubsDbConnector dbc(this);
        if (dbc.acquireDbConnection(true) != nullptr)
        {
            ClubMemberList memberList;
            result = dbc.getDb().getClubMembers(clubId, static_cast<uint32_t>(-1), 0, memberList);
            dbc.releaseConnection();

            if (result != ERR_OK)
            {
                WARN_LOG("[ClubsSlaveImpl].getUserIds: The indicated club (id - " << clubId << ") does not exist");
            }
            else
            {
                ids.reserve(memberList.size());
                for (ClubMemberList::const_iterator itr = memberList.begin(); itr != memberList.end(); ++itr)
                {
                    UserIdentificationPtr blazeId = ids.pull_back();
                    blazeId->setBlazeId((*itr)->getUser().getBlazeId());
                    (*itr)->getUser().getPlatformInfo().copyInto(blazeId->getPlatformInfo());
                    blazeId->setExternalId(getExternalIdFromPlatformInfo(blazeId->getPlatformInfo()));
                    blazeId->setName((*itr)->getUser().getName());
                    blazeId->setPersonaNamespace((*itr)->getUser().getPersonaNamespace());
                }
            }
        }
        else
        {
            WARN_LOG("[ClubsSlaveImpl].getUserIds: The indicated club (id - " << clubId << ") does not exist");
            result = CLUBS_ERR_INVALID_CLUB_ID;
        }
    }
    else
    {
        WARN_LOG("[ClubsSlaveImpl].getUserIds: The indicated object (type - " << bobjType.toString().c_str() << ") is not a club");
        result = CLUBS_ERR_INVALID_ARGUMENT;
    }
    return result;
}

BlazeRpcError ClubsSlaveImpl::countSessions(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    BlazeRpcError result = ERR_SYSTEM;
    WARN_LOG("[ClubsSlaveImpl].countSessions - Not Implemented!");
    return result;
}

BlazeRpcError ClubsSlaveImpl::countUsers(const EA::TDF::ObjectId& bobjId, uint32_t& count)
{
    BlazeIdList ids;
    BlazeRpcError result = getUserBlazeIds(bobjId, ids);
    if (result == ERR_OK)
        count = ids.size();
    return result;
}

void ClubsSlaveImpl::onScheduledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName)
{
    if (blaze_strcmp(taskName, INACTIVE_CLUB_PURGE_TASKNAME) == 0)
        mInactiveClubPurgeTaskId = taskId;
    else if (blaze_strcmp(taskName, FETCH_COMPONENTINFO_TASKNAME) == 0)
        mUpdateComponentInfoTaskId = taskId;
}

void ClubsSlaveImpl::onExecutedTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName)
{
    if (mInactiveClubPurgeTaskId == taskId)
    {
        purgeStaleObjectsFromDatabase();
    }
    else if (mUpdateComponentInfoTaskId == taskId)
    {
        takeComponentInfoSnapshot();
    }
}

void ClubsSlaveImpl::onCanceledTask(const EA::TDF::Tdf* tdf, TaskId taskId, const char8_t* taskName)
{
    if (blaze_strcmp(taskName, INACTIVE_CLUB_PURGE_TASKNAME) == 0)
        mInactiveClubPurgeTaskId = INVALID_TASK_ID;
    else if (blaze_strcmp(taskName, FETCH_COMPONENTINFO_TASKNAME) == 0)
        mUpdateComponentInfoTaskId = INVALID_TASK_ID;
}

BlazeRpcError ClubsSlaveImpl::resetStadiumName(ClubId clubId, eastl::string& statusMessage)
{
    ClubMetadata clubMetadata;
    BlazeRpcError outError;

    outError = getClubMetadata(clubId, clubMetadata);
    if (Blaze::ERR_OK == outError)
    {        
        //A typical club metadata looks like
        //<StadName value= "El Alcoraz" /><KitId value="11370496" />...

        eastl::string xmlStr = clubMetadata.getMetaDataUnion().getMetadataString();
        eastl::string stadiumNameResetStr = xmlStr;
        size_t replaceStatrtPos = xmlStr.find(sStadNameElementLeading, 0);
       
        if (replaceStatrtPos != eastl::string::npos)
        {
            size_t replaceEndPos = xmlStr.find(sStadNameElementEnding, replaceStatrtPos);

            if (replaceEndPos != eastl::string::npos)
            {
                eastl::string leadingStr = xmlStr.substr(0, replaceStatrtPos + sStadNameElementLeading.length());
                eastl::string endingStr = xmlStr.substr(replaceEndPos, eastl::string::npos);

                //replace the  stadium name
                //final metadata string
                stadiumNameResetStr = leadingStr;
                stadiumNameResetStr.append(sResetStadName);
                stadiumNameResetStr.append(endingStr);
            }
            else
            {
                outError = CLUBS_ERR_CORRUPTED_CLUB_METADATA;
                ERR_LOG("[ClubsSlaveImpl].resetStadiumName(): club's metadata might be corrupted:   " << clubId << " data: " << xmlStr);
            }
        }
        else
        {
            outError = CLUBS_ERR_CORRUPTED_CLUB_METADATA;
            ERR_LOG("[ClubsSlaveImpl].resetStadiumName(): club's metadata might be corrupted:   " << clubId << " data: " << xmlStr);
        }
        clubMetadata.getMetaDataUnion().setMetadataString(stadiumNameResetStr.c_str());


        DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
        if (dbConn != nullptr)
        {
            ClubsDatabase clubDb;
            clubDb.setDbConn(dbConn);

            outError = clubDb.updateMetaData(clubId, CLUBS_METADATA_UPDATE, clubMetadata.getMetaDataUnion(), false);
        }
        else
        {
            ASSERT_LOG("[ClubsSlaveImpl].resetStadiumName() no dbConn specified!");
            outError = ERR_SYSTEM;
        }
    }
    else
    {
        statusMessage = "Failed to fetch meta data of the club";
        ERR_LOG("[ClubsSlaveImpl].resetStadiumName(): failed to fetch club metadata  " << clubId << " with error code: " << outError);
    }

    return outError;
}


BlazeRpcError ClubsSlaveImpl::fetchStadiumName(ClubId clubId, eastl::string &stadiumName)
{
    ClubMetadata clubMetadata;
    BlazeRpcError outError = getClubMetadata(clubId, clubMetadata);
    
    if (Blaze::ERR_OK == outError)
    {
        stadiumName.clear();

        //parse out stadium name
        size_t stadiumNameStartPos, stadiumNameEndPos;        
        eastl::string xmlStr = clubMetadata.getMetaDataUnion().getMetadataString();

        stadiumNameStartPos = xmlStr.find(sStadNameElementLeading, 0);
        if (stadiumNameStartPos != eastl::string::npos)
        {
            stadiumNameEndPos = xmlStr.find(sStadNameElementEnding, stadiumNameStartPos);

            if (stadiumNameEndPos != eastl::string::npos)
            {
                stadiumNameStartPos += sStadNameElementLeading.length();

                stadiumName = xmlStr.substr(stadiumNameStartPos, stadiumNameEndPos - stadiumNameStartPos);
                stadiumName.trim();
            }
            else
            {
                outError = CLUBS_ERR_CORRUPTED_CLUB_METADATA;
                ERR_LOG("[ClubsSlaveImpl].resetStadiumName(): club's metadata might be corrupted:   " << clubId << " data: " << xmlStr);
            }
        } 
        else
        {
            outError = CLUBS_ERR_CORRUPTED_CLUB_METADATA;
            ERR_LOG("[ClubsSlaveImpl].resetStadiumName(): club's metadata might be corrupted:   " << clubId << " data: " << xmlStr);
        }
    }
    else
    {
        ERR_LOG("[ClubsSlaveImpl].fetchStadiumName(): failed to fetch club metadata  " << clubId << " with error code: " << outError);
    }
    return outError;
}

BlazeRpcError ClubsSlaveImpl::getClubMetadata(ClubId clubId, ClubMetadata& clubMetadata)
{
    BlazeRpcError outError;

    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
    if (dbConn != nullptr)
    {
        ClubsDatabase clubDb;        

        clubDb.setDbConn(dbConn);

        outError = clubDb.getClubMetadata(clubId, clubMetadata);       
    }
    else
    {
        ASSERT_LOG("[ClubsSlaveImpl].getClubMetadata() no dbConn specified!");
        outError = ERR_SYSTEM;
    }

    return outError;
}

} // Clubs
} // Blaze
