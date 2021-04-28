/*************************************************************************************************/
/*!
    \file   gametype.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "gametype.h"
#include "gamereportexpression.h"

namespace Blaze
{
namespace GameReporting
{

////////////////////////////////////////////////////////////////////////////////////////////////////
HistoryTableDefinition::HistoryTableDefinition()
{
    mHistoryLimit = 0;
    mDeepestLevel = 0;
}

void HistoryTableDefinition::addColumn(const char8_t* columnName, ColumnType type)
{
    ReportAttributeName columnNameStr(columnName);
    if (mColumnList.find(columnNameStr) != mColumnList.end())
    {
        ERR_LOG("[HistoryTableDefintion].addColumn: Duplicate column '" << columnName << "' found in table");
        return;
    }
    mColumnList[columnNameStr] = type;
}

void HistoryTableDefinition::addPrimaryKey(const char8_t* keyName)
{
    mPrimaryKeys.push_back(ReportAttributeName(keyName));
}

void HistoryTableDefinition::setHistoryLimit(uint32_t historyLimit)
{
    mHistoryLimit = historyLimit;
}

void HistoryTableDefinition::setDeepestLevel(uint32_t deepestLevel)
{
    mDeepestLevel = deepestLevel;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
GameType::GameType() 
{
    mReferenceTdf = nullptr;
}

GameType::~GameType()
{
    mGREReportRoot.free();

    HistoryTableDefinitions::const_iterator histDefIt = mHistoryTableDefinitions.begin();
    HistoryTableDefinitions::const_iterator histDefItEnd = mHistoryTableDefinitions.end();
    for ( ; histDefIt != histDefItEnd; ++histDefIt )
    {
        delete histDefIt->second;
    }
    mHistoryTableDefinitions.clear();

    histDefIt = mBuiltInTableDefinitions.begin();
    histDefItEnd = mBuiltInTableDefinitions.end();
    for ( ; histDefIt != histDefItEnd; ++histDefIt )
    {
        delete histDefIt->second;
    }
    mBuiltInTableDefinitions.clear();

    histDefIt = mGamekeyTableDefinitions.begin();
    histDefItEnd = mGamekeyTableDefinitions.end();
    for ( ; histDefIt != histDefItEnd; ++histDefIt )
    {
        delete histDefIt->second;
    }
    mGamekeyTableDefinitions.clear();
}

bool operator==(const GameType::StatUpdate& lhs, const GameType::StatUpdate& rhs)
{
    //  comparing pointers (Config) for object equivalence is a bit risky.  configs come from game types, which on reconfiguration are trashed.
    return (lhs.Name == rhs.Name && lhs.Config == rhs.Config);
}

bool GameType::EntityStatUpdate::operator < (const GameType::EntityStatUpdate& other) const
{
    const int32_t result = blaze_strcmp(statUpdate.Config->getCategory(), other.statUpdate.Config->getCategory());
    if (result != 0)
        return (result < 0);

    if (entityId != other.entityId)
        return (entityId < other.entityId);

    return false;
}

bool GameType::getStatUpdate(const GameTypeReportConfig& reportConfig, const char8_t* statUpdateName, StatUpdate& update) const
{
    //  search the reportConfig stat updates section for the update 
    //GameTypeReportConfig::StatUpdatesMap::const_iterator cit = reportConfig.getStatUpdates().find(ReportAttributeName(statUpdateName));

    //  NOTE THIS SLOW COMPARE IS DUE TO HOW THE CONFIGDECODER DECODES CONFIGS INTO MAPS - NOT SORTING THE VECTOR_MAP.
    //  MUST RESORT TO SIMPLE FIND STRING IN MAP UNTIL FIXED.
    GameTypeReportConfig::StatUpdatesMap::const_iterator cit = reportConfig.getStatUpdates().begin();
    for (; cit != reportConfig.getStatUpdates().end(); ++cit)
    {
        if (blaze_strcmp(cit->first.c_str(),statUpdateName)==0)
            break;
    }
    if (cit == reportConfig.getStatUpdates().end())
        return false;
    update.Config = cit->second;
    update.Name = cit->first;
    return true;
}

bool GameType::init(const GameTypeConfig& gameTypeConfig, const GameManager::GameReportName& gameReportName)
{
    mGameReportName = gameReportName;
    updateConfigReferences(gameTypeConfig);

    const GameTypeReportConfig& report = gameTypeConfig.getReport();

    //  parse reportTdf class name - use to construct reference report TDF for variable TDF members.
    mReferenceTdf = createReportTdf(report.getReportTdf());
    if (mReferenceTdf == nullptr)
    {
        ERR_LOG("[GameType].init: failed to createReportTdf for gametype(" << mGameReportName.c_str() << ")");
        return false;
    }

    //  descend report configuration to generate parse tokens for each config node.
    if (!mGREReportRoot.build(*this, &report))
    {
        ERR_LOG("[GameType].init: failed to create GRE cache for gametype(" << mGameReportName.c_str() << ")");
        return false;
    }

    //  visit with an empty context
    GameReportContext& context = ReportTdfVisitor::createReportContext(nullptr);
    context.tdf = mReferenceTdf;    
    bool success = ReportTdfVisitor::visit(report, mGREReportRoot, *mReferenceTdf, context, *mReferenceTdf);
    ReportTdfVisitor::flushReportContexts();

    //  ensure that stat update list has 1-1 relationship between GameTypeReportConfig and GRECacheReport
    //  print out warning if discrepancies in the order and cardinality are found
    if (report.getStatUpdates().size() != mGREReportRoot.mStatUpdates.size())
    {
        WARN_LOG("[GameType].init: Discrepancies found between config and cache report - the number of stat updates in game type '" <<
            mGameReportName.c_str() << "'");
    }
    else
    {
        GameTypeReportConfig::StatUpdatesMap::const_iterator cit = report.getStatUpdates().begin();
        GameTypeReportConfig::StatUpdatesMap::const_iterator citEnd = report.getStatUpdates().end();
        GRECacheReport::StatUpdates::const_iterator citGre = mGREReportRoot.mStatUpdates.begin();
        GRECacheReport::StatUpdates::const_iterator citGreEnd = mGREReportRoot.mStatUpdates.end();

        for (; cit != citEnd && citGre != citGreEnd; ++cit, ++citGre)
        {
            if (blaze_stricmp(cit->first.c_str(), citGre->first.c_str()) != 0)
            {
                WARN_LOG("[GameType].init: Discrepancies found between config and cache report - the order of stat update in game type '"
                    << mGameReportName.c_str() << "'");
                break;
            }

            const GameTypeReportConfig::StatUpdate* statUpdate = cit->second;
            const GRECacheReport::StatUpdate* greStatUpdate = citGre->second;

            if (statUpdate->getStats().size() != greStatUpdate->mStats.size())
            {
                WARN_LOG("[GameType].init: Discrepancies found between config and cache report - the number of stats under stat update '" <<
                    cit->first.c_str() << "' in game type '" << mGameReportName.c_str() << "'");
                break;
            }

            GameTypeReportConfig::StatUpdate::StatsMap::const_iterator citStat = statUpdate->getStats().begin();
            GameTypeReportConfig::StatUpdate::StatsMap::const_iterator citStatEnd = statUpdate->getStats().end();
            GRECacheReport::StatUpdate::Stats::const_iterator citGreStat = greStatUpdate->mStats.begin();
            GRECacheReport::StatUpdate::Stats::const_iterator citGreStatEnd = greStatUpdate->mStats.end();

            for (; citStat != citStatEnd && citGreStat != citGreStatEnd; ++citStat, ++citGreStat)
            {
                if (blaze_stricmp(citStat->first.c_str(), citGreStat->first.c_str()) != 0)
                {
                    WARN_LOG("[GameType].init: Discrepancies found between config and cache report - the order of stats under stat update '" <<
                        cit->first.c_str() << "' in game type '" << mGameReportName.c_str() << "'");
                    break;
                }
            }
        }
    }

    if (!mHistoryTableDefinitions.empty())
    {
        initBuiltInTableDefinitions();
    }

    if (mGamekeyTableDefinitions.empty())
    {
        initGamekeyTableDefinitions();
    }

    return success;
}

void GameType::updateConfigReferences(const GameTypeConfig& gameTypeConfig)
{
    mConfig = &gameTypeConfig;
    mGREReportRoot.mReportConfig = &(gameTypeConfig.getReport());
}

void GameType::initBuiltInTableDefinitions()
{
    //  create built-in participant table
    HistoryTableDefinition* participantTable = BLAZE_NEW HistoryTableDefinition();
    participantTable->addColumn("entity_id", HistoryTableDefinition::ENTITY_ID);
    participantTable->addPrimaryKey("entity_id");
    mBuiltInTableDefinitions["participant"] = participantTable;

    //  create built-in submitter table
    HistoryTableDefinition* submitterTable = BLAZE_NEW HistoryTableDefinition();
    submitterTable->addColumn("entity_id", HistoryTableDefinition::ENTITY_ID);
    submitterTable->addPrimaryKey("entity_id");
    mBuiltInTableDefinitions["submitter"] = submitterTable;
}

void GameType::initGamekeyTableDefinitions()
{
    // create built-in gamekey table
    HistoryTableDefinition* gamekeyTable = BLAZE_NEW HistoryTableDefinition();
    gamekeyTable->addColumn("game_id", HistoryTableDefinition::ENTITY_ID);
    gamekeyTable->addColumn("online", HistoryTableDefinition::INT);
    gamekeyTable->addColumn("flags", HistoryTableDefinition::INT);
    gamekeyTable->addColumn("flag_reason", HistoryTableDefinition::STRING);
    gamekeyTable->addPrimaryKey("game_id");
    mGamekeyTableDefinitions["gamekey"] = gamekeyTable;
}

bool GameType::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{
    //  construct TDF based on config.
    VisitorState& state = mStateStack.top();
    const GameTypeReportConfig& config = *state.config;

    const char8_t* memberName = nullptr;
    if (!parentTdf.getMemberNameByTag(tag, memberName))
        return false;

    GameTypeReportConfig::SubreportsMap::const_iterator subreportIt = config.getSubreports().find(memberName);
    if (subreportIt != config.getSubreports().end())
    {
        const GameTypeReportConfig& subReportConfig = *(subreportIt->second);
        EA::TDF::TdfId tdfid = EA::TDF::TdfFactory::get().getTdfIdFromName(subReportConfig.getReportTdf());
        if (tdfid != EA::TDF::INVALID_TDF_ID)
        {
            value.create(tdfid);
            if (value.isValid())
            {
                //  dig into the created tdf for other subreports to create.
                value.get()->visit(*this, parentTdf, *value.get());
                return true;
            }
            else
            {
                ERR_LOG("[GameType].visit(): [" << mGameReportName.c_str() << "] Failed to create variable subreport EA::TDF::Tdf " << subReportConfig.getReportTdf() << "contained within subreport" << memberName << ".");    
                return false;
            }
        }
        ERR_LOG("[GameType].visit(): [" << mGameReportName.c_str() << "] EA::TDF::Tdf " << subReportConfig.getReportTdf() << " for variable member not registered.");    
    }

    return false;
}

//  when creating the GameType report EA::TDF::Tdf, initialize the TDF containers with at least one member so the ReportTdfVisitor can visit into
//  these members.
void GameType::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    value.initVector(1);
}

void GameType::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    value.initMap(1);
}

EA::TDF::Tdf* GameType::createReportTdf(const char8_t* tdfName)
{
    //  parse reportTdf class name - use to construct reference report TDF for variable TDF members.
    EA::TDF::TdfId reportTdfId = EA::TDF::TdfFactory::get().getTdfIdFromName(tdfName);
    if (reportTdfId == EA::TDF::INVALID_TDF_ID)
    {
        ERR_LOG("[GameType].createReportTdf: invalid report TDF specified in configuration : " << tdfName);
        return nullptr;
    }
    EA::TDF::Tdf* result = EA::TDF::TdfFactory::get().create(reportTdfId);
    if (result == nullptr)
    {
        ERR_LOG("[GameType].createReportTdf: unable to create report TDF specified in configuration : " << tdfName);        
    }

    return result;
}

//  parse the config section for any remaining setup for the game type.
//      - game history tables
bool GameType::visitGameHistory(const GameTypeReportConfig::GameHistoryList& gameHistoryConfigs, const GRECacheReport::GameHistoryList& greGameHistoryList, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    //  only process the first iteration (the configuration only needs to process a single element of the container.)
    if (context.containerIteration > 0)
        return true;

    // ============================================================
    //  ADD GAME HISTORY TABLES
    // ============================================================
    GameTypeReportConfig::GameHistoryList::const_iterator citHistoryConfig = gameHistoryConfigs.begin();
    GameTypeReportConfig::GameHistoryList::const_iterator citHistoryConfigEnd = gameHistoryConfigs.end();
    for ( ; citHistoryConfig != citHistoryConfigEnd; ++citHistoryConfig )
    {
        // find existing table or construct a new one.
        bool newHistoryTable = false;
        const GameTypeReportConfig::GameHistory& historyTableConfig = **citHistoryConfig;
        HistoryTableDefinitions::iterator itHistoryTableDef = mHistoryTableDefinitions.find(historyTableConfig.getTable());
        
        if (itHistoryTableDef == mHistoryTableDefinitions.end())
        {
            eastl::pair<HistoryTableDefinitions::iterator, bool> inserter = mHistoryTableDefinitions.insert(
                HistoryTableDefinitions::value_type(GameTypeReportConfig::GameHistory::TableName(historyTableConfig.getTable()), nullptr)
                );            
            itHistoryTableDef = inserter.first;
            itHistoryTableDef->second = BLAZE_NEW HistoryTableDefinition();
            
            newHistoryTable = true;                 // need this to verify that primary keys are only set up for the initial table definition.
        }

        HistoryTableDefinition& historyTableDef = *itHistoryTableDef->second;
        const char8_t* historyTableName = itHistoryTableDef->first.c_str();

        //  set the limit of rows per entity in this game history table
        historyTableDef.setHistoryLimit(historyTableConfig.getHistoryLimit());

        if (context.level > historyTableDef.getDeepestLevel())
            historyTableDef.setDeepestLevel(context.level);

        //  insert columns
        //      determine column types based on the column configuration "value expression".
        //      evaluate the config's "value expression", returning type (value isn't important for definition purposes)
        typedef GameTypeReportConfig::ReportValueByAttributeMap ConfigColumnsMap;
        ConfigColumnsMap::const_iterator citConfigColumn = historyTableConfig.getColumns().begin();
        ConfigColumnsMap::const_iterator citConfigColumnEnd  = historyTableConfig.getColumns().end();
        
        for (; citConfigColumn != citConfigColumnEnd; ++citConfigColumn)
        {
            const char8_t* columnName = citConfigColumn->first.c_str();
            
            EA::TDF::TdfGenericValue columnValue;
            GameReportExpression exp(*this, citConfigColumn->second);
            exp.evaluate(columnValue, context);

            if (exp.getResultCode() == GameReportExpression::ERR_OK || exp.getResultCode() == GameReportExpression::ERR_NO_RESULT)
            {        
                HistoryTableDefinition::ColumnType colType;

                switch (columnValue.getType())
                {
                case EA::TDF::TDF_ACTUAL_TYPE_BOOL:
                case EA::TDF::TDF_ACTUAL_TYPE_UINT8:
                case EA::TDF::TDF_ACTUAL_TYPE_UINT16:
                case EA::TDF::TDF_ACTUAL_TYPE_UINT32:                
                    colType = HistoryTableDefinition::UINT;
                    break;

                case EA::TDF::TDF_ACTUAL_TYPE_INT8:
                case EA::TDF::TDF_ACTUAL_TYPE_INT16:
                case EA::TDF::TDF_ACTUAL_TYPE_INT32:
                case EA::TDF::TDF_ACTUAL_TYPE_ENUM:
                    colType = HistoryTableDefinition::INT;
                    break;

                case EA::TDF::TDF_ACTUAL_TYPE_INT64:
                case EA::TDF::TDF_ACTUAL_TYPE_UINT64:
                    colType = HistoryTableDefinition::ENTITY_ID;        
                    break;

                case EA::TDF::TDF_ACTUAL_TYPE_FLOAT:
                    colType = HistoryTableDefinition::FLOAT;
                    break;

                case EA::TDF::TDF_ACTUAL_TYPE_STRING:
                    colType = HistoryTableDefinition::STRING;
                    break;
                default:
                    colType = HistoryTableDefinition::NUM_ATTRIBUTE_TYPES;                 
                    break;             
                }

                if (colType == HistoryTableDefinition::NUM_ATTRIBUTE_TYPES)
                {
                    ERR_LOG("[GameType].visitGameHistory() Type [" << mGameReportName.c_str() << "], column type for column '" 
                            << columnName << "' is invalid for table '" << historyTableName << "'.");
                }
                else
                {
                    historyTableDef.addColumn(columnName, colType);
                }
            }
        }

        //  now that columns have been defined, add primary keys (which are keyed off column name)
        typedef GameTypeReportConfig::GameHistory::StringReportAttributeNameList ConfigPrimaryKeyList;
        ConfigPrimaryKeyList::const_iterator citPrimaryKey = historyTableConfig.getPrimaryKey().begin();
        ConfigPrimaryKeyList::const_iterator citPrimaryKeyEnd = historyTableConfig.getPrimaryKey().end();

        //  if this definition adds to an existing table definition - we're not going to allow additional primary key definitions (at least for now.)
        //  in theory there shouldn't be a problem lifting that restriction.
        if (!newHistoryTable && citPrimaryKey != citPrimaryKeyEnd)
        {
            ERR_LOG("[GameType].visitGameHistory() Type [" << mGameReportName.c_str() << "], primary keys must be defined only in the first table definition for table '" 
                    << historyTableName << "'.");
        }
        else
        {
            for (; citPrimaryKey != citPrimaryKeyEnd; ++citPrimaryKey)
            {
                const ReportAttributeName& primaryKeyStr = *citPrimaryKey;

                const HistoryTableDefinition::ColumnList& tableColumnList = historyTableDef.getColumnList();
                if (tableColumnList.find(primaryKeyStr) == tableColumnList.end())
                {
                    ERR_LOG("[GameType].visitGameHistory() Type [" << mGameReportName.c_str() << "], error finding primary key column '" 
                            << primaryKeyStr.c_str() << "' in table '" << historyTableName << "'.");

                    return false;
                }
                else
                {
                    historyTableDef.addPrimaryKey(primaryKeyStr.c_str());
                }
            }    
        }
    }

    return true;
}

//  validate config's metric updates are integer type (to tally totals/avgs)
bool GameType::visitMetricUpdates(const GameTypeReportConfig::ReportValueByAttributeMap& metricUpdates, const GRECacheReport::MetricUpdates& greMetricUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    GameTypeReportConfig::ReportValueByAttributeMap::const_iterator cit = metricUpdates.begin();
    GameTypeReportConfig::ReportValueByAttributeMap::const_iterator citEnd = metricUpdates.end();
    if (cit == citEnd)
        return true;

    for (; cit != citEnd; ++cit)
    {
        const char8_t *metricName = cit->first.c_str();
        GameReportExpression& expr = *(greMetricUpdates.find(cit->first)->second);
        EA::TDF::TdfGenericValue result;
        if (!expr.evaluate(result, context, false))
        {
            ERR_LOG("[GameType].visitMetricUpdates: expression parse failed for metric update='" 
                << metricName << "', expr='" << cit->second.c_str() << "'");
            return false;
        }
        else
        {
            TRACE_LOG("[GameType].visitMetricUpdates: metric update " << metricName << "=" << cit->second.c_str() << ".");
            if (!result.getTypeDescription().isIntegral())
            {
                ERR_LOG("[GameType].visitMetricUpdates: metric update " << metricName << "=" << cit->second.c_str() << ", value must be an integer");
                return false;
            }
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

GameTypeCollection::~GameTypeCollection()
{
    clear();
}


//  init() expects a configmap with a list of game types.
bool GameTypeCollection::init(const GameTypeConfigMap& gameTypesConfigMap)
{
    GameTypeConfigMap::const_iterator itConfig = gameTypesConfigMap.begin();
    GameTypeConfigMap::const_iterator itConfigEnd = gameTypesConfigMap.end();

    for (;itConfig != itConfigEnd; ++itConfig)
    {
        GameManager::GameReportName gameReportName = itConfig->first;
        const char8_t* typeName = gameReportName.c_str();

        if (mGameTypeMap.find(gameReportName) == mGameTypeMap.end())
        {
            const GameTypeConfig* gameTypeConfig = itConfig->second;

            //  create a new game type
            GameType *gameType = BLAZE_NEW GameType();
            if (!gameType->init(*gameTypeConfig, gameReportName))
            {
                //  invalid game type config - abort
                ERR_LOG("[GameTypeCollection].init: Game type configuration for '" << typeName << "' is invalid.  Aborting configuration.");
                return false;
            }

            mGameTypeMap.insert(eastl::make_pair(gameReportName, gameType));

            INFO_LOG("[GameTypeCollection].init: Registered game type '" << typeName << "'");
        }
    }

    /*
    //  read in game history tables
    GameTypeMap::const_iterator gameTypeItEnd = mGameTypeMap.end();
    for (GameTypeMap::const_iterator gameTypeIt = mGameTypeMap.begin(); gameTypeIt != gameTypeItEnd; ++gameTypeIt)
    {
        GameType& gameType = *(gameTypeIt->second);
        gameType.defineHistoryTables();
    }
    */

    return true;
}


