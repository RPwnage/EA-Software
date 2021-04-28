/*************************************************************************************************/
/*!
    \file   reportparserutil.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "reportparserutil.h"

#include "framework/util/shared/blazestring.h"
#include "framework/controller/controller.h"

#include "stats/rpc/statsslave.h"
#include "stats/statsconfig.h"

#include "gamereporting/rpc/gamereporting_defines.h"
#include "gamereporting/tdf/gamehistory.h"

#include "gamereportexpression.h"
#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameReporting
{

namespace Utilities
{

///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  ReportParser implementation
//
///////////////////////////////////////////////////////////////////////////////////////////////////
ReportParser::ReportParser(const GameType& gameType, ProcessedGameReport& report, const GameManager::PlayerIdList* playerIds, const EventTypes* eventTypes) : 
    mGameType(gameType), mGameInfo(report.getGameInfo()), mDnfStatus(report.getDnfStatusMap()),
        mMetricsCollection(report.getMetricsCollection()), mEventTypes(eventTypes)
{
    mReportParseFlags = 0;
    mReportParseMode = REPORT_PARSE_NULL;
    mUpdateStatsRequestBuilder = nullptr;
    mGameHistoryReport = nullptr;
    mBlazeError = Blaze::ERR_OK;

    if (report.getReportType() == REPORT_TYPE_OFFLINE && !gameType.getArbitraryUserOfflineReportProcessing())
    {
        // For offline game reports, if arbitraryUserOfflineReportProcessing is false, disable stats and
        // game history updates for all users except the submitter(s)

        ignoreStatUpdatesForObjectType(ENTITY_TYPE_USER, true);
        ignoreGameHistoryForObjectType(ENTITY_TYPE_USER, true);

        for (GameManager::PlayerIdList::const_iterator iter = report.getSubmitterIds().begin(); iter != report.getSubmitterIds().end(); ++iter)
        {
            GameManager::PlayerId playerId = *iter;
            if (!UserSessionManager::isStatelessUser(playerId))
            {
                ignoreGameHistoryForEntityId(ENTITY_TYPE_USER, playerId, false);
                ignoreStatUpdatesForEntityId(ENTITY_TYPE_USER, playerId, false);
            }
        }
    }
    else if (playerIds != nullptr)
    {
        ignoreStatUpdatesForObjectType(ENTITY_TYPE_USER, true);
        ignoreGameHistoryForObjectType(ENTITY_TYPE_USER, true);

        // Enable stats and game history updates for non-stateless participants only

        for (GameManager::PlayerIdList::const_iterator iter = playerIds->begin(); iter != playerIds->end(); ++iter)
        {
            GameManager::PlayerId playerId = *iter;
            if (!UserSessionManager::isStatelessUser(playerId))
            {
                ignoreStatUpdatesForEntityId(ENTITY_TYPE_USER, playerId, false);
                ignoreGameHistoryForEntityId(ENTITY_TYPE_USER, playerId, false);
            }
        }
    }
}


ReportParser::~ReportParser()
{
    clear();
}


//  parses a EA::TDF::Tdf and extracts all information from a compliant EA::TDF::Tdf needed to inspect and report data.
//  if the EA::TDF::Tdf didn't parse, returns false.
bool ReportParser::parse(EA::TDF::Tdf& tdf, uint32_t parseFlags, const GameTypeReportConfig *config)
{
    mBlazeError = Blaze::ERR_OK;

    //  find the report config based on the report EA::TDF::Tdf.
    const GameTypeReportConfig& thisReportConfig = config != nullptr ? *config : mGameType.getConfig();
    const GRECacheReport* thisGRECache = mGameType.getGRECache().find(&thisReportConfig);
    if (thisGRECache == nullptr)
    {
        ERR_LOG("[ReportParser].parse() : No GRE Cache defined for the report configuration (gametype='" 
                << mGameType.getGameReportName().c_str() << "',tdf='" << thisReportConfig.getReportTdf() << "')");
        return false;
    }

    if (blaze_strcmp(thisReportConfig.getReportTdf(), tdf.getFullClassName()) != 0)
    {
        ERR_LOG("[ReportParser].parse() : For GameType='" << mGameType.getGameReportName().c_str() << "', report TDF class '" 
                << thisReportConfig.getReportTdf() << "' does not match the configuration '" << tdf.getFullClassName() << "'");
        return false;
    }

    //  begin parse.
    uint32_t curParseFlagMask = 0x00000001; 

    //  if parsing keyscopes, enforce parse of report values.
    if ((parseFlags & REPORT_PARSE_KEYSCOPES) != 0)
        parseFlags |= REPORT_PARSE_VALUES;   
    if ((parseFlags & REPORT_PARSE_VALUES) != 0)
        parseFlags |= REPORT_PARSE_METRICS;

    //  turn off options already executed in earlier calls;
    parseFlags = parseFlags & (~mReportParseFlags);   

    while (curParseFlagMask < REPORT_PARSE_ALL && mBlazeError == ERR_OK)
    {
        //  define the tdf's parsing context.
        GameReportContext& thisContext = createReportContext(nullptr);
        thisContext.tdf = &tdf;
        thisContext.gameInfo = mGameInfo;
        thisContext.dnfStatus = &mDnfStatus;

        uint32_t curParseFlag = (parseFlags & curParseFlagMask);

        if (curParseFlag)
        {
            //  validate whether it's possible to run the requested parse method

            if (curParseFlag == REPORT_PARSE_STATS)
            {
                if (mStatCategorySummaryList.size() == 0)
                {
                    BlazeRpcError err = ERR_SYSTEM;
                    Stats::StatsSlave* statsComp = (Blaze::Stats::StatsSlave*) gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false/*reqLocal*/, true, &err);
                    if (statsComp != nullptr)
                    {
                        BlazeRpcError rc = ERR_SYSTEM;
                        Blaze::Stats::StatCategoryList res;
                        
                        rc = statsComp->getStatCategoryList(res);          
                        if (rc == ERR_OK)
                        {
                            res.getCategories().copyInto(mStatCategorySummaryList);
                            mCategoryNameTypeMap.clear();
                            Stats::StatCategoryList::StatCategorySummaryList::const_iterator catIter = mStatCategorySummaryList.begin();
                            Stats::StatCategoryList::StatCategorySummaryList::const_iterator endIter = mStatCategorySummaryList.end();
                            while (catIter != endIter)
                            {
                                mCategoryNameTypeMap[(*catIter)->getName()] = (*catIter)->getEntityType();
                                ++catIter;
                            }
                        }
                        else
                        {
                            ERR_LOG("[ReportParser].parse() : getStatCategoryList RPC failed with error:" << rc);
                            return false;
                        }
                    }
                    else
                    {
                        ERR_LOG("[ReportParser].parse() : Failed to get the stats slave handle.");
                        return false;
                    }
                }
                //  validate that we've parsed data we need before parsing stats.
                if ((mReportParseFlags & REPORT_PARSE_VALUES)==0)
                {
                    ERR_LOG("[ReportParser].parse() : Failed to parse stats - value parsing is required first (current state flags=" 
                            << mReportParseFlags << ")");
                    return false;
                }
            }
            else if (curParseFlag == REPORT_PARSE_GAME_HISTORY)
            {
                if (mGameHistoryReport == nullptr)
                {
                    ERR_LOG("[ReportParser].parse() : Attempting to parse game history into an unspecified target GameHistoryReport. (current state flags=" << mReportParseFlags << ")");
                    return false;
                }
            }

            mReportParseMode = static_cast<ReportParseMode>(curParseFlag);    // guaranteed to be a valid parse mode.
            ReportTdfVisitor::visit(thisReportConfig, *thisGRECache, tdf, thisContext, tdf);
            mReportParseFlags |= curParseFlag;
        }
        else
        {
            mReportParseMode = REPORT_PARSE_NULL;
        }
        curParseFlagMask <<= 1;

        flushReportContexts();
    }

    return mBlazeError==ERR_OK;
}


//  clears all data parsed from the EA::TDF::Tdf.   
void ReportParser::clear()
{
    if (mUpdateStatsRequestBuilder != nullptr)
    {
        mUpdateStatsRequestBuilder = nullptr;
    }

    if (mGameHistoryReport != nullptr)
    {
        mGameHistoryReport = nullptr;
    }

    // this should wipe out all data since all constituent members are objects that perform their own garbage collection.
    mKeyscopesMap.clear();     
    mReportParseFlags = 0;

    TableRowMapByLevel::iterator rowMapIter, rowMapEnd;
    rowMapIter = mTraversedHistoryData.begin();
    rowMapEnd = mTraversedHistoryData.end();

    for (; rowMapIter != rowMapEnd; ++rowMapIter)
    {
        rowMapIter->second.release();
    }

    mTraversedHistoryData.clear();

    mEvents.clear();

#ifdef TARGET_achievements
    mGrantAchievementRequests.clear();
    mPostEventsRequests.clear();
#endif
}

