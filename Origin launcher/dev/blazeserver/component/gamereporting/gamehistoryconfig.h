/*************************************************************************************************/
/*!
\file   gamehistoryconfig.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEHISTORY_CONFIG_H
#define BLAZE_GAMEHISTORY_CONFIG_H

/*** Include files *******************************************************************************/
#include "EASTL/list.h"
#include "EASTL/hash_set.h"
#include "EASTL/string.h"

#include "framework/database/preparedstatement.h"

#include "gamereporting/tdf/gamehistory.h"

#include "gametype.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace GameReporting
{

class GameReportQueryConfig;
class GameReportViewConfig;

static const uint32_t MAX_QUERY_LENGTH = 2048;
static const uint32_t MAX_LIMIT = 64;

//  TODO: refactor to use a different container if necessary? **to be deprecated**
typedef eastl::hash_set<const char8_t*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > ColumnSet;
typedef eastl::hash_map<const char8_t*, ColumnSet*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > GameReportColumnSetMapByType;

typedef eastl::hash_set<eastl::string> StringHashSet;
typedef eastl::hash_map<eastl::string, StringHashSet* > StringHashSetMap;

typedef eastl::map<QueryName, GameReportQueryConfig *, CaseSensitiveStringLessThan> GameReportQueryConfigMap;
typedef eastl::map<ViewName, GameReportViewConfig *, CaseSensitiveStringLessThan> GameReportViewConfigMap;

class GameReportQueryConfig
{
public:
    GameReportQueryConfig(const GameReportQuery& query, const GameType* gameType);
    ~GameReportQueryConfig();

    const GameReportQuery* getGameReportQuery() const { return &mQuery; }
    const char8_t* getQueryStringPrependFilter() const { return mQueryStringPrependFilter.c_str(); }
    const char8_t* getQueryStringAppendFilter() const { return mQueryStringAppendFilter.c_str(); }
    const StringHashSet* getColumnSetByTable(const char8_t* table) const;
    const StringHashSet* getRequestedTables() const { return &mRequestedTables; }
    uint32_t getNumOfVars() const { return mNumOfVars; }

private:
    GameReportQuery mQuery;
    eastl::string mQueryStringPrependFilter;
    eastl::string mQueryStringAppendFilter;
    uint32_t mNumOfVars;
    StringHashSet mRequestedTables;
    StringHashSetMap mColumnSetByTable;
};

class GameReportQueriesCollection
{
public:
    bool init(const GameReportingConfig& config, const GameTypeMap& gameTypeMap);
    bool parseGameReportColumns(const GameHistoryReportingQueryColumnList& gameHistoryReportingQueryColumnList,
    GameReportQuery::GameReportColumnKeyList& columnKeyList, const GameType *gameType) const;

    void clearGameReportQueryConfigMap();

    ~GameReportQueriesCollection()
    {
        clearGameReportQueryConfigMap();
    }
    const GameReportQueryConfig* getGameReportQueryConfig(const char8_t* name) const;
    const GameReportQueryConfigMap *getGameReportQueryConfigMap() const
    {
        return &mGameReportQueryConfigMap;
    }
private:
    GameReportQueryConfigMap mGameReportQueryConfigMap;
};

class GameReportViewConfig
{
public:
    GameReportViewConfig(const GameReportView& view, const GameType* gameType);
    ~GameReportViewConfig();

    const GameReportView* getGameReportView() const { return &mView; }
    uint32_t getMaxVar() const { return mMaxVar; }
    uint32_t getMaxIndex() const { return mMaxIndex; }
    const char8_t* getQueryStringPrependIndex0Filter() const { return mQueryStringPrependIndex0Filter.c_str(); }
    const char8_t* getQueryStringPrependIndex1Filter() const { return mQueryStringPrependIndex1Filter.c_str(); }
    const char8_t* getQueryStringAppendFilter() const { return mQueryStringAppendFilter.c_str(); }

private:
    GameReportView mView;
    uint32_t mMaxVar;
    uint32_t mMaxIndex;
    eastl::string mQueryStringPrependIndex0Filter;
    eastl::string mQueryStringPrependIndex1Filter;
    eastl::string mQueryStringAppendFilter;
};


class GameReportViewsCollection
{
public:
    bool init(const GameReportingConfig& config, const GameTypeMap& gameTypeMap);
    bool parseGameReportColumns(const GameHistoryReportingViewColumnList& gameHistoryReportingViewColumnList, GameReportView::GameReportColumnList &columnList, const GameType* gameType) const;

    void clearGameReportViewConfigMap();

    ~GameReportViewsCollection()
    {
        clearGameReportViewConfigMap();
    }
    const GameReportViewConfig* getGameReportViewConfig(const char8_t* name) const;
    const GameReportViewConfigMap *getGameReportViewConfigMap() const { return &mGameReportViewConfigMap; }
    const GameReportViewInfosList::GameReportViewInfoList *getGameReportViewInfoList() const { return &mGameReportViewInfoList; }

private:
    GameReportViewConfigMap mGameReportViewConfigMap;
    GameReportViewInfosList::GameReportViewInfoList mGameReportViewInfoList;
};


class GameHistoryConfigParser
{
public:
    GameHistoryConfigParser()
    {
    }
    ~GameHistoryConfigParser()
    {
    }

    //  parses queries, views and from the game types, a flat map of table names to columns.
    bool init(const GameReportingConfig& config, const GameTypeCollection& gameTypes);
    void clear();

    //  after reconfiguration, the config references need to be updated.
    void updateConfigReferences(const GameReportingConfig& config);

    const GameReportQueriesCollection& getGameReportQueriesCollection() const
    {
        return mGameReportQueriesCollection;
    }
    const GameReportViewsCollection& getGameReportViewsCollection() const
    {
        return mGameReportViewsCollection;
    }
    const GameHistoryConfig& getConfig() const
    {
        return *mConfig;
    }

private:
    //  helpers
    bool onConfigureGameHistory(const GameReportingConfig& config, const GameTypeCollection& gameTypes);

private:
    GameReportQueriesCollection mGameReportQueriesCollection;
    GameReportViewsCollection mGameReportViewsCollection;
    
    const GameHistoryConfig* mConfig;
};

bool findGameHistoryTable(const GameTypeReportConfig& report, const char8_t* tableName, const GameTypeReportConfig::GameHistory* &gameHistory, const char8_t* columnName = nullptr);

void validateAttributeConfig(const char8_t* attributeType, const char8_t* name, const char8_t* gameReportName, const GameTypeConfig& gameType, ConfigureValidationErrors& validationErrors);
bool validateAttribute(const char8_t* attributeType, const char8_t* name, const GameType* gameType);

bool validateExpression(const char8_t* expression);

void validateGameHistoryReportingColumn(const char8_t* colName, const char8_t* colTable, uint16_t colIndex, const char8_t* gameReportName, const GameTypeConfig& gameType, ConfigureValidationErrors& validationErrors);
bool validateGameReportColumnKey(const GameReportColumnKey& key, const GameType* gameType);

void validateGameHistoryReporingColumnUserCoreIdentName(const char8_t* entityType, const char8_t* userCoreIdentName, ConfigureValidationErrors& validationErrors);

void validateGameHistoryReportingFilter(const GameHistoryReportingFilter& filter, const char8_t* gameReportName, const GameTypeConfig& gameType, bool onlyIndexZero, ConfigureValidationErrors& validationErrors);
bool validateGameReportFilter(GameReportFilter& filter, const GameType* gameType, bool onlyIndexZero);

bool parseGameReportFilters(const GameHistoryReportingFilterList& gameHistoryReportingFilterList, 
    GameReportFilterList& filterList, const GameType *gameType, bool onlyIndexZero);

bool validateQueryVarValue(const char8_t *queryVarValue);

const eastl::string expandBlazeObjectId(const EA::TDF::ObjectId& userSetId);

} // GameReporting
} // Blaze
#endif // BLAZE_GAMEHISTORY_CONFIG_H
