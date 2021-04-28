/*************************************************************************************************/
/*!
    \file   reportparserutil.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_REPORTPARSERUTIL_H
#define BLAZE_GAMEREPORTING_REPORTPARSERUTIL_H

#include "gametype.h"
#include "gamereporttdf.h"

#ifdef TARGET_achievements
#include "achievements/tdf/achievements.h"
#endif
#include "stats/tdf/stats_server.h"
#include "stats/tdf/stats.h"
#include "stats/updatestatsrequestbuilder.h"
#include "processedgamereport.h"
#include "util/eventparserutil.h"
#include "util/psnmatchutil.h"
#include "util/statsserviceutil.h"
#include "EASTL/map.h"
#include "EATDF/tdfbasetypes.h"


namespace Blaze
{
namespace GameReporting
{

class GameHistoryReport;
class GameReportExpression;

namespace Utilities
{

/**
    class ReportParser

    Use to parse out information from a collated game report during the report's call to GameReportingSlave::process.
    Often users will call parse multiple times to extract information in stages.  For example, extracting keyscopes
    prior to any custom stat sets.  Then after a custom implementation writes out its own stats, invoke
    the parse method again to generate stat queries encoded in the report.

    Custom code should check for blaze errors following a parse and handle them accordingly.  In most cases,
    ReportParserUtil::getBlazeError() != Blaze::ERR_OK should abort processing.

    Reports must be compliant with the report TDF specification as documented.

 */
class ReportParser : public ReportTdfVisitor
{
    NON_COPYABLE(ReportParser);

public:
    ReportParser(const GameType& gameType, ProcessedGameReport& report, const GameManager::PlayerIdList* playerIds = nullptr, const EventTypes* eventTypes = nullptr);
    ~ReportParser() override;

    enum ReportParseMode
    {
        REPORT_PARSE_NULL = 0x00000000,             // nop
        REPORT_PARSE_VALUES = 0x00000001,           // fills in report with values via configuration
        REPORT_PARSE_KEYSCOPES = 0x00000002,        // generates keyscope name/index pairs for the stats utility keyed from entity/statview
        REPORT_PARSE_STATS = 0x00000004,            // parses stat updates (and generates keyscope info as well.)
        REPORT_PARSE_ACHIEVEMENTS = 0x00000008,     // parses achievement updates
        REPORT_PARSE_GAME_HISTORY = 0x00000010,     // parse game history attributes from TDF.
        REPORT_PARSE_PLAYERS = 0x00000020,          // parses player IDs ONLY (all other parse modes also do this implicitly.)
        REPORT_PARSE_METRICS = 0x00000040,          // parses metrics values in the report
        REPORT_PARSE_EVENTS = 0x00000100,           // parses events in the report
        REPORT_PARSE_STATS_SERVICE = 0x00000200,    // parses stat updates for the Stats Service
        REPORT_PARSE_PSN_MATCH = 0x00000400,        // parses PSN Match report update

        // all possible parse modes (flags) -- this value must be updated if you add new parse modes (flags)
        REPORT_PARSE_ALL = REPORT_PARSE_VALUES
            | REPORT_PARSE_KEYSCOPES
            | REPORT_PARSE_STATS
            | REPORT_PARSE_ACHIEVEMENTS
            | REPORT_PARSE_GAME_HISTORY
            | REPORT_PARSE_PLAYERS
            | REPORT_PARSE_METRICS
            | REPORT_PARSE_EVENTS
            | REPORT_PARSE_STATS_SERVICE
            | REPORT_PARSE_PSN_MATCH
    };

    //  required for stat updates.  invokes the updatestats util's stat commands while traversing the TDF.
    void setUpdateStatsRequestBuilder(Stats::UpdateStatsRequestBuilder* builder)
    {
        mUpdateStatsRequestBuilder = builder;
    }

    // required for Stat Service updates
    void setStatsServiceUtil(StatsServiceUtilPtr util)
    {
        mStatsServiceUtil = util;
    }

    // required for PSN Match updates
    void setPsnMatchUtil(PsnMatchUtilPtr util)
    {
        mPsnMatchUtil = util;
    }

    //  required for parsing game history.
    void setGameHistoryReport(GameHistoryReport* report)
    {
        mGameHistoryReport = report;
    }

    const CustomEvents& getEvents()
    {
        return mEvents;
    }

#ifdef TARGET_achievements
    typedef eastl::vector<Achievements::GrantAchievementRequestPtr> GrantAchievementRequestList;
    typedef eastl::vector<Achievements::PostEventsRequestPtr> PostEventsRequestList;