//  visit the top level subreport config for the player id
bool ReportParser::visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports)
{
    if (subReportConfig.getPlayerId()[0] != '\0')
    {
        GameReportExpression& expr = *greSubReport.mPlayerId;        
        EA::TDF::TdfGenericValue value;
        if (!expr.evaluate(value, context))
        {
            ERR_LOG("[ReportParser].visitSubreport : GameType '" << mGameType.getGameReportName().c_str() << "', error parsing player ID value.");
            return false;
        }

        BlazeId playerId;
        EA::TDF::TdfGenericReference refPlayerId(playerId);
        if (!value.convertToIntegral(refPlayerId))
        {
            ERR_LOG("[ReportParser].visitSubreport : GameType '" << mGameType.getGameReportName().c_str() << "', error parsing player ID value."
                    " Type BlazeId (uint64_t) required, type " << (value.getFullName() ? value.getFullName() : "(unknown)") << " found." );
            return false;
        }
        
        if (mPlayerIdSet.find(playerId) == mPlayerIdSet.end())
        {
            mPlayerIdSet.insert(playerId);
        }
    }
    return true;
}

//  visit the map of stat updates for the subreport
bool ReportParser::visitStatUpdates(const GameTypeReportConfig::StatUpdatesMap& statUpdates,  const GRECacheReport::StatUpdates& greStatUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    GameTypeReportConfig::StatUpdatesMap::const_iterator cit = statUpdates.begin();
    GameTypeReportConfig::StatUpdatesMap::const_iterator citEnd = statUpdates.end();

    if (cit == citEnd)
        return true;

    for ( ; cit != citEnd; ++cit)
    {
        const GameTypeReportConfig::StatUpdate* statUpdate = cit->second;
        const GRECacheReport::StatUpdate* greStatUpdate = greStatUpdates.find(cit->first)->second;
        bool skipStatUpdate = false;

        if (!checkCondition(*greStatUpdate->mCondition, context))
        {
            TRACE_LOG("[ReportParser].visitStatUpdates() : skipping updates for report game type '"
                << mGameType.getGameReportName().c_str() << "', category='" << statUpdate->getCategory() << "', entity='" << statUpdate->getEntityId() << "'");
            continue;
        }

        //  define entity used by this stat update
        GameReportExpression& entityExpr = *greStatUpdate->mEntityId;
        EA::TDF::TdfGenericValue entityIdResult;
        EntityId entityId = 0;
        bool evaluationSuccessful = false;

        if (entityExpr.evaluate(entityIdResult, context))
        {
            if (entityIdResult.isTypeInt())
            {
                EA::TDF::TdfGenericReference tempRef(entityId);
                evaluationSuccessful = entityIdResult.convertToIntegral(tempRef);
            }
            else if (entityIdResult.isTypeString())
            {
                if (blaze_str2int(entityIdResult.asString(), &entityId) != entityIdResult.asString())
                {
                    evaluationSuccessful = true;
                }
            }
        }

        if (!evaluationSuccessful)
        {
            ERR_LOG("[ReportParser].visitStatUpdates() : entityId expression parse failed for report game type '" 
                << mGameType.getGameReportName().c_str() << "', expr='" << statUpdate->getEntityId() << "'");
            setBlazeErrorFromExpression(entityExpr);
            return false;       // can't update stats without an entity.
        }

        //  define keyscope maps used.
        GameType::StatUpdate statUpdateKey;
        statUpdateKey.Config = statUpdate;
        statUpdateKey.Name = cit->first;
        GameType::EntityStatUpdate keyscopeMapKey(entityId, statUpdateKey);

        //  allow reuse of keyscopes - note we still want to recalculate scopes on another parse-run since values could've changed in the report
        //  or context.
        KeyscopesMap::insert_return_type keyinserter = mKeyscopesMap.insert(keyscopeMapKey);
        if (keyinserter.second)
        {
            TRACE_LOG("[ReportParser].visitStatUpdates() : GameType '" << mGameType.getGameReportName().c_str() << "', Adding keyscope for entity "
                      << entityId << ", configname='" << statUpdateKey.Name.c_str() << "'");
        }

        //  lookup scope index map - note this value may change depending on how the keyscopeIndex expression is evaluated in the config.
        //  this guarantees ScopeNameValueMap instances are unique per stat row if the keyscope index changes.
        ScopeNameValueByIndexMap& scopeIndexMap = keyinserter.first->second;

        uint64_t scopeMapIndex = 0;
        if (*statUpdate->getKeyscopeIndex() != '\0')
        {
            GameReportExpression& keyscopeIndexExpr = *greStatUpdate->mKeyscopeIndex;
            EA::TDF::TdfGenericValue keyscopeIndexResult;
            if (!keyscopeIndexExpr.evaluate(keyscopeIndexResult, context) && 
                keyscopeIndexExpr.getResultCode() != GameReportExpression::ERR_NO_RESULT && 
                !keyscopeIndexResult.getTypeDescription().isString() && !keyscopeIndexResult.getTypeDescription().isIntegral())
            {
                ERR_LOG("[ReportParser].visitStatUpdates() : keyscopeIndex expression parse failed for report game type '" 
                        << mGameType.getGameReportName().c_str() << "', expr='" << statUpdate->getKeyscopeIndex() << "'");
                setBlazeErrorFromExpression(keyscopeIndexExpr);
                return false;       // can't update stats without an entity.
            }
            if (keyscopeIndexExpr.getResultCode() != GameReportExpression::ERR_NO_RESULT)
            {
                EA::TDF::TdfGenericReference refIndex(scopeMapIndex);
                if (!keyscopeIndexResult.convertToIntegral(refIndex))           // Try to get the value as an int first: 
                {
                    if (keyscopeIndexResult.getTypeDescription().isString())    // Then try as a string
                    {
                        //  converting game attributes - which are strings unfortunately.
                        blaze_str2int(keyscopeIndexResult.asString(), &scopeMapIndex);
                    }
                    else
                    {
                        ERR_LOG("[ReportParser].visitStatUpdates() : keyscopeIndex expression parse failed for report game type '" 
                                << mGameType.getGameReportName().c_str() << "', expr='" << statUpdate->getKeyscopeIndex() << "' with result type " 
                                << (keyscopeIndexResult.getFullName() ? keyscopeIndexResult.getFullName() : "(unknown)") << " .");
                        setBlazeErrorFromExpression(keyscopeIndexExpr);
                        return false;       // can't update stats without an entity.
                    }
                }
            }
        }
        
        ScopeNameValueByIndexMap::insert_return_type scopeMapInserter = scopeIndexMap.insert(scopeMapIndex);

        Stats::ScopeNameValueMap& scopeNameValueMap = scopeMapInserter.first->second;
        scopeNameValueMap.clear();  // need to clear so keyscopes don't accumulate over iterations

        //  generate keyscopes.
        GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citKeyscopeConfig = statUpdate->getKeyscopes().begin();
        GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citKeyscopeConfigEnd = statUpdate->getKeyscopes().end();

        for ( ; citKeyscopeConfig != citKeyscopeConfigEnd; ++citKeyscopeConfig)
        {            
            GameReportExpression& expr = *(greStatUpdate->mKeyscopes.find(citKeyscopeConfig->first)->second);
            const char8_t* scopeName = citKeyscopeConfig->first;
            EA::TDF::TdfGenericValue result;
            uint64_t keyscopeValue = 0;
            EA::TDF::TdfGenericReference refKeyscope(keyscopeValue);
            if (!expr.evaluate(result, context))
            {
                if (statUpdate->getOptional())
                {
                    skipStatUpdate = true;
                    break;
                }
                WARN_LOG("[ReportParser].visitStatUpdates() : keyscope expression parse failed for report game type '" 
                         << mGameType.getGameReportName().c_str() << "', scope='" << scopeName << "'");
                setBlazeErrorFromExpression(expr);
                return false;       // not a good idea to update stats without keyscopes if they are defined?
            }
            else if (!result.convertToIntegral(refKeyscope))
            {
                if (result.getTypeDescription().isString())
                {
                    //  covert to integer as this may be a text string (i.e. from game manager attributes.)
                    if (blaze_str2int(result.asString(), &keyscopeValue) == result.asString())
                    {
                        WARN_LOG("[ReportParser].visitStatUpdates() : keyscope expression parse failed for report game type '" 
                                 << mGameType.getGameReportName().c_str() << "', STRING='" << result.asString() << "'");
                        return false;       
                    }
                }
                else
                {
                    WARN_LOG("[ReportParser].visitStatUpdates() : keyscope expression parse failed for report game type '" 
                             << mGameType.getGameReportName().c_str() << "', NOT AN INTEGER");
                    return false;       // not a good idea to update stats without keyscopes if they are defined?
                }
            }

            eastl::pair<Stats::ScopeNameValueMap::iterator, bool> inserter = scopeNameValueMap.insert(Stats::ScopeNameValueMap::value_type(scopeName, 0));
            inserter.first->second = keyscopeValue;
        }

        if (skipStatUpdate)
            continue;

        if (mReportParseMode == REPORT_PARSE_STATS)
        {
            EA::TDF::TdfGenericValue categoryResult(statUpdate->getCategory());
            if (*statUpdate->getCategoryExpr() != '\0')
            {
                GameReportExpression& categoryExpr = *greStatUpdate->mCategory;
                if (!categoryExpr.evaluate(categoryResult, context) || !categoryResult.getTypeDescription().isString())
                {
                    ERR_LOG("[ReportParser].visitStatUpdates() : category expression parse failed for report game type '"
                        << mGameType.getGameReportName().c_str() << "', expr='" << statUpdate->getCategoryExpr() << "'");
                    setBlazeErrorFromExpression(categoryExpr);
                    return false;
                }
            }

            //  generate stat row.
            TRACE_LOG("[ReportParser].visitStatUpdates() : stat row for entityId=" << entityId << ", category='" 
                      << categoryResult.asString() << "'");
            if (mUpdateStatsRequestBuilder == nullptr)
            {
                ERR_LOG("ReportParser].visitStatUpdates() : no valid stats request builder.");
                return false;
            }

            if (mStatCategorySummaryList.size() != 0)
            {
                //get the blaze object type by the stats category.
                CategoryNameTypeMap::const_iterator cntIter =mCategoryNameTypeMap.find(categoryResult.asString());
                if (cntIter != mCategoryNameTypeMap.end())
                {
                    //lookup the blaze object type map - check whether or not the object type can override to update the stat.
                    //If the value for the object type is set to ture, the mean is that we just update the entity stats that are allowed to update.
                    //if the vlaue is false or don't find the object type in the map, the mean is that we can update the stat of all entities.
                    bool ignoreObjectType = false;
                    EA::TDF::ObjectType objType = cntIter->second;
                    BlazeObjectTypeMap::const_iterator otIter = mOverrideStatUpdateByObjectType.find(objType);
                    if (otIter != mOverrideStatUpdateByObjectType.end())
                    {
                        ignoreObjectType = otIter->second;
                    }

                    if (ignoreObjectType)
                    {
                        //can't override to update the stat for the object type. So, check whether the entity is allowed to update his stat.
                        bool noStatUpdating = true;
                        EA::TDF::ObjectId objectId(objType, entityId);
                        BlazeObjectIdMap::const_iterator oiIter = mOverrideStatUpdateByObjectId.find(objectId);
                        if (oiIter != mOverrideStatUpdateByObjectId.end())
                        {
                            noStatUpdating = oiIter->second;
                        }

                        if (noStatUpdating)
                        {
                            TRACE_LOG("[ReportParser].visitStatUpdates: skipped stat update for entityType='" <<
                                objType.toString().c_str() << "' and entityId = '" << entityId << "'.");
                            continue;
                        }
                    }
                }
            }
            mUpdateStatsRequestBuilder->startStatRow(categoryResult.asString(), entityId, scopeNameValueMap.size() > 0 ? &scopeNameValueMap : nullptr);

            GameTypeReportConfig::StatUpdate::StatsMap::const_iterator citStat = statUpdate->getStats().begin();
            GameTypeReportConfig::StatUpdate::StatsMap::const_iterator citStatEnd = statUpdate->getStats().end();

            GRECacheReport::StatUpdate::Stats::const_iterator citGreStat = (greStatUpdate->mStats).begin();
            GRECacheReport::StatUpdate::Stats::const_iterator citGreStatEnd = (greStatUpdate->mStats).end();

            for ( ; citStat != citStatEnd && citGreStat != citGreStatEnd; ++citStat, ++citGreStat)
            {
                const GameTypeReportConfig::StatUpdate::Stat *stat = citStat->second;
                const GRECacheReport::StatUpdate::Stat* greStat = citGreStat->second;
                
                //  execute stat update conditionally
                if (checkCondition(*greStat->mCondition, context))
                {
                    GameReportExpression& expr = *greStat->mValue;
                    EA::TDF::TdfGenericValue result;          

                    if (!expr.evaluate(result, context))
                    {
                        if (statUpdate->getOptional())
                        {
                            continue;
                        }
                        WARN_LOG("[ReportParser].visitStatUpdates() : expression parse failed for report game type '" 
                                 << mGameType.getGameReportName().c_str() << "', expr='" << stat->getValue() << "'");
                        setBlazeErrorFromExpression(expr);
                    }
                    else
                    {
                        char8_t valueBuf[Collections::MAX_ATTRIBUTEVALUE_LEN];
                        const char8_t* value = valueBuf;
                        genericToString(result, valueBuf, sizeof(valueBuf));
         
                        TRACE_LOG("[ReportParser].visitStatUpdates() : stat '" << citStat->first.c_str() << "' = '" << value << "'");
                        mUpdateStatsRequestBuilder->makeStat(citStat->first, value, stat->getType());
                    }
                }
            }
            //  complete stat row
            mUpdateStatsRequestBuilder->completeStatRow();
        }
    }

    return true;
}

