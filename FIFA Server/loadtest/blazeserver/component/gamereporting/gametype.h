/*************************************************************************************************/
/*!
    \file   gametype.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_GAMETYPE_H
#define BLAZE_GAMEREPORTING_GAMETYPE_H

#include "gamereportingconfig.h"
#include "gamereporttdf.h"

#include "EASTL/vector_map.h"


namespace Blaze
{
namespace GameReporting
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Defines the DB columns for a game type's game history table.
//
class HistoryTableDefinition
{
public:
    HistoryTableDefinition();

    enum ColumnType 
    {
        INT,
        UINT,
        DEFAULT_INT,
        FLOAT,
        STRING,
        ENTITY_ID,
        NUM_ATTRIBUTE_TYPES
    };

    typedef eastl::vector<ReportAttributeName> PrimaryKeyList;
    typedef eastl::vector_map<ReportAttributeName, ColumnType> ColumnList;
    const ColumnList &getColumnList() const { return mColumnList; }
    const PrimaryKeyList &getPrimaryKeyList() const { return mPrimaryKeys; }
    uint32_t getHistoryLimit() const { return mHistoryLimit; }
    uint32_t getDeepestLevel() const { return mDeepestLevel; }

    void addColumn(const char8_t* columnName, ColumnType type);
    void addPrimaryKey(const char8_t* keyName);
    void setHistoryLimit(uint32_t historyLimit);
    void setDeepestLevel(uint32_t deepestLevel);

private:
    ColumnList mColumnList;
    PrimaryKeyList mPrimaryKeys;
    uint32_t mHistoryLimit;
    uint32_t mDeepestLevel;
};

typedef eastl::vector_map<GameTypeReportConfig::GameHistory::TableName, HistoryTableDefinition *, CaseSensitiveStringLessThan> HistoryTableDefinitions;


////////////////////////////////////////////////////////////////////////////////////////////////////
//  Contains GameType specific information linked to this type.   All GameType objects are owned 
//  by a GameTypeCollection object defined afterwards.
//

class GameType: private ReportTdfVisitor
{
public:
    GameType();
    ~GameType() override;

    bool init(const GameTypeConfig& gameTypeConfig, const GameManager::GameReportName& gameReportName);

    void updateConfigReferences(const GameTypeConfig& gameTypeConfig);

    const GameManager::GameReportName& getGameReportName() const {
        return mGameReportName;
    }
    const bool getArbitraryUserOfflineReportProcessing() const {
        return mConfig->getArbitraryUserOfflineReportProcessing();
    }
    const char8_t* getReportProcessorClass() const {
        return mConfig->getReportProcessorClass();
    }
    const char8_t* getReportCollatorClass() const {
        return mConfig->getReportCollatorClass();
    }
    const char8_t* getCustomDataTdf() const {
        return mConfig->getCustomDataTdf();
    }
    const GameTypeReportConfig& getConfig() const {
        return mConfig->getReport();
    }
    const EA::TDF::Tdf* getCustomConfig() const {
        return mConfig->getCustomConfig();
    }
    const GRECacheReport& getGRECache() const {
        return mGREReportRoot;
    }
    const HistoryTableDefinitions& getHistoryTableDefinitions() const {
        return mHistoryTableDefinitions;
    }
    const HistoryTableDefinitions& getBuiltInTableDefinitions() const {
        return mBuiltInTableDefinitions;
    }
    const HistoryTableDefinitions& getGamekeyTableDefinitions() const {
        return mGamekeyTableDefinitions;
    }

    //  searches for a stat update using the basis reportConfig.  this method does not descent into the config's subreports since it's
    //  assumed the called would have done this themselves.
    struct StatUpdate
    {
        const GameTypeReportConfig::StatUpdate* Config;
        ReportAttributeName Name;
    };
    friend bool operator==(const StatUpdate& lhs, const StatUpdate& rhs);
    //  key for an entity stat update.
    struct EntityStatUpdate
    {
    public:
        EntityStatUpdate();
        EntityStatUpdate(EntityId id, StatUpdate update) : entityId(id), statUpdate(update)
        {
        }

        bool operator < (const EntityStatUpdate& other) const;

        EntityId entityId;
        StatUpdate statUpdate;
    };
    bool getStatUpdate(const GameTypeReportConfig& reportConfig, const char8_t* statUpdateName, StatUpdate& update) const; 

    //  constructed during configuration - used for bootstrapping and other validation purposes.
    static EA::TDF::Tdf* createReportTdf(const char8_t* tdfName);

private:
    //  visit top-level game type of the config subreport
    bool visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports) override { return true; }
    //  visit the list of game history section of the config subreport
    bool visitGameHistory(const GameTypeReportConfig::GameHistoryList& gameHistoryList, const GRECacheReport::GameHistoryList& greGameHistoryList, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of stat updates for the subreport
    bool visitStatUpdates(const GameTypeReportConfig::StatUpdatesMap& statUpdates, const GRECacheReport::StatUpdates& greStatUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }
    //  visit the map of Stats Service updates for the subreport
    bool visitStatsServiceUpdates(const GameTypeReportConfig::StatsServiceUpdatesMap& statsServiceUpdates, const GRECacheReport::StatsServiceUpdates& greStatsServiceUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }
    //  visit the map of report values for the subreport
    bool visitReportValues(const GameTypeReportConfig::ReportValueByAttributeMap& reportValues, const GRECacheReport::ReportValues& greReportValues, const GameReportContext& context, EA::TDF::Tdf& tdfOut) const override { return true; }
    //  visit the map of metric updates for this subreport
    bool visitMetricUpdates(const GameTypeReportConfig::ReportValueByAttributeMap& metricUpdates, const GRECacheReport::MetricUpdates& greMetricUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of events for the subreport
    bool visitEventUpdates(const GameTypeReportConfig::EventUpdatesMap& events, const GRECacheReport::EventUpdates& greEvents, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }
    //  visit the map of achievements for the subreport
    bool visitAchievementUpdates(const GameTypeReportConfig::AchievementUpdates& achievements, const GRECacheReport::AchievementUpdates& greAchievements, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }
    //  visit the PSN Match report update section of the subreport
    bool visitPsnMatchUpdates(const GameTypeReportConfig::PsnMatchUpdatesMap& psnMatchUpdates, const GRECacheReport::PsnMatchUpdates& grePsnMatchUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }

    GameManager::GameReportName mGameReportName;

    //  Reference of configuration for the game type.
    const GameTypeConfig* mConfig;
    GRECacheReport mGREReportRoot;

    //  Definitions of this game type's history tables.
    HistoryTableDefinitions mHistoryTableDefinitions;
    HistoryTableDefinitions mBuiltInTableDefinitions;
    HistoryTableDefinitions mGamekeyTableDefinitions;

    EA::TDF::TdfPtr mReferenceTdf;


private:  
    // if a variable TDF found, construct
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override;
    //  initialize containers
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override;

    void initBuiltInTableDefinitions();
    void initGamekeyTableDefinitions();
};


///////////////////////////////////////////////////////////////////////////////
//  container for all game types defined in gamereporting.cfg
//  
typedef eastl::vector_map<GameManager::GameReportName, GameType *> GameTypeMap;

class GameTypeCollection
{
public:
    //  house keeping.
    bool init(const GameTypeConfigMap& gameTypesConfigMap);
    void clear();
    size_t size() const {
        return mGameTypeMap.size();
    }

    //  query methods
    const GameType *getGameType(const GameManager::GameReportName& gameReportName) const;
    
    //  retrieve all game types in collection - take care when using this method as a schedule fiber call may reconfigure
    //  and possibly invalidate these returned GameTypes.  used to iterate through all game types in the collection.
    //  note getGameTypes() will return an up-to-date game type list - meaning this method isn't a simple getter.  
    const GameTypeMap& getGameTypeMap() const {
        return mGameTypeMap;
    }

    //  after reconfiguration, the config references need to be updated.
    void updateConfigReferences(const GameTypeConfigMap& gameTypesConfigMap);

    ~GameTypeCollection();

private:
    GameTypeMap mGameTypeMap;
};

} //namespace Blaze
} //namespace GameReporting
#endif
