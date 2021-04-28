/*************************************************************************************************/
/*!
    \file   gamereportexpression.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "gametype.h"


namespace Blaze
{
namespace GameReporting
{

GRECacheReport::~GRECacheReport()
{
    free();
}

GRECacheReport::StatUpdate::StatUpdate(const GameType& gameType, const GameTypeReportConfig::StatUpdate& statUpdateConfig)
{
    mCategory = BLAZE_NEW GameReportExpression(gameType, statUpdateConfig.getCategoryExpr());
    mCategory->parse();
    mEntityId = BLAZE_NEW GameReportExpression(gameType, statUpdateConfig.getEntityId());
    mEntityId->parse();
    mKeyscopeIndex = BLAZE_NEW GameReportExpression(gameType, statUpdateConfig.getKeyscopeIndex());
    mKeyscopeIndex->parse();
    mCondition = BLAZE_NEW GameReportExpression(gameType, statUpdateConfig.getCondition());
    mCondition->parse();

    for (GameTypeReportConfig::StatUpdate::StatsMap::const_iterator citStat = statUpdateConfig.getStats().begin(), citStatEnd = statUpdateConfig.getStats().end();
            citStat != citStatEnd;
            ++citStat)
    {
        const GameTypeReportConfig::StatUpdate::Stat *stat = citStat->second;
        mStats[citStat->first] = BLAZE_NEW Stat(gameType, *stat);
    }
    for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citKey = statUpdateConfig.getKeyscopes().begin(), citKeyEnd = statUpdateConfig.getKeyscopes().end();
            citKey != citKeyEnd;
            ++citKey
            )
    {
        mKeyscopes[citKey->first] = BLAZE_NEW GameReportExpression(gameType, citKey->second.c_str());
        mKeyscopes[citKey->first]->parse();
    }
}

GRECacheReport::StatUpdate::~StatUpdate()
{
    for (Stats::const_iterator citStat = mStats.begin(), citStatEnd = mStats.end();
            citStat != citStatEnd;
            ++citStat)
    {
        Stat *stat = citStat->second;
        delete stat;
    }
    for (Keyscopes::const_iterator citKey = mKeyscopes.begin(), citKeyEnd = mKeyscopes.end(); citKey != citKeyEnd; ++citKey)
    {
        delete citKey->second;
    }
    delete mCategory;
    delete mEntityId;
    delete mKeyscopeIndex;
    delete mCondition;
}

GRECacheReport::StatUpdate::Stat::Stat(const GameType& gameType, const GameTypeReportConfig::StatUpdate::Stat& statConfig)
{
    mValue = BLAZE_NEW GameReportExpression(gameType, statConfig.getValue());
    mValue->parse();
    mCondition = BLAZE_NEW GameReportExpression(gameType, statConfig.getCondition());
    mCondition->parse();
}

GRECacheReport::StatUpdate::Stat::~Stat()
{
    delete mValue;
    delete mCondition;
}

GRECacheReport::StatsServiceUpdate::StatsServiceUpdate(const GameType& gameType, const GameTypeReportConfig::StatsServiceUpdate& statsServiceUpdateConfig)
{
    mEntityId = BLAZE_NEW GameReportExpression(gameType, statsServiceUpdateConfig.getEntityId());
    mEntityId->parse();
    mCondition = BLAZE_NEW GameReportExpression(gameType, statsServiceUpdateConfig.getCondition());
    mCondition->parse();

    for (GameTypeReportConfig::StatsServiceUpdate::DimensionalStatsMap::const_iterator citDimStatList = statsServiceUpdateConfig.getDimensionalStats().begin(),
         citDimStatListEnd = statsServiceUpdateConfig.getDimensionalStats().end(); citDimStatList != citDimStatListEnd; ++citDimStatList)
    {
        DimensionalStatList* greDimStatList =  BLAZE_NEW DimensionalStatList();
        
        for (EA::TDF::TdfStructVector<GameTypeReportConfig::StatsServiceUpdate::DimensionalStat>::const_iterator citDimStat = citDimStatList->second->begin(),
            citDimStatEnd = citDimStatList->second->end(); citDimStat != citDimStatEnd; ++citDimStat)
        {
            DimensionalStat *dimensionalStat = BLAZE_NEW DimensionalStat(gameType, *(*citDimStat));
            greDimStatList->push_back(dimensionalStat);
        }
        mDimensionalStats[citDimStatList->first] = greDimStatList;
    }

    for (GameTypeReportConfig::StatsServiceUpdate::StatsMap::const_iterator citStat = statsServiceUpdateConfig.getStats().begin(), citStatEnd = statsServiceUpdateConfig.getStats().end();
        citStat != citStatEnd;
        ++citStat)
    {
        const GameTypeReportConfig::StatsServiceUpdate::Stat *stat = citStat->second;
        mStats[citStat->first] = BLAZE_NEW Stat(gameType, *stat);
    }
}

GRECacheReport::StatsServiceUpdate::~StatsServiceUpdate()
{
    for (DimensionalStats::const_iterator citDimStat = mDimensionalStats.begin(), citDimStatEnd = mDimensionalStats.end(); citDimStat != citDimStatEnd; ++citDimStat)
    {
        DimensionalStatList* dimStatList = citDimStat->second;
        for (DimensionalStatList::const_iterator citDimStatList = dimStatList->begin(), citDimStatListEnd = dimStatList->end(); 
            citDimStatList != citDimStatListEnd; ++citDimStatList)
        {
            DimensionalStat *dimensionalStat = *citDimStatList;
            delete dimensionalStat;
        }
        delete dimStatList;
    }
    for (Stats::const_iterator citStat = mStats.begin(), citStatEnd = mStats.end();
        citStat != citStatEnd;
        ++citStat)
    {
        Stat *stat = citStat->second;
        delete stat;
    }
    delete mEntityId;
    delete mCondition;
}

GRECacheReport::StatsServiceUpdate::Stat::Stat(const GameType& gameType, const GameTypeReportConfig::StatsServiceUpdate::Stat& statConfig)
{
    mValue = BLAZE_NEW GameReportExpression(gameType, statConfig.getValue());
    mValue->parse();
    mCondition = BLAZE_NEW GameReportExpression(gameType, statConfig.getCondition());
    mCondition->parse();
}
GRECacheReport::StatsServiceUpdate::Stat::~Stat()
{
    delete mValue;
    delete mCondition;
}
GRECacheReport::StatsServiceUpdate::DimensionalStat::DimensionalStat(const GameType& gameType, const GameTypeReportConfig::StatsServiceUpdate::DimensionalStat& dimStatConfig)
{
    mValue = BLAZE_NEW GameReportExpression(gameType, dimStatConfig.getValue());
    mValue->parse();
    mCondition = BLAZE_NEW GameReportExpression(gameType, dimStatConfig.getCondition());
    mCondition->parse();
    
    for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citKey = dimStatConfig.getDimensions().begin(),
        citKeyEnd = dimStatConfig.getDimensions().end(); citKey != citKeyEnd; ++citKey)
    {
        mDimensions[citKey->first] = BLAZE_NEW GameReportExpression(gameType, citKey->second.c_str());
        mDimensions[citKey->first]->parse();
    }
}
GRECacheReport::StatsServiceUpdate::DimensionalStat::~DimensionalStat()
{
    delete mValue;
    delete mCondition;
    
    for (Dimensions::const_iterator citKey = mDimensions.begin(), citKeyEnd = mDimensions.end(); citKey != citKeyEnd; ++citKey)
    {
        delete citKey->second;
    }
}
GRECacheReport::GameHistory::GameHistory(const GameType& gameType, const GameTypeReportConfig::GameHistory& gameHistoryConfig)
{
    for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator cit = gameHistoryConfig.getColumns().begin(), citEnd = gameHistoryConfig.getColumns().end();
            cit != citEnd;
            ++cit)
    {
        mColumns[cit->first] = BLAZE_NEW GameReportExpression(gameType, cit->second.c_str());
        mColumns[cit->first]->parse();
    }
}

GRECacheReport::GameHistory::~GameHistory()
{
    for (Columns::const_iterator citColumn = mColumns.begin(), citColumnEnd = mColumns.end(); citColumn != citColumnEnd; ++citColumn)
    {
        delete citColumn->second;
    }
}

GRECacheReport::Event::Event(const GameType& gameType, const GameTypeReportConfig::Event& eventConfig)
{
    mEntityId = BLAZE_NEW GameReportExpression(gameType, eventConfig.getEntityId());
    mEntityId->parse();

    mTdfMemberName = BLAZE_NEW GameReportExpression(gameType, eventConfig.getTdfMemberName());
    mTdfMemberName->parse();

    for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator cit = eventConfig.getEventData().begin(),
         citEnd = eventConfig.getEventData().end(); 
         cit != citEnd; ++cit)
    {
        mEventData[cit->first] = BLAZE_NEW GameReportExpression(gameType, cit->second.c_str());
        mEventData[cit->first]->parse();
    }
}

GRECacheReport::Event::~Event()
{
    delete mEntityId;
    delete mTdfMemberName;

    for (EventData::iterator cit = mEventData.begin(), citEnd = mEventData.end(); 
         cit != citEnd; ++cit)
    {
        delete cit->second;
    }
    mEventData.clear();
}

GRECacheReport::AchievementUpdates::AchievementUpdates(const GameType& gameType, const GameTypeReportConfig::AchievementUpdates& achieveConfig)
{
    mEntityId = BLAZE_NEW GameReportExpression(gameType, achieveConfig.getEntityId());
    mEntityId->parse();
    for (GameTypeReportConfig::AchievementUpdates::AchievementList::const_iterator citAchieve = achieveConfig.getAchievements().begin(), citAchieveEnd = achieveConfig.getAchievements().end();
        citAchieve != citAchieveEnd;
        ++citAchieve)
    {
        Achievement *achievement = BLAZE_NEW Achievement(gameType, **citAchieve);
        mAchievements[(*citAchieve)->getId()] = achievement;
    }
    for (GameTypeReportConfig::AchievementUpdates::EventList::const_iterator citEvent = achieveConfig.getEventList().begin(), citEventEnd = achieveConfig.getEventList().end();
        citEvent != citEventEnd;
        ++citEvent)
    {
        Event *event = BLAZE_NEW Event(gameType, **citEvent);
        mEvents.push_back(event);
    }
}

GRECacheReport::AchievementUpdates::~AchievementUpdates()
{
    for (Achievements::const_iterator citAchieve = mAchievements.begin(), citAchieveEnd = mAchievements.end();
        citAchieve != citAchieveEnd;
        ++citAchieve)
    {
        delete citAchieve->second;
    }
    for (Events::const_iterator citEvent = mEvents.begin(), citEventEnd = mEvents.end();
        citEvent != citEventEnd;
        ++citEvent)
    {
        delete *citEvent;
    }
    delete mEntityId;
}

GRECacheReport::AchievementUpdates::Achievement::Achievement(const GameType& gameType, const GameTypeReportConfig::AchievementUpdates::Achievement& achieveConfig)
{
    mPoints = BLAZE_NEW GameReportExpression(gameType, achieveConfig.getPoints());
    mPoints->parse();
    mCondition = BLAZE_NEW GameReportExpression(gameType, achieveConfig.getCondition());
    mCondition->parse();
}

GRECacheReport::AchievementUpdates::Achievement::~Achievement()
{
    delete mPoints;
    delete mCondition;
}

GRECacheReport::AchievementUpdates::Event::Event(const GameType& gameType, const GameTypeReportConfig::AchievementUpdates::Event& eventConfig)
{
    for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citEventData = 
        eventConfig.getEventData().begin(), citEventDataEnd = eventConfig.getEventData().end();
        citEventData != citEventDataEnd; ++citEventData)
    {
        mData[citEventData->first] = BLAZE_NEW GameReportExpression(gameType, citEventData->second);
        mData[citEventData->first]->parse();
    }
}

GRECacheReport::AchievementUpdates::Event::~Event()
{
    for (EventData::const_iterator citEventData = mData.begin(), citEventDataEnd = mData.end();
        citEventData != citEventDataEnd; ++citEventData)
    {
        delete citEventData->second;
    }
}

GRECacheReport::PsnMatchUpdate::PsnMatchUpdate(const GameType& gameType, const GameTypeReportConfig::PsnMatchUpdate& psnMatchUpdateConfig)
{
    mMatchResults = BLAZE_NEW MatchResults(gameType, psnMatchUpdateConfig.getMatchResults());

    for (const auto& matchStat : psnMatchUpdateConfig.getMatchStats())
    {
        mMatchStats[matchStat.first] = BLAZE_NEW GameReportExpression(gameType, matchStat.second->getVal());
        mMatchStats[matchStat.first]->parse();
    }
}

GRECacheReport::PsnMatchUpdate::~PsnMatchUpdate()
{
    delete mMatchResults;

    for (const auto& matchStat : mMatchStats)
    {
        delete matchStat.second;
    }
}

GRECacheReport::PsnMatchUpdate::MatchResults::MatchResults(const GameType& gameType, const GameTypeReportConfig::PsnMatchUpdate::MatchResults& matchResults)
{
    mCooperativeResult = BLAZE_NEW GameReportExpression(gameType, matchResults.getCooperativeResult());
    mCooperativeResult->parse();

    mCompetitiveResult = BLAZE_NEW CompetitiveResult(gameType, matchResults.getCompetitiveResult());
}

GRECacheReport::PsnMatchUpdate::MatchResults::~MatchResults()
{
    delete mCooperativeResult;

    delete mCompetitiveResult;
}

GRECacheReport::PsnMatchUpdate::MatchResults::CompetitiveResult::CompetitiveResult(const GameType& gameType, const GameTypeReportConfig::PsnMatchUpdate::MatchResults::CompetitiveResult& compResult)
{
    mEntityId = BLAZE_NEW GameReportExpression(gameType, compResult.getEntityId());
    mEntityId->parse();
    mScore = BLAZE_NEW GameReportExpression(gameType, compResult.getScore());
    mScore->parse();
}

GRECacheReport::PsnMatchUpdate::MatchResults::CompetitiveResult::~CompetitiveResult()
{
    delete mEntityId;
    delete mScore;
}

const GRECacheReport* GRECacheReport::find(const GameTypeReportConfig* target) const
{
    if (target == mReportConfig)
        return this;

    for (SubReports::const_iterator cit = mSubReports.begin(); cit != mSubReports.end(); ++cit)
    {
        const GRECacheReport* child = (*cit).second;
        const GRECacheReport *result = child->find(target);
        if (result != nullptr)
            return result;
    }

    return nullptr;
}

bool GRECacheReport::build(const GameType& gameType, const GameTypeReportConfig *config)
{
    if (mPlayerId != nullptr)
        return false;

    mPlayerId = BLAZE_NEW GameReportExpression(gameType, config->getPlayerId());
    if (!mPlayerId->parse())
        return false;

    for (GameTypeReportConfig::StatUpdatesMap::const_iterator cit = config->getStatUpdates().begin(), citEnd = config->getStatUpdates().end();
            cit != citEnd;
            ++cit)
    {
        StatUpdate* greStatUpdate = BLAZE_NEW StatUpdate(gameType, *cit->second);
        mStatUpdates[cit->first] = greStatUpdate;
    }
    for (GameTypeReportConfig::StatsServiceUpdatesMap::const_iterator cit = config->getStatsServiceUpdates().begin(), citEnd = config->getStatsServiceUpdates().end();
            cit != citEnd;
            ++cit)
    {
        StatsServiceUpdate* greStatsServiceUpdate = BLAZE_NEW StatsServiceUpdate(gameType, *cit->second);
        mStatsServiceUpdates[cit->first] = greStatsServiceUpdate;
    }
    for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator cit = config->getReportValues().begin(), citEnd = config->getReportValues().end();
            cit != citEnd;
            ++cit)
    {
        mReportValues[cit->first] = BLAZE_NEW GameReportExpression(gameType, cit->second.c_str());
        if (!mReportValues[cit->first]->parse())
            return false;
    }
    for (GameTypeReportConfig::GameHistoryList::const_iterator cit = config->getGameHistory().begin(), citEnd = config->getGameHistory().end();
            cit != citEnd;
            ++cit)
    {
        GameHistory *gameHistory = BLAZE_NEW GameHistory(gameType, *(*cit));
        mGameHistory.push_back(gameHistory);
    }
    for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator cit = config->getMetricUpdates().begin(), citEnd = config->getMetricUpdates().end();
            cit != citEnd;
            ++cit)
    {
        mMetricUpdates[cit->first] = BLAZE_NEW GameReportExpression(gameType, cit->second.c_str());
        mMetricUpdates[cit->first]->parse();
    }
    for (GameTypeReportConfig::EventUpdatesMap::const_iterator cit = config->getEventUpdates().begin(), citEnd = config->getEventUpdates().end();
            cit != citEnd;
            ++cit)
    {
        Event* reportEvent = BLAZE_NEW Event(gameType, *cit->second);
        mEventUpdates[cit->first] = reportEvent;
    }

    mAchievementUpdates = BLAZE_NEW AchievementUpdates(gameType, config->getAchievementUpdates());

    for (const auto& mapUpdate : config->getPsnMatchUpdates())
    {
        PsnMatchUpdate* grePsnMatchUpdate = BLAZE_NEW PsnMatchUpdate(gameType, *mapUpdate.second);
        mPsnMatchUpdates[mapUpdate.first] = grePsnMatchUpdate;
    }

    for (GameTypeReportConfig::SubreportsMap::const_iterator cit = config->getSubreports().begin(), citEnd = config->getSubreports().end(); cit != citEnd; ++cit)
    {
        GRECacheReport* greCache = BLAZE_NEW GRECacheReport();
        if (!greCache->build(gameType, cit->second))
            return false;
        mSubReports[cit->first] = greCache;
    }

    mReportConfig = config;

    return true;
}

void GRECacheReport::free()
{
    if (mPlayerId != nullptr)
    {
        delete mPlayerId;
        mPlayerId = nullptr;
    }

    for (StatUpdates::const_iterator cit = mStatUpdates.begin(), citEnd = mStatUpdates.end(); cit != citEnd; ++cit)
    {
        delete cit->second;
    }
    mStatUpdates.clear();

    for (StatsServiceUpdates::const_iterator cit = mStatsServiceUpdates.begin(), citEnd = mStatsServiceUpdates.end(); cit != citEnd; ++cit)
    {
        delete cit->second;
    }
    mStatsServiceUpdates.clear();

    for (ReportValues::const_iterator citRV = mReportValues.begin(), citRVEnd = mReportValues.end(); citRV != citRVEnd; ++citRV)
    {
        delete citRV->second;
    }
    mReportValues.clear();

    for (GameHistoryList::const_iterator cit = mGameHistory.begin(), citEnd = mGameHistory.end(); cit != citEnd; ++cit)
    {
        delete (*cit);
    }
    mGameHistory.clear();

    for (MetricUpdates::const_iterator cit = mMetricUpdates.begin(), citEnd = mMetricUpdates.end(); cit != citEnd; ++cit)
    {
        delete cit->second;
    }
    mMetricUpdates.clear();

    for (EventUpdates::const_iterator cit = mEventUpdates.begin(), citEnd = mEventUpdates.end(); cit != citEnd; ++cit)
    {
        delete cit->second;
    }
    mEventUpdates.clear();

    for (SubReports::const_iterator cit = mSubReports.begin(), citEnd = mSubReports.end(); cit != citEnd; ++cit)
    {
        delete cit->second;
    }
    mSubReports.clear();

    if (mAchievementUpdates != nullptr)
    {
        delete mAchievementUpdates;
        mAchievementUpdates = nullptr;
    }

    for (const auto& mapUpdate : mPsnMatchUpdates)
    {
        delete mapUpdate.second;
    }
    mPsnMatchUpdates.clear();

    mReportConfig = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
void GameReportContext::setParent(const GameReportContext& owner) 
{
    parent = &owner;
    gameInfo = owner.gameInfo;
    tdf = nullptr;
    container = owner.container;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
bool ReportTdfVisitor::VisitorState::hasContextContainer(const GameReportContext& grContext) const
{
    if (!hasContainer())
        return false;

    if (!grContext.container.isValid())
        return false;

    if (containerType == LIST_CONTAINER && grContext.container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
    {
        if (container.vecBase == &grContext.container.value.asList())
            return true;
    }
    else if (containerType == MAP_CONTAINER && grContext.container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP)
    {
        if (container.mapBase == &grContext.container.value.asMap())
            return true;
    }
    
    return false;
}

ReportTdfVisitor::VisitorState* ReportTdfVisitor::VisitorStateStack::push(bool copyTop)
{
    if (size() < capacity())
    {
        if (!copyTop || empty())
        {
            return &push_back();
        }
        push_back(back());
    }
    else
    {
        // Limit capacity since we never want to resize the stack to prevent trashing references to stack elements kept by other methods.
        ERR_LOG("[ReportTdfVisitor].push() : STACK OVERFLOW!  Report hierarchy is deeper than allowed (capacity=" << capacity() << ")");
        return nullptr;
    }
    VisitorState& cur = back();

    //  state values to reset.
    cur.containerType = VisitorState::NO_CONTAINER;
    cur.container.mapBase = nullptr;
    cur.container.vecBase = nullptr;
    cur.isMapKey = false;

    return &cur;
}

void ReportTdfVisitor::VisitorStateStack::printTop() const
{
    INFO_LOG("[ReportTdfVisitor::VisitorStateStack].printTop : size=" << size());
}

/////////////////////////////////////////////////////////////////////////////////////////

ReportTdfVisitor::ReportTdfVisitor() 
{
    mVisitSuccess = true;
    mReportContexts.Init(64, nullptr, 0, &ReportTdfVisitor::allocCore, &ReportTdfVisitor::freeCore, (void *)this); 
}

ReportTdfVisitor::~ReportTdfVisitor()
{
}

void* ReportTdfVisitor::allocCore(size_t nSize, void *pContext)
{
    return BLAZE_ALLOC(nSize);
}

void ReportTdfVisitor::freeCore(void *pCore, void *pContext)
{
    BLAZE_FREE(pCore);
}

//  directly visit the tdf matching the passed in config's report EA::TDF::Tdf.
//  in most cases : tdfOut == *context.tdf
//  this means the report will act on itself, write to itself, etc.

//  Visit the TDF and match against the report config.
//  For this to work, the report config and the TDF hierarchy and class mappings must match
//  The class mappings are taken from the configuration::GameTypeReportConfig::reportTdf, and validated against
//  the visited GameReport TDF.
//  Any discrepancy will result in an error (mVisitSuccess will be set to FALSE.)
//
//  The visitor maintains a state stack that keeps track of the current configuration and report context.
//  Each visit() EA::TDF::TdfVisitor method implemented below uses the state pushed onto the stack by its caller.
//  
//  Those classes that override the ReportTdfVisitor pure virtual methods 
bool ReportTdfVisitor::visit(const GameTypeReportConfig& reportConfig, const GRECacheReport& greReportConfig, EA::TDF::Tdf& tdfOut, GameReportContext& context, EA::TDF::Tdf& tdfOutParent)
{
    // perform any necessary validation of the configuration against the reference TDF to call out discrepencies early.
    if (context.tdf == nullptr)
    {
        const char8_t* gameReportName = ((greReportConfig.mPlayerId != nullptr) && (greReportConfig.mPlayerId->getGameReportName() != nullptr)? greReportConfig.mPlayerId->getGameReportName() : "NULL");
        ERR_LOG("[ReportTdfVisitor].parseReportConfig() : No reference TDF, of expected class name " << reportConfig.getReportTdf() << " for this subreport. Game type: " << gameReportName);
        return false;
    }

    const EA::TDF::Tdf& referenceTdf = *context.tdf; 

    //  set current state to use the passed in report context as a parent - 
    //  the context is cloned and the supplied reference EA::TDF::Tdf is set at the context's current.
    VisitorState* pState = mStateStack.push();
    if (pState == nullptr)
        return false;
    VisitorState& state = *pState;
    state.context = &context;
    state.config = &reportConfig;
    state.greCache = &greReportConfig;

    //  validate TDF, then visit it to traverse the top level TDF members.   
    if (blaze_strcmp(state.config->getReportTdf(), referenceTdf.getFullClassName()) != 0)
    {
        ERR_LOG("[GameType].visit(): EA::TDF::Tdf " << referenceTdf.getFullClassName() << " does not match configured EA::TDF::Tdf " 
                << state.config->getReportTdf() << " for this subreport.");
        mVisitSuccess = false;
    }

    // ============================================================
    //  VISIT TOP-LEVEL TDF MEMBERS
    //  This visitor ignores any containers and Tdfs that could are
    //  subreports (which are handled below.)
    // ============================================================
    tdfOut.visit(*this, tdfOutParent, referenceTdf);   

    // ============================================================
    //  VISIT REPORT SECTIONS
    // ============================================================
    bool traverseChildReports = true;
    if (mVisitSuccess)
    {
        mVisitSuccess = visitReportSection(*state.config, *state.greCache, tdfOut, *state.context, traverseChildReports);
    }   

    // ============================================================
    //  VISIT SUBREPORTS
    //  (recursively calling into parseReportConfig)
    // ============================================================
    if (traverseChildReports)
    {
        GameTypeReportConfig::SubreportsMap::const_iterator cit = reportConfig.getSubreports().begin();
        GameTypeReportConfig::SubreportsMap::const_iterator citEnd = reportConfig.getSubreports().end();
        GRECacheReport::SubReports::const_iterator citGRE = greReportConfig.mSubReports.begin();
        GRECacheReport::SubReports::const_iterator citGREEnd = greReportConfig.mSubReports.end();

        for (; cit != citEnd && citGRE != citGREEnd && mVisitSuccess; ++cit, ++citGRE)
        {
            const GameTypeReportConfig& subreport = *cit->second;
            const GRECacheReport& greSubReport = *citGRE->second;
            const char* tdfMemberName = cit->first.c_str();

            const EA::TDF::TdfMemberInfo* refMemberInfo = nullptr, *memberInfoOut = nullptr;
            referenceTdf.getMemberInfoByName(tdfMemberName, refMemberInfo);
            tdfOut.getMemberInfoByName(tdfMemberName, memberInfoOut);
            if (refMemberInfo == nullptr || memberInfoOut == nullptr)
            {
                ERR_LOG("[GameType].parseReportConfig() : subreport " << tdfMemberName << " not found in report TDF " << referenceTdf.getFullClassName());
                mVisitSuccess = false;
                continue;
            }
            //  determine type
            EA::TDF::TdfGenericReferenceConst refTdfValue;
            if (!referenceTdf.getValueByTag(refMemberInfo->getTag(), refTdfValue))
            {
                ERR_LOG("[ReportTdfVisitor].parseReportConfig() : error retrieving value for subreport " << tdfMemberName << " not found in report TDF " 
                        << referenceTdf.getFullClassName());
                mVisitSuccess = false;
                continue;
            }

            //  this will not work with primitives - only with TdfGenericReferences that have a pointer to source data.
            //  the code below deals with only those types that are supported in any case.
            EA::TDF::TdfGenericReferenceConst tdfValueOut;
            if (!tdfOut.getValueByTag(refMemberInfo->getTag(), tdfValueOut))
            {
                ERR_LOG("[ReportTdfVisitor].parseReportConfig() : error retrieving value for subreport " << tdfMemberName 
                        << " not found in report outTDF " << referenceTdf.getFullClassName());
                mVisitSuccess = false;
                continue;
            }

            if (refTdfValue.getType() == EA::TDF::TDF_ACTUAL_TYPE_TDF || refTdfValue.getType() == EA::TDF::TDF_ACTUAL_TYPE_VARIABLE)
            {            
                //  create a new context for the TDF subreport and apply that context to the current state
                //  after visiting the TDF subreport, revert context back to the parent's.
                EA::TDF::Tdf* childTdfOut = (refTdfValue.getType() == EA::TDF::TDF_ACTUAL_TYPE_TDF) ? const_cast<EA::TDF::Tdf*>(&tdfValueOut.asTdf()) : const_cast<EA::TDF::Tdf*>(tdfValueOut.asVariable().get());
                EA::TDF::Tdf* refChildTdf = (refTdfValue.getType() == EA::TDF::TDF_ACTUAL_TYPE_TDF) ? const_cast<EA::TDF::Tdf*>(&refTdfValue.asTdf()) : const_cast<EA::TDF::Tdf*>(refTdfValue.asVariable().get());
                if (childTdfOut == nullptr || refChildTdf == nullptr)
                {
                    // If both are nullptr, then I guess it's okay
                    if (childTdfOut != refChildTdf)
                    {
                        ERR_LOG("[ReportTdfVisitor].parseReportConfig() : Incomplete variable Tdf value for subreport " << tdfMemberName 
                                << " found in report outTDF " << referenceTdf.getFullClassName() << " for " << (childTdfOut == nullptr ? "child " : "")  << (refChildTdf == nullptr ? "ref " : "") << ".");
                        mVisitSuccess = false;
                    }
                    continue;
                }
                
                GameReportContext* thisContext = state.context;
                GameReportContext& childContext = createReportContext(thisContext);
                childContext.tdf = refChildTdf;                
                childContext.container = thisContext->container;
                childContext.containerIteration = thisContext->containerIteration;
                state.context = &childContext;
                mVisitSuccess = visit(subreport, greSubReport, *childTdfOut, *state.context, tdfOut);
                state.context = thisContext;   
            }
            else if (refTdfValue.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP)
            {
                //  traverse the map
                if (refTdfValue.getTypeDescription().asMapDescription()->valueType.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_TDF)
                {                    
                    GameReportContext& childContext = createReportContext(state.context);
                    childContext.container.value  = refTdfValue;
                    if (refTdfValue.getTypeDescription().asMapDescription()->keyType.isIntegral())
                    {
                        childContext.container.Index.id = 0;
                    }
                    else if (refTdfValue.getTypeDescription().asMapDescription()->keyType.isString())
                    {
                        childContext.container.Index.str = nullptr;
                    }

                    VisitorState* pChildState = mStateStack.push();
                    if (pChildState != nullptr)
                    {
                        EA::TDF::TdfMapBase& tdfMapOut = const_cast<EA::TDF::TdfMapBase&>(tdfValueOut.asMap());
                        VisitorState& childState = *pChildState;
                        childState.context = &childContext;
                        childState.config = &subreport;
                        childState.greCache = &greSubReport;
                        childState.containerType = VisitorState::MAP_CONTAINER;
                        childState.container.mapBase = const_cast<EA::TDF::TdfMapBase*>(&refTdfValue.asMap());
                        childState.isMapKey = true;         // start visitation assuming first visit is going to be a key.
                        tdfMapOut.visitMembers(*this, tdfOut, tdfOut, refMemberInfo->getTag(), refTdfValue.asMap());
                        mStateStack.pop();
                    }                
                }
            }
            else if (refTdfValue.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
            {
                //  traverse the list
                if (refTdfValue.getTypeDescription().asListDescription()->valueType.getTdfType() == EA::TDF::TDF_ACTUAL_TYPE_TDF)
                {
                    GameReportContext& childContext = createReportContext(state.context);
                    childContext.container.value = refTdfValue;

                    VisitorState* pChildState = mStateStack.push();
                    if (pChildState != nullptr)
                    {
                        EA::TDF::TdfVectorBase& tdfVectorOut = const_cast<EA::TDF::TdfVectorBase&>(tdfValueOut.asList());
                        VisitorState& childState = *pChildState;
                        childState.context = &childContext;
                        childState.config = &subreport;
                        childState.greCache = &greSubReport;
                        childState.containerType = VisitorState::LIST_CONTAINER;
                        childState.container.vecBase = const_cast<EA::TDF::TdfVectorBase*>(&refTdfValue.asList());
                        tdfVectorOut.visitMembers(*this, tdfOut, tdfOut, refMemberInfo->getTag(), refTdfValue.asList());
                        mStateStack.pop();
                    }                
                }
            }
            else
            {
                //  unsupported subreport type - error out.
                ERR_LOG("ReportTdfVisitor].parseReportConfig() : subreport " << tdfMemberName << " n report TDF " << referenceTdf.getFullClassName() 
                        << " is unsupported for type=" << refTdfValue.getType());
            }
        }
    }

    mStateStack.pop();

    return mVisitSuccess;
}

bool ReportTdfVisitor::visitReportSection(const GameTypeReportConfig& reportConfig, const GRECacheReport& greReport, EA::TDF::Tdf& tdfOut, GameReportContext& context, bool& traverseChildReports)
{
    if (mVisitSuccess)
    {
        mVisitSuccess = visitSubreport(reportConfig, greReport, context, tdfOut, traverseChildReports);
    }

    //  visit report values.
    if (mVisitSuccess)
    {
        mVisitSuccess = visitReportValues(reportConfig.getReportValues(), greReport.mReportValues, context, tdfOut);
    }

    //  visit the map of stat updates for the subreport
    if (mVisitSuccess)
    {
        mVisitSuccess = visitStatUpdates(reportConfig.getStatUpdates(), greReport.mStatUpdates, context, tdfOut);
    }

    //  visit the map of stat updates for the subreport
    if (mVisitSuccess)
    {
        mVisitSuccess = visitStatsServiceUpdates(reportConfig.getStatsServiceUpdates(), greReport.mStatsServiceUpdates, context, tdfOut);
    }

    //  visit game history section
    if (mVisitSuccess)
    {
        mVisitSuccess = visitGameHistory(reportConfig.getGameHistory(), greReport.mGameHistory, context, tdfOut);
    }

    //  visit metric updates section
    if (mVisitSuccess)
    {
        mVisitSuccess = visitMetricUpdates(reportConfig.getMetricUpdates(), greReport.mMetricUpdates, context, tdfOut);
    }

    //  visit events section
    if (mVisitSuccess)
    {
        mVisitSuccess = visitEventUpdates(reportConfig.getEventUpdates(), greReport.mEventUpdates, context, tdfOut);
    }

    //  visit achievements section
    if (mVisitSuccess)
    {
        mVisitSuccess = visitAchievementUpdates(reportConfig.getAchievementUpdates(), *greReport.mAchievementUpdates, context, tdfOut);
    }

    //  visit PSN Match report update section
    if (mVisitSuccess)
    {
        mVisitSuccess = visitPsnMatchUpdates(reportConfig.getPsnMatchUpdates(), greReport.mPsnMatchUpdates, context, tdfOut);
    }

    return mVisitSuccess;
}

//  if visiting this TDF inside a container, then execute a new visit with config and context.
bool ReportTdfVisitor::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{
    //  set current state to use the passed in report context as a parent - 
    //  the context is cloned and the supplied reference EA::TDF::Tdf is set at the context's current.
    VisitorState& state = mStateStack.top();
    
    bool isContainerValue = state.containerType != VisitorState::NO_CONTAINER && !state.isMapKey;

    if (isContainerValue)
    {
        //  descend into TDF using config.
        //  establish that the current context is the reference TDF.   Note we're not creating a new context as this was done
        //  earlier when starting to traverse the container in the visit config method.
        state.context->tdf = &referenceValue;
        visit(*state.config, *state.greCache, value, *state.context, parentTdf);

        //  if this EA::TDF::Tdf is in a container and on the same level as the current context container, update the context's container information.
        //  TDFs and contexts may be on different levels. 
        if (state.hasContextContainer(*state.context))
        {
            if (state.context->container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST)
                state.context->containerIteration++;
            else if (state.context->container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP && !state.isMapKey)
                state.context->containerIteration++;
        }
        if (state.containerType == VisitorState::MAP_CONTAINER)
        {
            //  switch to key state.
            state.isMapKey = true;
        }
    }  

    return true;
}

//  Utils
//  Purpose is to update map keys if currently visiting a map 
void ReportTdfVisitor::updateVisitorStateForInt(uint64_t value, const uint64_t referenceValue)
{
    //  set current state to use the passed in report context as a parent - 
    //  the context is cloned and the supplied reference EA::TDF::Tdf is set at the context's current.
    VisitorState& state = mStateStack.top();    
    if (state.containerType == VisitorState::MAP_CONTAINER && state.isMapKey)
    {        
        //  if this EA::TDF::Tdf is in a container and on the same level as the current context container, update the context's container information.
        //  TDFs and contexts may be on different levels. 
        if (state.hasContextContainer(*state.context))
        {
            if (state.context->container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP && state.isMapKey)
            {
                if (state.context->container.value.getTypeDescription().asMapDescription()->keyType.isIntegral())
                    state.context->container.Index.id = value;
            }
        }
        updateVisitorState(value, referenceValue);
    }
}

//  Purpose is to update map keys if currently visiting a map 
void ReportTdfVisitor::updateVisitorStateForString(EA::TDF::TdfString& value, const EA::TDF::TdfString& referenceValue)
{
    //  set current state to use the passed in report context as a parent - 
    //  the context is cloned and the supplied reference EA::TDF::Tdf is set at the context's current.
    VisitorState& state = mStateStack.top();    
    if (state.containerType == VisitorState::MAP_CONTAINER && state.isMapKey)
    {        
        //  if this EA::TDF::Tdf is in a container and on the same level as the current context container, update the context's container information.
        //  TDFs and contexts may be on different levels. 
        if (state.hasContextContainer(*state.context))
        {
            if (state.context->container.value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP && state.isMapKey)
            {
                if (state.context->container.value.getTypeDescription().asMapDescription()->keyType.isString())
                    state.context->container.Index.str = value.c_str();         // EASTL assumption
            }
        }
        updateVisitorState(value, referenceValue);
    }  
}

//  updates common state within visit methods (primitives only for container management)
template < typename T > void ReportTdfVisitor::updateVisitorState(T& value, const T& referenceValue)
{
  //  set current state to use the passed in report context as a parent - 
    //  the context is cloned and the supplied reference EA::TDF::Tdf is set at the context's current.
    VisitorState& state = mStateStack.top();
    
    bool isContainerKey = state.containerType == VisitorState::MAP_CONTAINER && state.isMapKey;

    if (isContainerKey)
    {
        state.isMapKey = false;
    }
}

GameReportContext& ReportTdfVisitor::createReportContext(const GameReportContext* parent)
{
    GameReportContext& context = *(new(mReportContexts.Malloc()) GameReportContext);
    if (parent != nullptr)
    {
        //  copy data persisted between all contexts
        context.parent = parent;
        context.gameInfo = parent->gameInfo;
        context.dnfStatus = parent->dnfStatus;
        context.level = parent->level + 1;
    }
    return context;
}

void ReportTdfVisitor::flushReportContexts()
{
    mReportContexts.Reset();
}

} //namespace Blaze
} //namespace GameReporting