void ReportParser::genericToString(const EA::TDF::TdfGenericValue& result, char8_t* valueBuf, size_t bufSize)
{
    switch (result.getType())
    {
    case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIi32, result.asBool() ? 1 : 0);
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIu8, result.asUInt8());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIu16, result.asUInt16());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT32:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIu32, result.asUInt32());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIu64, result.asUInt64());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT8:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIi8, result.asInt8());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT16:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIi16, result.asInt16());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT32:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIi32, result.asInt32());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT64:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIi64, result.asInt64());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
        blaze_snzprintf(valueBuf, bufSize, "%" PRIi32, result.asEnum());
        break;

    case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:
        blaze_snzprintf(valueBuf, bufSize, "%f", result.asFloat());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_STRING:        
        blaze_strnzcpy(valueBuf, result.asString(), bufSize);
        break;
    default:
        break;
    }
}

double ReportParser::genericToDouble(const EA::TDF::TdfGenericValue& result)
{
    double value = 0.0;

    switch (result.getType())
    {
    case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
        value = result.asBool() ? 1.0 : 0.0;
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
        value = static_cast<double>(result.asUInt8());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
        value = static_cast<double>(result.asUInt16());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT32:
        value = static_cast<double>(result.asUInt32());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
        value = static_cast<double>(result.asUInt64());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT8:
        value = static_cast<double>(result.asInt8());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT16:
        value = static_cast<double>(result.asInt16());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT32:
        value = static_cast<double>(result.asInt32());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_INT64:
        value = static_cast<double>(result.asInt64());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
        value = static_cast<double>(result.asEnum());
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:
        value = result.asFloat();
        break;
    case EA::TDF::TDF_ACTUAL_TYPE_STRING:
        WARN_LOG("[ReportParser].genericToDouble: expression parsed to unsupported string type for report game type '"
            << mGameType.getGameReportName().c_str() << "', result='" << result.asString() << "' (value set to 0.0)");
        value = 0.0;
        break;
    default:
        break;
    }

    return value;
}