    const GrantAchievementRequestList& getGrantAchievementRequests()
    {
        return mGrantAchievementRequests;
    }
    const PostEventsRequestList& getPostEventsRequests()
    {
        return mPostEventsRequests;
    }
#endif

    //  parses a EA::TDF::Tdf and extracts all information from a compliant EA::TDF::Tdf needed to inspect and report data.
    //  if the EA::TDF::Tdf didn't parse, returns false.  also to check if a blaze error occurred during parse, call getErrorCode()
    //  pass in flags from ReportParseMode to parse individual sections.  for example if you need just keyscopes to update your own stats,
    //      then followup with another request to parse the report TDF stats.
    //  The EA::TDF::Tdf must match the configuration passed.  if no configuration is passed to this method, assumes the top-level game report EA::TDF::Tdf.
    //      this method will validate the config with the tdf to at least match the class.
    bool parse(EA::TDF::Tdf& tdf, uint32_t parseFlags, const GameTypeReportConfig* config=nullptr);

    //  walk TDF and extract player/game/general report type attributes into a GameHistoryReport for submission to GameHistory post-process.
    bool parseGameHistoryAttributes(GameHistoryReport& gameHistoryReport, const EA::TDF::Tdf& tdf);

    //  clears all data parsed from the EA::TDF::Tdf.
    void clear();

    //  THESE METHODS ARE VALID ONLY AFTER CALLING parse() with the appropriate parse mode(s)
    //  generates keyscopes for the UpdateStats utility, returns the number of keyscopes
    //  statUpdate should point to a stat update from the configuration obtained from the GameType (see GameType::getStatUpdate)
    //  index is optional - relevent if the report's StatUpdate configuration specified a keyscopeIndex option.   The keyscopeIndex option allows a second
    //      dimension to keyscopes, keyed off of entityId / statUpdate.  This allows multiple scope name/value pairs per entityId/StatUpdate.
    size_t generateScopeIndexMap(Stats::ScopeNameValueMap& scopemap, EntityId entityId, const GameType::StatUpdate& statUpdate, uint64_t index=0);

    //  return a list of player blaze IDs generated during a parse.
    typedef eastl::vector_set<BlazeId> PlayerIdSet;
    const PlayerIdSet& getPlayerIdSet() const
    {
        return mPlayerIdSet;
    }

    void ignoreStatUpdatesForObjectType(const EA::TDF::ObjectType& objectType, bool ignore);
    void ignoreGameHistoryForObjectType(const EA::TDF::ObjectType& objectType, bool ignore);

    void ignoreStatUpdatesForEntityId(const EA::TDF::ObjectType& objectType, const EntityId& entityId, bool ignore);
    void ignoreGameHistoryForEntityId(const EA::TDF::ObjectType& objectType, const EntityId& entityId, bool ignore);

    //  Returns any blaze error generated by the parse.  Custom code can handle this error in any manner.  The stock pipeline will fail report
    //  processing if this is any value other than ERR_OK.
    BlazeRpcError getErrorCode() const
    {
        return mBlazeError;
    }

