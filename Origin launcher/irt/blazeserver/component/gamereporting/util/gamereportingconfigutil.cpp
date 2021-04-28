/*************************************************************************************************/
/*!
    \file   gamereportingconfigutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "framework/tdf/controllertypes_server.h"
#include "gamereporting/util/collatorutil.h"

#include "gamereporting/util/gamereportingconfigutil.h"
#include "gamereporting/gamehistoryconfig.h"
#include "gamereporting/gametype.h"

// stat includes
#include "stats/statscommontypes.h"

#include "framework/database/dbscheduler.h"

namespace Blaze
{
namespace GameReporting
{

GameReportingConfigUtil::GameReportingConfigUtil(GameTypeCollection& gameTypeCollection, GameHistoryConfigParser& gameHistoryConfig)
    : mGameTypeCollection(gameTypeCollection), mGameHistoryConfig(gameHistoryConfig)
{
}


void GameReportingConfigUtil::validateGameHistory(const GameHistoryConfig& config, ConfigureValidationErrors& validationErrors)
{
    if (config.getLowFrequentPurgeInterval() == INVALID_TIMER_ID)
    {
        eastl::string msg;
        msg.sprintf("[GameReportingConfigUtil].validateGameHistory(): low frequent purging interval can not be set to 0.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
    if (config.getHighFrequentPurgeInterval() == INVALID_TIMER_ID)
    {
        eastl::string msg;
        msg.sprintf("[GameReportingConfigUtil].validateGameHistory(): high frequent purging interval can not be set to 0.");
        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
        str.set(msg.c_str());
    }
}

void GameReportingConfigUtil::validateReportGameHistory(const char8_t* gameReportName, const GameTypeReportConfig& config, const GameReportContext& context, eastl::list<const char8_t*>& gameHistoryTables, ConfigureValidationErrors& validationErrors)
{
    for (GameTypeReportConfig::GameHistoryList::const_iterator it = config.getGameHistory().begin(); it != config.getGameHistory().end(); ++it)
    {
        bool alreadyDefined = false;
        for (eastl::list<const char8_t*>::iterator tableIt = gameHistoryTables.begin(); tableIt != gameHistoryTables.end(); ++tableIt)
        {
            if (blaze_strcmp((*it)->getTable(), *tableIt) == 0)
            {
                alreadyDefined = true;
                break;
            }
        }
        if (!alreadyDefined)
            gameHistoryTables.push_back((*it)->getTable());
        for (GameTypeReportConfig::ReportValueByAttributeMap::const_iterator columnIt = (*it)->getColumns().begin(); columnIt != (*it)->getColumns().end(); ++columnIt)
        {
            const char8_t* columnName = columnIt->first.c_str();

            EA::TDF::TdfGenericValue columnValue;
            GameReportExpression exp(gameReportName, columnIt->second);
            exp.evaluate(columnValue, context, false);

            if (exp.getResultCode() == GameReportExpression::ERR_OK || exp.getResultCode() == GameReportExpression::ERR_NO_RESULT)
            {
                if (!columnValue.getTypeDescription().isIntegral() && !columnValue.getTypeDescription().isFloat() && !columnValue.getTypeDescription().isString())
                {
                    eastl::string msg;
                    msg.sprintf("[GameReportingConfigUtil].validateReportGameHistory() : Type [%s], column type for column '%s' is invalid for table '%s'.",
                        gameReportName, columnName, (*it)->getTable());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
        if (alreadyDefined && !(*it)->getPrimaryKey().empty())
        {
            eastl::string msg;
            msg.sprintf("[GameReportingConfigUtil].validateReportGameHistory() : Type [%s], primary keys must be defined only in the first table definition for table '%s'.",
                gameReportName, (*it)->getTable());
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            for (GameTypeReportConfig::GameHistory::StringReportAttributeNameList::const_iterator keyIt = (*it)->getPrimaryKey().begin(); keyIt != (*it)->getPrimaryKey().end(); ++keyIt)
            {
                if ((*it)->getColumns().find(*keyIt) == (*it)->getColumns().end())
                {
                    eastl::string msg;
                    msg.sprintf("[GameReportingConfigUtil].validateReportGameHistory() : Type [%s], error finding primary key column '%s' in table '%s'.",
                        gameReportName, keyIt->c_str(), (*it)->getTable());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
    }
}

void GameReportingConfigUtil::validateReportStatsServiceConfig(const GameTypeReportConfig& config, ConfigureValidationErrors& validationErrors)
{
    for (GameTypeReportConfig::StatsServiceUpdatesMap::const_iterator it = config.getStatsServiceUpdates().begin(); it != config.getStatsServiceUpdates().end(); ++it)
    {
        for (GameTypeReportConfig::StatsServiceUpdate::DimensionalStatsMap::const_iterator dimMapIt = (*it).second->getDimensionalStats().begin();
            dimMapIt != (*it).second->getDimensionalStats().end(); ++dimMapIt)
        {
            for (auto dimMapListIt = (*dimMapIt).second->begin();
                dimMapListIt != (*dimMapIt).second->end(); ++dimMapListIt)
            {
                GameTypeReportConfig::StatsServiceUpdate::DimensionalStat* stat = (*dimMapListIt);
                if (stat->getDimensions().mapSize() == 0)
                {
                    eastl::string msg;
                    msg.sprintf("[GameReportingConfigUtil].validateReportStatsServiceConfig(): Empty dimensions in config for stat (%s)", (*dimMapIt).first.c_str());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
    }
}

void GameReportingConfigUtil::validateGameTypeReportConfig(const GameTypeReportConfig& config, ConfigureValidationErrors& validationErrors)
{
    validateReportStatsServiceConfig(config, validationErrors);
    // visit subreports
    const GameTypeReportConfig::SubreportsMap& subreports = config.getSubreports();
    for (auto subreportIt = subreports.begin(); subreportIt != subreports.end(); ++subreportIt)
    {
        validateGameTypeReportConfig(*subreportIt->second, validationErrors);
    }
}

void GameReportingConfigUtil::validateGameTypeConfig(const GameTypeConfigMap& config, ConfigureValidationErrors& validationErrors)
{
    for (GameTypeConfigMap::const_iterator it = config.begin(); it != config.end(); ++it)
    {
        const char8_t* gameReportName = it->first.c_str();

        bool alreadyDefined = false;
        for (GameTypeConfigMap::const_iterator nameIt = config.begin(); nameIt != it; ++nameIt)
        {
            if (blaze_strcmp(gameReportName, nameIt->first.c_str()) == 0)
            {
                alreadyDefined = true;
                WARN_LOG("[GameReportingConfigUtil].validateGameTypeConfig() : Game type '" << gameReportName << "' already defined.  Ignoring...");
                break;
            }
        }
        if (!alreadyDefined)
        {
            EA::TDF::TdfPtr referenceTdf = GameType::createReportTdf(it->second->getReport().getReportTdf());
            if (referenceTdf == nullptr)
            {
                eastl::string msg;
                msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingQueryConfig(): Failed to createReportTdf for gametype(%s)", gameReportName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            else
            {
                GameReportContext* context = BLAZE_NEW GameReportContext;
                context->tdf = referenceTdf;
                eastl::list<const char8_t*> gameHistoryTables;
                validateReportGameHistory(gameReportName, it->second->getReport(), *context, gameHistoryTables, validationErrors);
                gameHistoryTables.clear();
                delete context;

                validateReportStatsServiceConfig(it->second->getReport(), validationErrors);
                // visit subreports for stats service
                const GameTypeReportConfig::SubreportsMap& subreports = it->second->getReport().getSubreports();
                for (auto subreportIt = subreports.begin(); subreportIt != subreports.end(); ++subreportIt)
                {
                    validateGameTypeReportConfig(*subreportIt->second, validationErrors);
                }
            }
        }
    }
}

void GameReportingConfigUtil::validateGameHistoryReportingQueryConfig(const GameHistoryReportingQueryList& config, const GameTypeConfigMap& gameTypeConfig, ConfigureValidationErrors& validationErrors)
{
    for(GameHistoryReportingQueryList::const_iterator it = config.begin(); it != config.end(); ++it)
    {
        const char8_t* queryName = (*it)->getName();
        for (GameHistoryReportingQueryList::const_iterator nameIt = config.begin(); nameIt != it; ++nameIt)
        {
            if (blaze_strcmp(queryName, (*nameIt)->getName()) == 0)
            {
                eastl::string msg;
                msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingQueryConfig(): Duplicate game report query name %s", queryName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                break;
            }
        }

        const char8_t* typeName = (*it)->getTypeName();
        GameTypeConfigMap::const_iterator typeNameIt = gameTypeConfig.begin();
        for (; typeNameIt != gameTypeConfig.end(); ++typeNameIt)
        {
            if (blaze_strcmp(typeName, typeNameIt->first.c_str()) == 0)
                break;
        }
        if (typeNameIt == gameTypeConfig.end())
        {
            eastl::string msg;
            msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingQueryConfig(): Invalid query '%s'. Game type '%s' is not defined",
                queryName, typeName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            const GameTypeReportConfig::GameHistory* gameHistory;
            if (!findGameHistoryTable(typeNameIt->second->getReport(), nullptr, gameHistory))
            {
                eastl::string msg;
                msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingQueryConfig(): Invalid query '%s'. No game history table defined for game type '%s'",
                    queryName, typeName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            else
            {
                for(GameHistoryReportingFilterList::const_iterator filterIt = (*it)->getFilters().begin(); filterIt != (*it)->getFilters().end(); ++filterIt)
                {
                    validateGameHistoryReportingFilter(**filterIt, typeNameIt->first.c_str(), *(typeNameIt->second), true, validationErrors);
                }
                for(GameHistoryReportingQueryColumnList::const_iterator columnIt = (*it)->getColumns().begin(); columnIt != (*it)->getColumns().end(); ++columnIt)
                {
                    validateGameHistoryReportingColumn((*columnIt)->getName(), (*columnIt)->getTable(), (*columnIt)->getIndex(), typeNameIt->first.c_str(), *(typeNameIt->second), validationErrors);
                }
            }
        }
    }
}

void GameReportingConfigUtil::validateGameHistoryReportingViewConfig(const GameHistoryReportingViewList& config, const GameTypeConfigMap& gameTypeConfig, ConfigureValidationErrors& validationErrors)
{
    for(GameHistoryReportingViewList::const_iterator it = config.begin(); it != config.end(); it++)
    {
        const char8_t* viewName = (*it)->getName();
        for (GameHistoryReportingViewList::const_iterator nameIt = config.begin(); nameIt != it; ++nameIt)
        {
            if (blaze_strcmp(viewName, (*nameIt)->getName()) == 0)
            {
                eastl::string msg;
                msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingViewConfig(): Duplicate game report query name %s", viewName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
                break;
            }
        }

        const char8_t* typeName = (*it)->getTypeName();
        GameTypeConfigMap::const_iterator typeNameIt = gameTypeConfig.begin();
        for (; typeNameIt != gameTypeConfig.end(); ++typeNameIt)
        {
            if (blaze_strcmp(typeName, typeNameIt->first.c_str()) == 0)
                break;
        }
        if (typeNameIt == gameTypeConfig.end())
        {
            eastl::string msg;
            msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingViewConfig(): Invalid query '%s'. Game type '%s' is not defined",
                viewName, typeName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        else
        {
            const GameTypeReportConfig::GameHistory* gameHistory;
            if (!findGameHistoryTable(typeNameIt->second->getReport(), nullptr, gameHistory))
            {
                eastl::string msg;
                msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingViewConfig(): Invalid query '%s'. No game history table defined for game type '%s'",
                    viewName, typeName);
                EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                str.set(msg.c_str());
            }
            else
            {
                const char8_t* rowAttributeType = (*it)->getRowAttributeType();
                if (blaze_strcmp(rowAttributeType, "game") != 0)
                {
                    const GameTypeReportConfig::GameHistory* gameHistory2;
                    if (!findGameHistoryTable(typeNameIt->second->getReport(), rowAttributeType, gameHistory2))
                    {
                        eastl::string msg;
                        msg.sprintf("[GameReportingConfigUtil].validateGameHistoryReportingViewConfig(): Invalid attribute type for game report view %s", viewName);
                        EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                        str.set(msg.c_str());
                    }
                }

                for(GameHistoryReportingFilterList::const_iterator filterIt = (*it)->getFilters().begin(); filterIt != (*it)->getFilters().end(); ++filterIt)
                {
                    validateGameHistoryReportingFilter(**filterIt, typeNameIt->first.c_str(), *(typeNameIt->second), false, validationErrors);
                }
                for(GameHistoryReportingViewColumnList::const_iterator columnIt = (*it)->getColumns().begin(); columnIt != (*it)->getColumns().end(); ++columnIt)
                {
                    validateGameHistoryReportingColumn((*columnIt)->getName(), (*columnIt)->getTable(), (*columnIt)->getIndex(), typeNameIt->first.c_str(), *(typeNameIt->second), validationErrors);
                    validateGameHistoryReporingColumnUserCoreIdentName((*columnIt)->getEntityType(), (*columnIt)->getUserCoreIdentName(), validationErrors);
                    const char8_t* type = (*columnIt)->getType();
                    static const char8_t* TYPE_INT = "int";
                    static const char8_t* TYPE_FLOAT = "float";
                    static const char8_t* TYPE_STRING = "string";
                    Stats::StatType typeEnum = Stats::STAT_TYPE_INT;
                    if (blaze_strncmp(type, TYPE_INT, sizeof(TYPE_INT)) == 0)
                    {
                        typeEnum = Stats::STAT_TYPE_INT;
                    }
                    else if (blaze_strncmp(type, TYPE_FLOAT, sizeof(TYPE_FLOAT)) == 0)
                    {
                        typeEnum = Stats::STAT_TYPE_FLOAT;
                    }
                    else if (blaze_strncmp(type, TYPE_STRING, sizeof(TYPE_STRING)) == 0)
                    {
                        typeEnum = Stats::STAT_TYPE_STRING;
                    }
                    else
                    {
                        typeEnum = Stats::STAT_TYPE_INT;
                    }
                    const char8_t* format = (*columnIt)->getFormat();
                    const int32_t typeSize = 4; //support int32
                    char8_t recreateFormat[Blaze::Stats::STATS_FORMAT_LENGTH];
                    if (!Stats::validateFormat(typeEnum, format, typeSize, recreateFormat, sizeof(recreateFormat)))
                    {
                        WARN_LOG("[GameReportingConfigUtil].validateGameHistoryReportingViewConfig() : Format of " << (*columnIt)->getName()
                            << " based on type " << type << " is not valid, default format will be appllied");
                    }
                }
            }
        }
    }
}

void GameReportingConfigUtil::validateSkillInfoConfig(const SkillInfoConfig& config, ConfigureValidationErrors& validationErrors)
{
    const DampingTableList &dampingTableList = config.getDampingTables();
    for (DampingTableList::const_iterator it = dampingTableList.begin(); it != dampingTableList.end(); it++)
    {
        const char8_t* tableName = (*it)->getName();
        if(tableName == nullptr)
        {
            eastl::string msg;
            msg.sprintf("[GameReportingConfigUtil].validateSkillInfoConfig(): SkillDampingTable missing name parameter.", tableName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
        }
        DampingTableList::const_iterator it2 = dampingTableList.begin();
        for (; it2 != it; ++it2)
        {
            if (blaze_stricmp(tableName, (*it2)->getName()) == 0)
                break;
        }
        if(it2 != it)
        {
            eastl::string msg;
            msg.sprintf("[GameReportingConfigUtil].validateSkillInfoConfig(): skill damping table \"%s\" is duplicate name.", tableName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
        }
        const SkillDampingTable& dampingTable = (*it)->getTable();
        if(dampingTable.empty())
        {
            eastl::string msg;
            msg.sprintf("[GameReportingConfigUtil].validateSkillInfoConfig(): skill damping table \"%s\" is empty.", tableName);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
        }
    }
}


void GameReportingConfigUtil::validateCustomEventConfig(const EventTypes& eventTypes, ConfigureValidationErrors& validationErrors)
{
    for (EventTypes::const_iterator i = eventTypes.begin(), e = eventTypes.end(); i != e; ++i)
    {
        const EventType* eventType = *i;

        bool alreadyDefined = false;
        for (EventTypes::const_iterator nameIt = eventTypes.begin(); nameIt != i; ++nameIt)
        {
            const EventType* eventTypeTemp = *nameIt;
            if (blaze_strcmp(eventType->getName(), eventTypeTemp->getName()) == 0)
            {
                alreadyDefined = true;
                WARN_LOG("[GameReportingConfigUtil].validateCustomEventConfig() : Event type '" << eventType->getName() << "' already defined.  Ignoring...");
                break;
            }
        }

        if (!alreadyDefined)
        {
            if (eventType->getTdf() != nullptr && eventType->getTdf()[0] != '\0')
            {
                EA::TDF::TdfPtr referenceTdf = GameType::createReportTdf(eventType->getTdf());
                if (referenceTdf == nullptr)
                {
                    eastl::string msg;
                    msg.sprintf("[GameReportingConfigUtil].validateCustomEventConfig(): Failed to createReportTdf for eventtype(%s)", eventType->getName());
                    EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
                    str.set(msg.c_str());
                }
            }
        }
    }
}

bool GameReportingConfigUtil::init(const GameReportingConfig& gameReportingConfig)
{
    const GameTypeConfigMap& gameTypesMap = gameReportingConfig.getGameTypes();

    //  Parse and cache off GameType information
    if (!mGameTypeCollection.init(gameTypesMap))
    {
        ERR_LOG("[GameReportingConfigUtil].init() : unable to parse game types from configuration.");
        return false;
    }

    //  Parse and cache off slave specific Game History Information
    return mGameHistoryConfig.init(gameReportingConfig, mGameTypeCollection);
}

} // GameReporting
} // Blaze