// visit the map of Stats Service updates for the subreport
bool ReportParser::visitStatsServiceUpdates(const GameTypeReportConfig::StatsServiceUpdatesMap& statsServiceUpdates, const GRECacheReport::StatsServiceUpdates& greStatsServiceUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    if (mReportParseMode != REPORT_PARSE_STATS_SERVICE)
        return true;

    for (const auto& mapUpdate : statsServiceUpdates)
    {
        const GameTypeReportConfig::StatsServiceUpdate* statsServiceUpdate = mapUpdate.second;
        const GRECacheReport::StatsServiceUpdate* greStatsServiceUpdate = greStatsServiceUpdates.find(mapUpdate.first)->second;

        if (!checkCondition(*greStatsServiceUpdate->mCondition, context))
        {
            TRACE_LOG("[ReportParser].visitStatsServiceUpdates: skipping updates for report game type '"
                << mGameType.getGameReportName().c_str() << "', context='" << statsServiceUpdate->getContext() << "', category='" << statsServiceUpdate->getCategory() << "', entity='" << statsServiceUpdate->getEntityId() << "'");
            continue;
        }

        // define entity used by this stat update
        GameReportExpression& entityExpr = *greStatsServiceUpdate->mEntityId;
        EA::TDF::TdfGenericValue entityIdResult;
        EntityId entityId = 0;
        bool evaluationSuccessful = false;

        if (entityExpr.evaluate(entityIdResult, context))
        {
            if (entityIdResult.isTypeInt())
            {
                EA::TDF::TdfGenericReference tempRef(entityId);
                evaluationSuccessful = entityIdResult.convertToIntegral(tempRef);
            }
            else if (entityIdResult.isTypeString())
            {
                if (blaze_str2int(entityIdResult.asString(), &entityId) != entityIdResult.asString())
                {
                    evaluationSuccessful = true;
                }
            }
        }

        if (!evaluationSuccessful)
        {
            ERR_LOG("[ReportParser].visitStatsServiceUpdates: entityId expression parse failed for report game type '"
                << mGameType.getGameReportName().c_str() << "', expr='" << statsServiceUpdate->getEntityId() << "'");
            setBlazeErrorFromExpression(entityExpr);
            return false; // can't update stats without an entity.
        }

        // generate stat row
        TRACE_LOG("[ReportParser].visitStatsServiceUpdates: stat row for entityId='" << entityId << "', category='"
            << statsServiceUpdate->getCategory() << "', context='" << statsServiceUpdate->getContext() << "'");

        if (mStatsServiceUtil == nullptr)
        {
            ERR_LOG("ReportParser].visitStatsServiceUpdates: no request builder");
            return false;
        }

        // If entity IDs do not have to be restricted to an int type, then we can directly convert entityIdResult to a string.
        char8_t entityIdStr[Collections::MAX_ATTRIBUTEVALUE_LEN];
        blaze_snzprintf(entityIdStr, sizeof(entityIdStr), "%" PRIi64, entityId);

        mStatsServiceUtil->startStatRequest(statsServiceUpdate->getContext(), statsServiceUpdate->getCategory(), entityIdStr);

        GameTypeReportConfig::StatsServiceUpdate::DimensionalStatsMap::const_iterator dimStatListItr = statsServiceUpdate->getDimensionalStats().begin();
        GameTypeReportConfig::StatsServiceUpdate::DimensionalStatsMap::const_iterator dimStatListEnd = statsServiceUpdate->getDimensionalStats().end();

        GRECacheReport::StatsServiceUpdate::DimensionalStats::const_iterator greDimStatListItr = (greStatsServiceUpdate->mDimensionalStats).begin();
        GRECacheReport::StatsServiceUpdate::DimensionalStats::const_iterator greDimStatListEnd = (greStatsServiceUpdate->mDimensionalStats).end();

        for (; dimStatListItr != dimStatListEnd && greDimStatListItr != greDimStatListEnd; ++dimStatListItr, ++greDimStatListItr)
        {
            EA::TDF::TdfStructVector<GameTypeReportConfig::StatsServiceUpdate::DimensionalStat>::const_iterator dimStatItr = dimStatListItr->second->begin();
            EA::TDF::TdfStructVector<GameTypeReportConfig::StatsServiceUpdate::DimensionalStat>::const_iterator dimStatEnd = dimStatListItr->second->end();

            GRECacheReport::StatsServiceUpdate::DimensionalStatList::const_iterator greDimStatItr = greDimStatListItr->second->begin();
            GRECacheReport::StatsServiceUpdate::DimensionalStatList::const_iterator greDimStatEnd = greDimStatListItr->second->end();

            uint32_t listIndex = 0;
            for (; dimStatItr != dimStatEnd && greDimStatItr != greDimStatEnd; ++dimStatItr, ++greDimStatItr)
            {
                // execute dimStat update conditionally
                GRECacheReport::StatsServiceUpdate::DimensionalStat* greDimStat = *greDimStatItr;
                GameTypeReportConfig::StatsServiceUpdate::DimensionalStatPtr dimStat = *dimStatItr;
                if (checkCondition(*greDimStat->mCondition, context))
                {
                    GameReportExpression& expr = *greDimStat->mValue;
                    EA::TDF::TdfGenericValue result;

                    if (!expr.evaluate(result, context))
                    {
                        if (!statsServiceUpdate->getOptional())
                        {
                            WARN_LOG("[ReportParser].visitStatsServiceUpdates: expression parse failed for report game type '"
                                << mGameType.getGameReportName().c_str() << "', expr='" << dimStat->getValue() << "'");
                            setBlazeErrorFromExpression(expr);
                        }
                        continue;
                    }

                    double value = genericToDouble(result);

                    // create entries for this dimensional stat only if there is at least one dimension for it.

                    GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citDim    = dimStat->getDimensions().begin();
                    GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citDimEnd = dimStat->getDimensions().end();
                    bool hasDimensions = false;

                    for ( ; citDim != citDimEnd; ++citDim)
                    {
                        GameReportExpression& exprDim = *(greDimStat->mDimensions.find(citDim->first)->second);

                        if (!exprDim.evaluate(result, context))
                        {
                            if (!statsServiceUpdate->getOptional())
                            {
                                WARN_LOG("[ReportParser].visitStatsServiceUpdates: expression parse failed for report game type '" <<
                                mGameType.getGameReportName().c_str() << " - dimensions, ', expr='" << citDim->second << "'");

                                setBlazeErrorFromExpression(exprDim);
                            }
                            continue;
                        }
                        char8_t valueBuf[Collections::MAX_ATTRIBUTEVALUE_LEN];
                        const char8_t* dimValue = valueBuf;
                        if (hasDimensions == false)
                        {
                            // create the dimension list the first time a valid dimension is found
                            TRACE_LOG("[ReportParser].visitStatsServiceUpdates: dimStat '" << dimStatListItr->first.c_str() << "' = '" << value << "'");
                            mStatsServiceUtil->addDimensionListToStat(dimStatListItr->first, value, dimStat->getType());
                            hasDimensions = true;
                        }
                        genericToString(result, valueBuf, sizeof(valueBuf));

                        TRACE_LOG("[ReportParser].visitStatsServiceUpdates: dimStat adding dimension '" << citDim->first << "' = '" << dimValue << "'");
                        mStatsServiceUtil->addDimensionToStatList(dimStatListItr->first, listIndex, citDim->first, dimValue);
                    }
                    if (hasDimensions)
                    {
                        listIndex++;
                    }
                    else
                    {
                        WARN_LOG("[ReportParser].visitStatsServiceUpdates: dimStat " << dimStatListItr->first.c_str() << " has no valid dimensions");
                    }
                }
            }
        }
        GameTypeReportConfig::StatsServiceUpdate::StatsMap::const_iterator statItr = statsServiceUpdate->getStats().begin();
        GameTypeReportConfig::StatsServiceUpdate::StatsMap::const_iterator statEnd = statsServiceUpdate->getStats().end();

        GRECacheReport::StatsServiceUpdate::Stats::const_iterator greStatItr = (greStatsServiceUpdate->mStats).begin();
        GRECacheReport::StatsServiceUpdate::Stats::const_iterator greStatEnd = (greStatsServiceUpdate->mStats).end();

        for (; statItr != statEnd && greStatItr != greStatEnd; ++statItr, ++greStatItr)
        {
            const GameTypeReportConfig::StatsServiceUpdate::Stat *stat = statItr->second;
            const GRECacheReport::StatsServiceUpdate::Stat* greStat = greStatItr->second;

            // execute stat update conditionally
            if (checkCondition(*greStat->mCondition, context))
            {
                GameReportExpression& expr = *greStat->mValue;
                EA::TDF::TdfGenericValue result;

                if (!expr.evaluate(result, context))
                {
                    if (!statsServiceUpdate->getOptional())
                    {
                        WARN_LOG("[ReportParser].visitStatsServiceUpdates: expression parse failed for report game type '"
                            << mGameType.getGameReportName().c_str() << "', expr='" << stat->getValue() << "'");
                        setBlazeErrorFromExpression(expr);
                    }
                    continue;
                }
                double value = genericToDouble(result);

                TRACE_LOG("[ReportParser].visitStatsServiceUpdates: stat '" << statItr->first.c_str() << "' = '" << value << "'");
                mStatsServiceUtil->makeStat(statItr->first, value, stat->getType());
            }
        }
        // complete stat row
        mStatsServiceUtil->completeStatRequest();
    }

    return true;
}