void GameTypeCollection::clear()
{
    //  wipe out game types from the Id map
    GameTypeMap::iterator citEnd = mGameTypeMap.end();
    for (GameTypeMap::iterator cit = mGameTypeMap.begin(); cit != citEnd; ++cit)
    {
        delete cit->second;
    }
    mGameTypeMap.clear();
}
void GameTypeCollection::updateConfigReferences(const GameTypeConfigMap& gameTypesConfigMap)
{
    GameTypeConfigMap::const_iterator itConfig = gameTypesConfigMap.begin();
    GameTypeConfigMap::const_iterator itConfigEnd = gameTypesConfigMap.end();
    for (;itConfig != itConfigEnd; ++itConfig)
    {
        GameManager::GameReportName gameReportName = itConfig->first;
        GameTypeMap::const_iterator it = mGameTypeMap.find(gameReportName);
        if (it != mGameTypeMap.end())
        {
            const GameTypeConfig* gameTypeConfig = itConfig->second;
            GameType *gameType = it->second;
            gameType->updateConfigReferences(*gameTypeConfig);
        }
    }
}

//  query methods
const GameType *GameTypeCollection::getGameType(const GameManager::GameReportName& gameReportName) const
{
    GameTypeMap::const_iterator gameTypeIt = mGameTypeMap.find(gameReportName);
    if (gameTypeIt == mGameTypeMap.end())
    {
        return nullptr;
    }
    
    return gameTypeIt->second;
}

} //namespace Blaze
} //namespace GameReporting