    static bool checkCondition(GameReportExpression& condexpr, const GameReportContext& context);

protected:
    //  visit the top-level subreport config
    bool visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports) override;
    //  visit the list of game history section of the config subreport
    bool visitGameHistory(const GameTypeReportConfig::GameHistoryList& gameHistoryList, const GRECacheReport::GameHistoryList& greGameHistoryList, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of stat updates for the subreport
    bool visitStatUpdates(const GameTypeReportConfig::StatUpdatesMap& statUpdates, const GRECacheReport::StatUpdates& greStatUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of Stats Service updates for the subreport
    bool visitStatsServiceUpdates(const GameTypeReportConfig::StatsServiceUpdatesMap& statsServiceUpdates, const GRECacheReport::StatsServiceUpdates& greStatsServiceUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of report values for the subreport - note this is done through EA::TDF::TdfVisitor calls for TDF primitive attributes instead.
    bool visitReportValues(const GameTypeReportConfig::ReportValueByAttributeMap& reportValues, const GRECacheReport::ReportValues& greReportValues, const GameReportContext& context, EA::TDF::Tdf& tdfOut) const override { return true; }
    //  visit the map of metric updates for this subreport.
    bool visitMetricUpdates(const GameTypeReportConfig::ReportValueByAttributeMap& metricUpdates, const GRECacheReport::MetricUpdates& greMetricUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of events for the subreport
    bool visitEventUpdates(const GameTypeReportConfig::EventUpdatesMap& events, const GRECacheReport::EventUpdates& greEvents, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of achievements for the subreport
    bool visitAchievementUpdates(const GameTypeReportConfig::AchievementUpdates& achievements, const GRECacheReport::AchievementUpdates& greAchievements, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the PSN Match report update section of the subreport
    bool visitPsnMatchUpdates(const GameTypeReportConfig::PsnMatchUpdatesMap& psnMatchUpdates, const GRECacheReport::PsnMatchUpdates& grePsnMatchUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;

private:
    const GameType& mGameType;
    const GameInfo* mGameInfo;
    const CollatedGameReport::DnfStatusMap& mDnfStatus;
    GameReportingMetricsCollection& mMetricsCollection;
    Stats::StatCategoryList::StatCategorySummaryList mStatCategorySummaryList;
    const EventTypes* mEventTypes;

    GameHistoryReport *mGameHistoryReport;                  // passed in through the parseGameHistoryAttributes call

    uint32_t mReportParseFlags;                             // indicates what's already been parsed via parse()
    ReportParseMode mReportParseMode;                       // indicates current parsing mode for the EA::TDF::TdfVisitor.
    mutable BlazeRpcError mBlazeError;                      // error returned from parse.

    //  maintain a current list of ReportClasses in the parsed report TDF.  These are extracted from the ReportClass attribute
    typedef eastl::vector<ReportClassName> ReportClassNameList;
    ReportClassNameList mReportClassNames;

    //  ScopeNameValueMap relative to the statupdate+entity. (TODO - consider moving to a hash map.)

    typedef eastl::map<uint64_t, Stats::ScopeNameValueMap> ScopeNameValueByIndexMap;
    typedef eastl::map<GameType::EntityStatUpdate, ScopeNameValueByIndexMap > KeyscopesMap;
    KeyscopesMap mKeyscopesMap;

    PlayerIdSet mPlayerIdSet;

    ///////////////////////////////////////////////// Events ///////////////////////////////////////////////
    CustomEvents mEvents;
    Utilities::EventTdfParser mEventTdfParser;

    /////////////////////////////////////////////////  Stats ///////////////////////////////////////////////
    Stats::UpdateStatsRequestBuilder* mUpdateStatsRequestBuilder;

    void makeStat(const char8_t *name, const char8_t *val, int32_t updateType);

    template <typename T> void setIntReportValueViaConfig(T& value, const VisitorState& state, const EA::TDF::Tdf& parentTdf, uint32_t tag);

    bool getValueFromTdfViaConfig(EA::TDF::TdfGenericValue& value, const GameTypeReportConfig& config, const GameReportContext& context, const EA::TDF::Tdf& parentTdf, uint32_t tag) const;

    void setBlazeErrorFromExpression(const GameReportExpression& expr) const;

    typedef eastl::hash_map<const char8_t*, EA::TDF::ObjectType, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > CategoryNameTypeMap;
    CategoryNameTypeMap mCategoryNameTypeMap;

    ///////////////////////////////////////////// Stats Service ///////////////////////////////////////////
    StatsServiceUtilPtr mStatsServiceUtil;

    PsnMatchUtilPtr mPsnMatchUtil;

    //////////////////////////////////////////////// Game History ///////////////////////////////////////////////
    typedef eastl::map<EA::TDF::ObjectType, bool> BlazeObjectTypeMap;
    typedef eastl::map<EA::TDF::ObjectId, bool> BlazeObjectIdMap;
    typedef eastl::map<uint32_t, GameHistoryReport::TableRowMap > TableRowMapByLevel;

    BlazeObjectTypeMap mOverrideStatUpdateByObjectType;
    BlazeObjectIdMap mOverrideStatUpdateByObjectId;
    BlazeObjectTypeMap mOverrideGameHistoryByObjectType;
    BlazeObjectIdMap mOverrideGameHistoryByObjectId;
    TableRowMapByLevel mTraversedHistoryData;

#ifdef TARGET_achievements
    //////////////////////////////////////////////// Achievements ///////////////////////////////////////////////
    GrantAchievementRequestList mGrantAchievementRequests;
    PostEventsRequestList mPostEventsRequests;
#endif

    void genericToString(const EA::TDF::TdfGenericValue& result, char8_t* valueBuf, size_t bufSize);
    double genericToDouble(const EA::TDF::TdfGenericValue& result);

    /////////////////////////////////////////////////  TDF Parser ///////////////////////////////////////////////
    //  attribute parsing helpers.

    //  Visit Primitive Members - set report values based on configuration using visitation versus reflection
    //  (StatUpdates/GameHistory use the specific ReportTdfVisitor overrides.)
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue=0.0f) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override;

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue) override {}    // unsupported. /*lint !e1961 */
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue = 0) override {}    // unsupported. /*lint !e1961 */
};

} //namespace Utilities

} //namespace GameReporting
} //namespace Blaze

#endif