//  visit the map of report values for the subreport
bool ReportParser::visitGameHistory(const GameTypeReportConfig::GameHistoryList& gameHistoryList, const GRECacheReport::GameHistoryList& greGameHistoryList, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    if (mReportParseMode != REPORT_PARSE_GAME_HISTORY)
        return true;

    if (mGameHistoryReport == nullptr)
    {
        ERR_LOG("[ReportParser].visitGameHistory: Unable to parse game history, no game history report specified for the report parser.");
        return false;
    }

    GameTypeReportConfig::GameHistoryList::const_iterator cit = gameHistoryList.begin();
    GameTypeReportConfig::GameHistoryList::const_iterator citEnd = gameHistoryList.end();  
    if (cit == citEnd)
        return true;

    GRECacheReport::GameHistoryList::const_iterator citGRE = greGameHistoryList.begin();
    GRECacheReport::GameHistoryList::const_iterator citGREEnd = greGameHistoryList.end();
    if (citGRE == citGREEnd)
        return true;

    if (mGameHistoryReport->getTableRowMap().empty())
    {
        mGameHistoryReport->getTableRowMap().reserve(gameHistoryList.size());
    }

    /*
        Reuse the history data traversed in the upper level of the tree. Clear the traversed history
        data in the same level as it has been used to the mGameHistoryReport in previous run(s)
    */
    TableRowMapByLevel::insert_return_type mapInserter = mTraversedHistoryData.insert(context.level);
    GameHistoryReport::TableRowMap& currentTableRowMap = mapInserter.first->second;
    if (!currentTableRowMap.empty())
        currentTableRowMap.release();

    for (; cit != citEnd && citGRE != citGREEnd; ++cit, ++citGRE)
    {
        const char8_t *tableName = (*cit)->getTable();

        HistoryTableDefinitions::const_iterator histIter = mGameType.getHistoryTableDefinitions().find(tableName);
        if (histIter == mGameType.getHistoryTableDefinitions().end())
        {
            // the history table definition for this table name should exist. Most likely there's a bug
            ERR_LOG("[ReportParser.visitGameHistory: table '" << tableName <<
                " should exist but not found in history table definition");
            continue;
        }

        eastl::set<ReportAttributeName> primaryKeySet;
        typedef GameTypeReportConfig::GameHistory::StringReportAttributeNameList ConfigPrimaryKeyList;
        ConfigPrimaryKeyList::const_iterator citPrimaryKey = (*cit)->getPrimaryKey().begin();
        ConfigPrimaryKeyList::const_iterator citPrimaryKeyEnd =(*cit)->getPrimaryKey().end();

        for (; citPrimaryKey != citPrimaryKeyEnd; ++citPrimaryKey)
        {
            primaryKeySet.insert(*citPrimaryKey);
        }

        GameHistoryReport::TableRows* tableRows = currentTableRowMap.allocate_element();

        const GameTypeReportConfig::ReportValueByAttributeMap& columnsMap = (*cit)->getColumns();
        GameHistoryReport::TableRow *row = tableRows->getTableRowList().pull_back();
        row->getAttributeMap().reserve(columnsMap.size());

        GRECacheReport::GameHistory::Columns::const_iterator greColIter = (*citGRE)->mColumns.begin();
        GRECacheReport::GameHistory::Columns::const_iterator greColIterEnd = (*citGRE)->mColumns.end();        

        for (; greColIter != greColIterEnd; ++greColIter)
        {
            GameReportExpression& expr = *greColIter->second;
            EA::TDF::TdfGenericValue result;
            if (!expr.evaluate(result, context))
            {
                // ERR_NO_RESULT is NOT an error, so don't treat it as such.
                if (expr.getResultCode() != GameReportExpression::ERR_NO_RESULT)
                {
                    WARN_LOG("[ReportParser.visitGameHistory: expression parse failed for report game type '" <<
                        mGameType.getGameReportName().c_str() << "'");
                    setBlazeErrorFromExpression(expr);
                }

                if (primaryKeySet.find(greColIter->first.c_str()) != primaryKeySet.end())
                {
                    ERR_LOG("[ReportParser.visitGameHistory: primary key value '" << greColIter->first.c_str() <<
                        "' must be provided when storing game history in database");
                    // we must include a primary key in order to save game history
                    return false;
                }
            }
            else
            {
                char8_t valueBuf[Collections::MAX_ATTRIBUTEVALUE_LEN];
                genericToString(result, valueBuf, sizeof(valueBuf));
            
                row->getAttributeMap().insert(eastl::make_pair(greColIter->first.c_str(), valueBuf));

                TRACE_LOG("[ReportParser].visitGameHistory: level=" << context.level << " - adding " << 
                    greColIter->first.c_str() << "=" << valueBuf << " into game history table '" << tableName << "'");
            }
        }

        // update the traversed history data
        currentTableRowMap.insert(eastl::make_pair(tableName, tableRows));

        // If we reach the deepest level of the tree, create a table row in the mGameHistoryReport using the traversed data
        if (context.level == histIter->second->getDeepestLevel())
        {
            bool newTableRows = false;
            bool noHistorySaving = false;
            bool ignoreObjectType = false;
            GameHistoryReport::TableRowsPtr tableRowsInReport = nullptr;
            GameHistoryReport::TableRow *tableRowInReport = nullptr;
            EntityId entityId = 0;

            GameHistoryReport::TableRowMap::const_iterator tableIter = mGameHistoryReport->getTableRowMap().find(tableName);

            if (tableIter != mGameHistoryReport->getTableRowMap().end())
            {
                TRACE_LOG("[ReportParser].visitGameHistory: found '" << tableName << "' in game history list");
                tableRowsInReport = tableIter->second;
            }
            else
            {
                TRACE_LOG("[ReportParser].visitGameHistory: adding '" << tableName << "' table in game history list");
                tableRowsInReport = mGameHistoryReport->getTableRowMap().allocate_element();
                newTableRows = true;
            }

            if ((*cit)->getEntityType().component == 0 && (*cit)->getEntityType().type == 0)
            {
                //if uninitialized
                BlazeObjectTypeMap::const_iterator otIter = mOverrideGameHistoryByObjectType.find(ENTITY_TYPE_USER);
                if (otIter != mOverrideGameHistoryByObjectType.end())
                {
                    ignoreObjectType = otIter->second;
                }
            }
            else
            {
                BlazeObjectTypeMap::const_iterator otIter = mOverrideGameHistoryByObjectType.find((*cit)->getEntityType());
                if (otIter != mOverrideGameHistoryByObjectType.end())
                {
                    ignoreObjectType = otIter->second;
                }
            }

            TableRowMapByLevel::const_iterator rowMapIter, rowMapEnd;
            rowMapIter = mTraversedHistoryData.begin();
            rowMapEnd = mTraversedHistoryData.end();

            // iterate each level in mTraversedHistoryData
            for (; rowMapIter != rowMapEnd; ++rowMapIter)
            {
                // insert the traversed data in each level into a same table row of mGameHistoryReport

                GameHistoryReport::TableRowMap::const_iterator trIter = rowMapIter->second.find(tableName);
                if (trIter != rowMapIter->second.end())
                {
                    GameHistoryReport::TableRowList::const_iterator rlIter, rlEnd;
                    rlIter = trIter->second->getTableRowList().begin();
                    rlEnd = trIter->second->getTableRowList().end();

                    for (; rlIter != rlEnd; ++rlIter)
                    {
                        if (tableRowInReport == nullptr)
                        {
                            tableRowInReport = tableRowsInReport->getTableRowList().pull_back();
                        }

                        if (!primaryKeySet.empty())
                        {
                            Collections::AttributeMap::const_iterator it = (*rlIter)->getAttributeMap().find((primaryKeySet.begin())->c_str());

                            if (it != (*rlIter)->getAttributeMap().end())
                            {
                                blaze_str2int(it->second, &entityId);
                                BlazeObjectIdMap::const_iterator oiIter;
                                if ((*cit)->getEntityType().component == 0 && (*cit)->getEntityType().type == 0)
                                {
                                    //if uninitialized
                                    EA::TDF::ObjectId objectId(ENTITY_TYPE_USER, entityId);
                                    oiIter = mOverrideGameHistoryByObjectId.find(objectId);
                                }
                                else
                                {
                                    EA::TDF::ObjectId objectId((*cit)->getEntityType(), entityId);
                                    oiIter = mOverrideGameHistoryByObjectId.find(objectId);
                                }

                                if (oiIter != mOverrideGameHistoryByObjectId.end())
                                {
                                    noHistorySaving = oiIter->second;
                                }
                                else
                                {
                                    noHistorySaving = ignoreObjectType;
                                }
                            }
                        }

                        tableRowInReport->getAttributeMap().insert((*rlIter)->getAttributeMap().begin(), (*rlIter)->getAttributeMap().end());
                    }
                }
            }

            if (noHistorySaving)
            {
                TRACE_LOG("[ReportParser].visitGameHistory: skipped game history saving for entityType='" <<
                    (*cit)->getEntityType().toString().c_str() << "' and entityId = '" << entityId << "'.");

                tableRowInReport->getAttributeMap().clear();                
                tableRowsInReport->getTableRowList().pop_back();
            }
            else if (newTableRows)
            {
                mGameHistoryReport->getTableRowMap().insert(eastl::make_pair(tableName, tableRowsInReport));
            }
        }
    }

    return true;
}


bool ReportParser::visitMetricUpdates(const GameTypeReportConfig::ReportValueByAttributeMap& metricUpdates, const GRECacheReport::MetricUpdates& greMetricUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    if (mReportParseMode != REPORT_PARSE_METRICS)
        return true;

    GameTypeReportConfig::ReportValueByAttributeMap::const_iterator cit = metricUpdates.begin();
    GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citEnd = metricUpdates.end();
   
    if (cit == citEnd)
        return true;

    for (; cit != citEnd; ++cit)
    {
        const char8_t *metricName = cit->first.c_str();
        GameReportExpression& expr = *(greMetricUpdates.find(cit->first)->second);
        EA::TDF::TdfGenericValue result;
        if (!expr.evaluate(result, context))
        {
            if (expr.getResultCode() != GameReportExpression::ERR_NO_RESULT)
            {
                WARN_LOG("[ReportParser].visitMetricUpdates() : expression parse failed for report game type '" 
                     << mGameType.getGameReportName().c_str() << "', expr='" << cit->second.c_str() << "'");
                setBlazeErrorFromExpression(expr);
            }
        }
        else
        {
            TRACE_LOG("[ReportParser].visitMetricUpdates() : metric update " << metricName << "=" << cit->second.c_str() << ".");

            uint64_t value;  
            EA::TDF::TdfGenericReference refValue(value);
            if (result.convertToIntegral(refValue))
            {
                mMetricsCollection.updateMetrics(metricName, value);
            }
            else 
            {
                ERR_LOG("[ReportParser].visitMetricUpdates() : metric update " << metricName << "=" << cit->second.c_str() << ", value must be an integer");
            }
        }
    }

    return true;
}

bool ReportParser::visitEventUpdates(const GameTypeReportConfig::EventUpdatesMap& events, const GRECacheReport::EventUpdates& greEvents, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    if (mReportParseMode != REPORT_PARSE_EVENTS)
        return true;

    GameTypeReportConfig::EventUpdatesMap::const_iterator cit = events.begin();
    GameTypeReportConfig::EventUpdatesMap::const_iterator citEnd = events.end();

    if (cit == citEnd)
        return true;

    GRECacheReport::EventUpdates::const_iterator citGre = greEvents.begin();
    GRECacheReport::EventUpdates::const_iterator citGreEnd = greEvents.end();

    // go through all the events for this subreport
    for ( ; cit != citEnd && citGre != citGreEnd; ++cit, ++citGre)
    {
        // check the event type is defined in the event list
        bool useGenericEventData = true;
        
        const EventType* eventType = nullptr;

        if (mEventTypes != nullptr)
        {
            for (EventTypes::const_iterator i = mEventTypes->begin(), e = mEventTypes->end(); i != e; ++i)
            {
                eventType = *i;
                if (blaze_stricmp(eventType->getName(), cit->first.c_str()) == 0)
                {
                    const char8_t* eventDataTdfClass = eventType->getTdf();
                    if (eventDataTdfClass != nullptr && eventDataTdfClass[0] != '\0')
                        useGenericEventData = false;

                    break;
                }
            }
        }

        if (eventType == nullptr)
        {
            WARN_LOG("[ReportParser].visitEventUpdates() : the event type (" << cit->first.c_str() << ") in game type ("
                     << mGameType.getGameReportName().c_str() << ") is invalid, please check the event type list.");
            continue;
        }

        if (mEvents.find(eventType->getName()) == mEvents.end())
        {
            CustomEvent* customEvent = BLAZE_NEW CustomEvent();
            customEvent->setEventType(eventType->getName());
            if (useGenericEventData)
            {
                GenericEvent* eventData = BLAZE_NEW GenericEvent();
                customEvent->setEventData(*eventData);
            }
            else
            {
                EA::TDF::Tdf* eventDataTdfObj = mGameType.createReportTdf(eventType->getTdf());
                customEvent->setEventData(*eventDataTdfObj);
            }
            mEvents[eventType->getName()] = customEvent;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Begin to parse the event...
        const GameTypeReportConfig::Event* reportEvent = cit->second;
        const GRECacheReport::Event* greReportEvent = citGre->second; 
        Collections::AttributeMap valueMap;

        for (GRECacheReport::Event::EventData::const_iterator i = greReportEvent->mEventData.begin(), e = greReportEvent->mEventData.end(); 
                i != e; ++i)
        {
            GameReportExpression& expr = *(i->second);
            const char8_t* attrName = i->first;
            EA::TDF::TdfGenericValue result;

            // evaluate the value using expression
            if (expr.evaluate(result, context))
            {
                char8_t value[64];
                genericToString(result, value, sizeof(value));

                // insert value directly to the generic event tdf
                if (useGenericEventData)
                {
                    GenericEvent* eventData = static_cast<GenericEvent*>(mEvents[eventType->getName()]->getEventData());
                    eventData->getEventMap()[attrName] = value;
                }
                // insert value to a temporary container
                else
                {
                    valueMap[attrName] = value;
                }
            }
        }

        if (!valueMap.empty())
        {
            // parsing entity id...
            GameReportExpression& entityExpr = *greReportEvent->mEntityId;
            EA::TDF::TdfGenericValue entityIdResult;

            if (!entityExpr.evaluate(entityIdResult, context) || !entityIdResult.getTypeDescription().isIntegral())
            {
                entityIdResult.set(static_cast<uint64_t>(0));
            }

            // add the report values into the eventTdfParser. They are used for visiting the tdf obejct.
            mEventTdfParser.initValuesForVisitor(entityIdResult, valueMap);
            mEventTdfParser.visit(*reportEvent, *greReportEvent, *(mEvents[eventType->getName()]->getEventData()), *(mEvents[eventType->getName()]->getEventData()));
        }
        // end of paring event...
        //////////////////////////////////////////////////////////////////////////////////////////////////

    } // end of for - go through all the events for this game type

    return true;
}

bool ReportParser::checkCondition(GameReportExpression& condexpr, const GameReportContext& context)
{
    bool runUpdate = true;
    EA::TDF::TdfGenericValue result;
    if (condexpr.evaluate(result, context))
    {
        EA::TDF::TdfGenericReference refRunUpdate(runUpdate);
        if (!result.convertToIntegral(refRunUpdate)) 
        {
            if (result.isTypeString())
            {
                runUpdate = !(result.asString() == "0") && !(result.asString() == "false");
            }
            else
            {
                WARN_LOG("[ReportParser].checkCondition() : Conditional expression returned a non-int non-string type ("<< result.getFullName() <<").");
            }
        }
    }
    return runUpdate;
}


bool ReportParser::visitAchievementUpdates(const GameTypeReportConfig::AchievementUpdates& achievements, const GRECacheReport::AchievementUpdates& greAchievements, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
#ifdef TARGET_achievements
    if (mReportParseMode != REPORT_PARSE_ACHIEVEMENTS)
        return true;

    Blaze::Achievements::ProductId productId = achievements.getProductId();
    if (productId.empty())
    {
        TRACE_LOG("[ReportParser].visitAchievementUpdates() : productId expression parse failed for report game type '" 
            << mGameType.getGameReportName().c_str() << "'");
        return true;       // can't update achievements without an product Id.
    }

    //  define entity used by this achievement update
    GameReportExpression& entityExpr = *greAchievements.mEntityId;
    EA::TDF::TdfGenericValue entityIdResult;
    if (!entityExpr.evaluate(entityIdResult, context))
    {
        TRACE_LOG("[ReportParser].visitAchievementUpdates() : entityId expression parse failed for report game type '" 
            << mGameType.getGameReportName().c_str() << "', expr='" << achievements.getEntityId() << "'");
        return true;       // can't update achievements without an entity.
    }

    EntityId entityId;
    EA::TDF::TdfGenericReference refEntityId(entityId);
    if (!entityIdResult.convertToIntegral(refEntityId))
    {
        TRACE_LOG("[ReportParser].visitAchievementUpdates() : entityId expression parse failed for report game type '" 
            << mGameType.getGameReportName().c_str() << "', expr='" << achievements.getEntityId() << "'  (unable to parse result type " << (entityIdResult.getFullName() ? entityIdResult.getFullName() : "(unknown)") << " )");
        return true;       // can't update achievements without an entity.
    }
    

    GameTypeReportConfig::AchievementUpdates::AchievementList::const_iterator achieveIter = achievements.getAchievements().begin(), achieveEnd = achievements.getAchievements().end();
    GRECacheReport::AchievementUpdates::Achievements::const_iterator greAchieve = greAchievements.mAchievements.begin(), greAchieveEnd = greAchievements.mAchievements.end();
    for (; greAchieve != greAchieveEnd && achieveIter != achieveEnd; ++greAchieve, ++achieveIter)
    {
        //  execute achievement update conditionally
        if (checkCondition(*greAchieve->second->mCondition, context))
        {
            EA::TDF::TdfGenericValue result;
            GameReportExpression& expr = *greAchieve->second->mPoints;

            if (!expr.evaluate(result, context))
            {
                WARN_LOG("[ReportParser].visitAchievementUpdates() : expression parse failed for report game type '" 
                    << mGameType.getGameReportName().c_str() << "', expr='" << (*achieveIter)->getPoints() << "'");
                setBlazeErrorFromExpression(expr);
            }
            else
            {
                int64_t points = 0;
                EA::TDF::TdfGenericReference refPoints(points);
                if (!result.convertToIntegral(refPoints))           // Try int conversion first:
                {
                    if (result.getTypeDescription().isString())     
                    {
                        blaze_str2int(result.asString(), &points);
                    }
                }

                TRACE_LOG("[ReportParser].visitAchievementUpdates() : Added achievement id = '" << greAchieve->first << "', points = '" << points << "'");
                Achievements::GrantAchievementRequest* request = BLAZE_NEW Achievements::GrantAchievementRequest;
                request->getUser().setType(Achievements::PERSONA);
                request->getUser().setId(entityId);
                request->getAuxAuth().getUser().setType(Blaze::Achievements::NUCLEUS_PERSONA);
                request->getAuxAuth().getUser().setId(entityId);
                request->setProductId(productId);
                request->setIncludeMetadata((*achieveIter)->getIncludeMetadata());
                request->setAchieveId(greAchieve->first);
                request->getProgress().setPoints(points);
                mGrantAchievementRequests.push_back(request);
            }
        }
    }

    // fill PostEventsRequests
    const char8_t* device = achievements.getDevice();
    GameTypeReportConfig::AchievementUpdates::EventList::const_iterator eit = achievements.getEventList().begin();
    GameTypeReportConfig::AchievementUpdates::EventList::const_iterator eEnd = achievements.getEventList().end();

    GRECacheReport::AchievementUpdates::Events::const_iterator greEvent = greAchievements.mEvents.begin(), greEventEnd = greAchievements.mEvents.end();
    for (; greEvent != greEventEnd && eit != eEnd; ++greEvent, ++eit)
    {
        //  execute achievement update conditionally
        Achievements::PostEventsRequest* request = BLAZE_NEW Achievements::PostEventsRequest;
        request->getUser().setType(Achievements::PERSONA);
        request->getUser().setId(entityId);
        request->getAuxAuth().getUser().setType(Blaze::Achievements::NUCLEUS_PERSONA);
        request->getAuxAuth().getUser().setId(entityId);
        request->setProductId(productId);

        Achievements::EventData* event = request->getPayload().getEvents().pull_back();
        event->setId((*eit)->getId());
        request->getPayload().setDevice(device);
        GRECacheReport::AchievementUpdates::Event::EventData& eventData = (*greEvent)->mData;
        GRECacheReport::AchievementUpdates::Event::EventData::const_iterator greEventData = eventData.begin(), greEventDataEnd = eventData.end();
        for (; greEventData != greEventDataEnd; ++greEventData)
        {
            GameReportExpression& expr = *greEventData->second;
            EA::TDF::TdfGenericValue result;
            if (!expr.evaluate(result, context))
            {
                // ERR_NO_RESULT is NOT an error, so don't treat it as such.
                if (expr.getResultCode() != GameReportExpression::ERR_NO_RESULT)
                {
                    WARN_LOG("[ReportParser.visitAchievementUpdates: expression parse failed for report game type '" <<
                        mGameType.getGameReportName().c_str() << "'");
                    setBlazeErrorFromExpression(expr);
                }
            }
            else
            {
                char8_t valueBuf[Collections::MAX_ATTRIBUTEVALUE_LEN];
                genericToString(result, valueBuf, sizeof(valueBuf));
                event->getData()[greEventData->first] = valueBuf;
            }
        }
        mPostEventsRequests.push_back(request);
    }

#endif
    return true;
}

//  generates keyscopes for the UpdateStats utility, returns the number of keyscopes
size_t ReportParser::generateScopeIndexMap(Stats::ScopeNameValueMap& scopemap, EntityId entityId, const GameType::StatUpdate& statUpdate, uint64_t index)
{
    GameType::EntityStatUpdate keyscopeMapKey(entityId, statUpdate);

    KeyscopesMap::const_iterator cit = mKeyscopesMap.find(keyscopeMapKey);
    if (cit == mKeyscopesMap.end())
        return 0;

    scopemap.clear();

    //  lookup by index.
    const ScopeNameValueByIndexMap::const_iterator citIndexMapIt = cit->second.find(index);
    if (citIndexMapIt == cit->second.end())
        return 0;

    //  retrieve name/value map at specified index and copy to target.
    const Stats::ScopeNameValueMap& scopeMap = citIndexMapIt->second;
    scopemap.insert(scopeMap.begin(), scopeMap.end());
    return scopemap.size();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

void ReportParser::makeStat(const char8_t *name, const char8_t *val, int32_t updateType)
{
    if (mUpdateStatsRequestBuilder != nullptr)
    {
        mUpdateStatsRequestBuilder->makeStat(name, val, updateType);
    }
}


bool ReportParser::visitPsnMatchUpdates(const GameTypeReportConfig::PsnMatchUpdatesMap& psnMatchUpdates, const GRECacheReport::PsnMatchUpdates& grePsnMatchUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    if (mReportParseMode != REPORT_PARSE_PSN_MATCH)
        return true;

    if (mPsnMatchUtil == nullptr)
    {
        ERR_LOG("ReportParser].visitPsnMatchUpdates: no request builder");
        return false;
    }

    PSNServices::Matches::GetMatchDetailResponsePtr matchDetail = mPsnMatchUtil->getMatchDetail();
    if (matchDetail == nullptr)
    {
        ERR_LOG("ReportParser].visitPsnMatchUpdates: no match detail");
        return false;
    }

    auto competitionType = PSNServices::Matches::PsnCompetitionTypeEnum::INVALID_COMPETITION_TYPE;
    PSNServices::Matches::ParsePsnCompetitionTypeEnum(matchDetail->getCompetitionType(), competitionType);

    auto resultType = PSNServices::Matches::PsnResultTypeEnum::INVALID_RESULT_TYPE;
    PSNServices::Matches::ParsePsnResultTypeEnum(matchDetail->getResultType(), resultType);

    for (const auto& mapUpdate : psnMatchUpdates)
    {
        const GameTypeReportConfig::PsnMatchUpdate* psnMatchUpdate = mapUpdate.second;
        const GRECacheReport::PsnMatchUpdate* grePsnMatchUpdate = grePsnMatchUpdates.find(mapUpdate.first)->second;

        GameReportExpression& entityExpr = *grePsnMatchUpdate->mMatchResults->mCompetitiveResult->mEntityId;
        EA::TDF::TdfGenericValue entityIdResult;
        EntityId entityId = 0;
        bool evaluationSuccessful = false;
        if (entityExpr.evaluate(entityIdResult, context))
        {
            if (entityIdResult.isTypeInt())
            {
                EA::TDF::TdfGenericReference tempRef(entityId);
                evaluationSuccessful = entityIdResult.convertToIntegral(tempRef);
            }
            else if (entityIdResult.isTypeString())
            {
                if (blaze_str2int(entityIdResult.asString(), &entityId) != entityIdResult.asString())
                {
                    evaluationSuccessful = true;
                }
            }
        }
        if (!evaluationSuccessful)
        {
            ERR_LOG("[ReportParser].visitPsnMatchUpdates: entityId expression parse failed for report game type '"
                << mGameType.getGameReportName().c_str() << "', expr='" << psnMatchUpdate->getMatchResults().getCompetitiveResult().getEntityId() << "'");
            setBlazeErrorFromExpression(entityExpr);
            return false;       // can't update stats without an entity.
        }

        GameReportExpression& scoreExpr = *grePsnMatchUpdate->mMatchResults->mCompetitiveResult->mScore;
        EA::TDF::TdfGenericValue scoreResult;
        if (!scoreExpr.evaluate(scoreResult, context))
        {
            TRACE_LOG("[ReportParser].visitPsnMatchUpdates: score expression parse failed for report game type '"
                    << mGameType.getGameReportName().c_str() << "', expr='" << psnMatchUpdate->getMatchResults().getCompetitiveResult().getScore() << "'");

            if (competitionType == PSNServices::Matches::PsnCompetitionTypeEnum::COMPETITIVE)
            {
                ERR_LOG("[ReportParser].visitPsnMatchUpdates: competitive type match requires 'score' for report game type '"
                    << mGameType.getGameReportName().c_str() << "'");
                return false;
            }
        }
        double score = genericToDouble(scoreResult);

        /// @todo [ps5-gr] move cooperativeResult to non-player-specific location ???  requires config change
        GameReportExpression& coopExpr = *grePsnMatchUpdate->mMatchResults->mCooperativeResult;
        EA::TDF::TdfGenericValue coopResult;
        int64_t coopValue = 0;
        if (coopExpr.evaluate(coopResult, context))
        {
            // cooperativeResult should be int (or enum) type
            EA::TDF::TdfGenericReference tempRef(coopValue);
            if (!coopResult.convertToIntegral(tempRef))
            {
                TRACE_LOG("[ReportParser].visitPsnMatchUpdates: cooperativeResult failed to convert for report game type '"
                    << mGameType.getGameReportName().c_str() << "', expr='" << psnMatchUpdate->getMatchResults().getCooperativeResult() << "'");
            }
        }
        else
        {
            TRACE_LOG("[ReportParser].visitPsnMatchUpdates: cooperativeResult expression parse failed for report game type '"
                << mGameType.getGameReportName().c_str() << "', expr='" << psnMatchUpdate->getMatchResults().getCooperativeResult() << "'");
        }
        if (competitionType == PSNServices::Matches::PsnCompetitionTypeEnum::COOPERATIVE)
        {
            if ((PSNServices::Matches::PsnCooperativeResultEnum) coopValue == PSNServices::Matches::PsnCooperativeResultEnum::INVALID_COOPRESULT_TYPE)
            {
                ERR_LOG("[ReportParser].visitPsnMatchUpdates: cooperative match requires valid 'cooperativeResult' for report game type '"
                    << mGameType.getGameReportName().c_str() << "'");
                return false;
            }
        }

        TRACE_LOG("[ReportParser].visitPsnMatchUpdates: results for entityId=" << entityId << ", score=" << score << ", cooperativeResult=" << coopValue);

        // additional MatchStatistics
        PsnMatchUtil::MatchStatMap matchStatMap;
        for (const auto& matchStat : psnMatchUpdate->getMatchStats())
        {
            const char8_t* statKey = matchStat.first.c_str();
            GameReportExpression& statValueExpr = *grePsnMatchUpdate->mMatchStats.find(matchStat.first)->second;
            EA::TDF::TdfGenericValue statValueResult;
            if (!statValueExpr.evaluate(statValueResult, context))
            {
                WARN_LOG("[ReportParser].visitPsnMatchUpdates: stat value expression parse failed for report game type '"
                    << mGameType.getGameReportName().c_str() << "', stat='" << statKey << "', expr='" << matchStat.second->getVal() << "'");
                continue;
            }
            matchStatMap[statKey] = (float) genericToDouble(statValueResult);

            mPsnMatchUtil->addMatchStatFormat(statKey, matchStat.second->getFormat());
        }

        mPsnMatchUtil->addPlayerResult(entityId, (float) score, coopValue, matchStatMap);
    }

    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  EA::TDF::TdfVisitor used to write report values to a report TDF.
//
///////////////////////////////////////////////////////////////////////////////////////////////////

void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue)
{
    VisitorState& state = mStateStack.top();
    if (mReportParseMode == REPORT_PARSE_VALUES && !state.isMapKey)
    {
        EA::TDF::TdfGenericValue result;
        if (getValueFromTdfViaConfig(result, *state.config, *state.context, parentTdf, tag))
        {
            EA::TDF::TdfGenericReference refValue(value);
            result.convertToIntegral(refValue);
        }
    }

    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    setIntReportValueViaConfig(value, state, parentTdf, tag);
    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue)
{
    VisitorState& state = mStateStack.top();
    if (mReportParseMode == REPORT_PARSE_VALUES && !state.isMapKey)
    {
        EA::TDF::TdfGenericValue result;
        if (getValueFromTdfViaConfig(result, *state.config, *state.context, parentTdf, tag) && result.getTypeDescription().isFloat())
        {
            value = result.asFloat();
        }
    }

    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}                          


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    VisitorState& state = mStateStack.top();
    if (mReportParseMode == REPORT_PARSE_VALUES && !state.isMapKey)
    {
        EA::TDF::TdfGenericValue result;
        if (getValueFromTdfViaConfig(result, *state.config, *state.context, parentTdf, tag) && result.getTypeDescription().isString())
        {
            value = result.asString();
        }
    }

    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


void ReportParser::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue)
{
    VisitorState& state = mStateStack.top();
    if (mReportParseMode == REPORT_PARSE_VALUES && !state.isMapKey)
    {
        EA::TDF::TdfGenericValue result;
        if (getValueFromTdfViaConfig(result, *state.config, *state.context, parentTdf, tag) && result.getTypeDescription().isString())
        {
            //  find enumeration using the string
            const char8_t* enumName = result.asString();
            //  write out enum value.
            if ((enumMap == nullptr) || !enumMap->findByName(enumName, value))
            {
                WARN_LOG("[ReportParser].visit() : Game type '" << mGameType.getGameReportName().c_str() << "', tag=" << tag 
                         << ", enum does not have a value mapped to '" << enumName << "'");
            }
        }
    }

    ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
}


//  unified method for integer report values.
template <typename T> void ReportParser::setIntReportValueViaConfig(T& value, const VisitorState& state, const EA::TDF::Tdf& parentTdf, uint32_t tag)
{
    if (mReportParseMode == REPORT_PARSE_VALUES && !state.isMapKey)
    {
        EA::TDF::TdfGenericValue result;
        if (getValueFromTdfViaConfig(result, *state.config, *state.context, parentTdf, tag))
        {
            EA::TDF::TdfGenericReference refValue(value);
            if (!result.convertToIntegral(refValue))
            {
                if (result.getTypeDescription().isString())
                {                
                    blaze_str2int(result.asString(), &value);             
                }
            }
        }
    }
}

//  evaluates a general expression from EA::TDF::Tdf tag/config
bool ReportParser::getValueFromTdfViaConfig(EA::TDF::TdfGenericValue& value, const GameTypeReportConfig& config, const GameReportContext& context, const EA::TDF::Tdf& parentTdf, uint32_t tag) const
{
    //  evaluate expression and store value
    const char8_t* valMemberName = nullptr;
    if (parentTdf.getMemberNameByTag(tag, valMemberName))
    {
        //  TODO: need to get the real pre-typecomp member name, not the C++ name - this code is making a big assumption about TDF member names.

        //  NOTE THIS SLOW COMPARE IS DUE TO HOW THE CONFIGDECODER DECODES CONFIGS INTO MAPS - NOT SORTING THE VECTOR_MAP.
        //  MUST RESORT TO SIMPLE FIND STRING IN MAP UNTIL FIXED.
        GameTypeReportConfig::ReportValueByAttributeMap::const_iterator cit = config.getReportValues().begin();
        for (; cit != config.getReportValues().end(); ++cit)
        {
            if (blaze_stricmp(cit->first.c_str(), valMemberName)==0)
                break;
        }
        if (cit != config.getReportValues().end())
        {
            const char8_t *valueExprStr = cit->second.c_str();
            //  found configuration for this member.
            GameReportExpression expr(mGameType, cit->second.c_str());
            
            if (expr.evaluate(value, context))
            {
                return true;
            }
            else
            {                
                WARN_LOG("[ReportParser].getValueFromTdfViaConfig() : expression parse failed for report game type '" << mGameType.getGameReportName().c_str() 
                         << "', tag='" << valMemberName << "', expr='" << valueExprStr << "'");
                setBlazeErrorFromExpression(expr);
            }
        }      
    }
    else
    {
        ERR_LOG("[ReportParser].getValueFromTdfViaConfig() : Tag " << tag << " not found in parentTdf '" << parentTdf.getClassName() << "' - SHOULD NOT HAPPEN");
    }

    return false;
}


void ReportParser::setBlazeErrorFromExpression(const GameReportExpression& expr) const
{
    GameReportExpression::ResultCode resultCode = expr.getResultCode();
    if (resultCode == GameReportExpression::ERR_OK)
        return;

    switch (resultCode)
    {
    case GameReportExpression::ERR_MISSING_GAME_ATTRIBUTE:
    case GameReportExpression::ERR_MISSING_PLAYER_ATTRIBUTE:
        mBlazeError = Blaze::GAMEREPORTING_COLLATION_ERR_MISSING_GAME_ATTRIBUTE;
        break;
    default:
        mBlazeError = Blaze::GAMEREPORTING_CUSTOM_ERR_PROCESSING_FAILED;
        break;                    
    }

    WARN_LOG("[ReportParser].setBlazeErrorFromExpression() : result=" << resultCode);
}

void ReportParser::ignoreStatUpdatesForObjectType(const EA::TDF::ObjectType& objectType, bool ignore)
{
    BlazeObjectTypeMap::iterator iter = mOverrideStatUpdateByObjectType.find(objectType);
    if (iter == mOverrideStatUpdateByObjectType.end())
    {
        mOverrideStatUpdateByObjectType.insert(eastl::make_pair(objectType, ignore));
    }
    else
    {
        iter->second = ignore;
    }

    TRACE_LOG("[ReportParser].ignoreStatUpdatesForObjectType: " << ((ignore) ? "disable" : "enable") << 
        " stat updates for all entityType=" << objectType.toString().c_str() << ".");
}

void ReportParser::ignoreGameHistoryForObjectType(const EA::TDF::ObjectType& objectType, bool ignore)
{
    BlazeObjectTypeMap::iterator iter = mOverrideGameHistoryByObjectType.find(objectType);
    if (iter == mOverrideGameHistoryByObjectType.end())
    {
        mOverrideGameHistoryByObjectType.insert(eastl::make_pair(objectType, ignore));
    }
    else
    {
        iter->second = ignore;
    }

    TRACE_LOG("[ReportParser].ignoreGameHistoryForObjectType: " << ((ignore) ? "disable" : "enable") << 
        " game history saving for all entityType=" << objectType.toString().c_str() << ".");
}

void ReportParser::ignoreStatUpdatesForEntityId(const EA::TDF::ObjectType& objectType, const EntityId& entityId, bool ignore)
{
    EA::TDF::ObjectId objectId(objectType, entityId);

    BlazeObjectIdMap::iterator iter = mOverrideStatUpdateByObjectId.find(objectId);
    if (iter == mOverrideStatUpdateByObjectId.end())
    {
        mOverrideStatUpdateByObjectId.insert(eastl::make_pair(objectId, ignore));
    }
    else
    {
        iter->second = ignore;
    }

    TRACE_LOG("[ReportParser].ignoreStatUpdatesForEntityId: " << ((ignore) ? "disable" : "enable") << 
        " stat updates for entityType=" << objectType.toString().c_str() << " entityId=" << entityId << ".");
}

void ReportParser::ignoreGameHistoryForEntityId(const EA::TDF::ObjectType& objectType, const EntityId& entityId, bool ignore)
{
    EA::TDF::ObjectId objectId(objectType, entityId);

    BlazeObjectIdMap::iterator iter = mOverrideGameHistoryByObjectId.find(objectId);
    if (iter == mOverrideGameHistoryByObjectId.end())
    {
        mOverrideGameHistoryByObjectId.insert(eastl::make_pair(objectId, ignore));
    }
    else
    {
        iter->second = ignore;
    }

    TRACE_LOG("[ReportParser].ignoreGameHistoryForEntityId: " << ((ignore) ? "disable" : "enable") << 
        " game history saving for entityType=" << objectType.toString().c_str() << " entityId=" << entityId << ".");
}


} //namaespace Utiltiies

} //namespace GameReporting
} //namespace Blaze
