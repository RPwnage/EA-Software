/*************************************************************************************************/
/*!
    \file   gamereportexpression.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_GAMEREPORTTDF_H
#define BLAZE_GAMEREPORTING_GAMEREPORTTDF_H

#include <EASTL/fixed_vector.h>
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereportexpression.h"
#include "PPMalloc/EAFixedAllocator.h"

namespace Blaze
{
namespace GameReporting
{
class GameType;

struct GameReportContext: public eastl::intrusive_list_node
{
    GameReportContext(): parent(nullptr), gameInfo(nullptr), dnfStatus(nullptr), tdf(nullptr) {
        containerIteration = 0;
        level = 0;
        mpNext = mpPrev = nullptr;
    }

    //  initializes a child context based on an existing context
    void setParent(const GameReportContext& owner);

    //  if this context derives from another context.
    const GameReportContext *parent;

    //  game manager game information (for game and player attributes)
    const GameInfo* gameInfo;

    //  DNF information for game.
    const CollatedGameReport::DnfStatusMap* dnfStatus;

    //  current EA::TDF::Tdf for source data.
    const EA::TDF::Tdf *tdf;

    //  the container where the tdf resides
    struct Container
    {
        bool isValid() const { return value.isValid() && (value.getType() == EA::TDF::TDF_ACTUAL_TYPE_LIST || value.getType() == EA::TDF::TDF_ACTUAL_TYPE_MAP); }
        EA::TDF::TdfGenericReferenceConst value;
        union
        {
            uint64_t id;                 // For TDF_TYPE_MAP if indexType == TDF_TYPE_INTEGER
            const char8_t* str;
        }
        Index;
    }
    container;

    uint32_t containerIteration;

    uint32_t level;
};

/**
    GameReportExpression map classes
    These classes are meant to map 1-to-1 to the GameTypeReportConfig configuration class
    They are used by the game reporting framework to precache parsed expressions
 */
struct GRECacheReport
{
    //  what report config inside the owning GameType does this node map to.  memory points to the config node inside GameType.
    const GameTypeReportConfig* mReportConfig;

    struct Event
    {
        GameReportExpression *mEntityId;
        GameReportExpression *mTdfMemberName;

        typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > EventData;
        EventData mEventData;

        Event(const GameType& gameType, const GameTypeReportConfig::Event& eventConfig);
        ~Event();
    };

    struct StatUpdate
    {
        GameReportExpression *mCategory;
        GameReportExpression *mEntityId;
        GameReportExpression *mKeyscopeIndex;
        GameReportExpression *mCondition;

        typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > Keyscopes;
        Keyscopes mKeyscopes;

        struct Stat
        {
            GameReportExpression *mValue;
            GameReportExpression *mCondition;

            Stat(const GameType& gameType, const GameTypeReportConfig::StatUpdate::Stat& stat);
            ~Stat();
        };

        typedef eastl::vector_map<ReportAttributeName, Stat*, EA::TDF::TdfStringCompareIgnoreCase > Stats;
        Stats mStats;

        StatUpdate(const GameType& gameType, const GameTypeReportConfig::StatUpdate& statUpdateConfig);
        ~StatUpdate();
    };

    struct StatsServiceUpdate
    {
        GameReportExpression *mEntityId;
        GameReportExpression *mCondition;

        struct DimensionalStat
        {
            typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > Dimensions;

            GameReportExpression *mValue;
            GameReportExpression *mCondition;
            Dimensions mDimensions;

            DimensionalStat(const GameType& gameType, const GameTypeReportConfig::StatsServiceUpdate::DimensionalStat& stat);
            ~DimensionalStat();
        };

        typedef eastl::list<DimensionalStat*> DimensionalStatList;
        typedef eastl::vector_map<ReportAttributeName, DimensionalStatList*, EA::TDF::TdfStringCompareIgnoreCase > DimensionalStats;
        DimensionalStats mDimensionalStats;

        struct Stat
        {
            GameReportExpression *mValue;
            GameReportExpression *mCondition;

            Stat(const GameType& gameType, const GameTypeReportConfig::StatsServiceUpdate::Stat& stat);
            ~Stat();
        };
        typedef eastl::vector_map<ReportAttributeName, Stat*, EA::TDF::TdfStringCompareIgnoreCase > Stats;
        Stats mStats;

        StatsServiceUpdate(const GameType& gameType, const GameTypeReportConfig::StatsServiceUpdate& statUpdateConfig);
        ~StatsServiceUpdate();
    };

    struct GameHistory
    {
        typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > Columns;
        Columns mColumns;
        GameHistory(const GameType& gameType, const GameTypeReportConfig::GameHistory& gameHistoryConfig);
        ~GameHistory();
    };

    struct AchievementUpdates
    {
        GameReportExpression *mEntityId;

        struct Achievement
        {
            GameReportExpression *mPoints;
            GameReportExpression *mCondition;

            Achievement(const GameType& gameType, const GameTypeReportConfig::AchievementUpdates::Achievement& achieve);
            ~Achievement();
        };

        struct Event
        {
            typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > EventData;
            EventData mData;

            Event(const GameType& gameType, const GameTypeReportConfig::AchievementUpdates::Event& event);
            ~Event();
        };

        typedef eastl::vector_map<ReportAttributeName, Achievement*, EA::TDF::TdfStringCompareIgnoreCase> Achievements;
        Achievements mAchievements;

        typedef eastl::list<Event*> Events;
        Events mEvents;

        AchievementUpdates(const GameType& gameType, const GameTypeReportConfig::AchievementUpdates& achieveUpdateConfig);
        ~AchievementUpdates();
    };

    struct PsnMatchUpdate
    {
        struct MatchResults
        {
            GameReportExpression *mCooperativeResult;

            struct CompetitiveResult
            {
                GameReportExpression *mEntityId;
                GameReportExpression *mScore;

                CompetitiveResult(const GameType& gameType, const GameTypeReportConfig::PsnMatchUpdate::MatchResults::CompetitiveResult& compResult);
                ~CompetitiveResult();
            };
            CompetitiveResult *mCompetitiveResult;

            MatchResults(const GameType& gameType, const GameTypeReportConfig::PsnMatchUpdate::MatchResults& matchResults);
            ~MatchResults();
        };
        MatchResults *mMatchResults;

        typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > MatchStats;
        MatchStats mMatchStats;

        PsnMatchUpdate(const GameType& gameType, const GameTypeReportConfig::PsnMatchUpdate& psnMatchUpdateConfig);
        ~PsnMatchUpdate();
    };

    GameReportExpression *mPlayerId;

    typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > ReportValues;
    ReportValues mReportValues;

    typedef eastl::vector_map<ReportAttributeName, StatUpdate*, EA::TDF::TdfStringCompareIgnoreCase > StatUpdates;
    StatUpdates mStatUpdates;

    typedef eastl::vector_map<ReportAttributeName, StatsServiceUpdate*, EA::TDF::TdfStringCompareIgnoreCase > StatsServiceUpdates;
    StatsServiceUpdates mStatsServiceUpdates;

    typedef eastl::list<GameHistory* > GameHistoryList;
    GameHistoryList mGameHistory;

    typedef eastl::vector_map<ReportAttributeName, GameReportExpression*, EA::TDF::TdfStringCompareIgnoreCase > MetricUpdates;
    MetricUpdates mMetricUpdates;

    typedef eastl::vector_map<ReportAttributeName, GRECacheReport*, EA::TDF::TdfStringCompareIgnoreCase > SubReports;
    SubReports mSubReports;

    typedef eastl::vector_map<ReportAttributeName, Event*, EA::TDF::TdfStringCompareIgnoreCase > EventUpdates;
    EventUpdates mEventUpdates;

    AchievementUpdates *mAchievementUpdates;

    typedef eastl::vector_map<ReportAttributeName, PsnMatchUpdate*, EA::TDF::TdfStringCompareIgnoreCase > PsnMatchUpdates;
    PsnMatchUpdates mPsnMatchUpdates;

    ////////////////////////////////////////////////////////////////////////////////////////
    GRECacheReport() : mReportConfig(nullptr), mPlayerId(nullptr), mAchievementUpdates(nullptr) {}
    ~GRECacheReport();

    // finds the GameTypeReportConfig that matches with the GRECacheReport and all children
    const GRECacheReport* find(const GameTypeReportConfig* target) const;
    bool build(const GameType& gameType, const GameTypeReportConfig *config);
    void free();
};

/**
    class ReportTdfVisitor

    Streamlines ReportTdf visitor interface, acting as a bridge between the report configuration and the report EA::TDF::Tdf class.
 */
class ReportTdfVisitor: public EA::TDF::TdfVisitor
{
public:
    //  directly visit the tdf matching the passed in config's report EA::TDF::Tdf.
    //  in most cases : tdfOut == *context.tdf,
    bool visit(const GameTypeReportConfig& reportConfig, const GRECacheReport& greReport, EA::TDF::Tdf& tdfOut, GameReportContext& context, EA::TDF::Tdf& tdfOutParent);

    //  visit top-level game type of the config subreport
    virtual bool visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports) = 0;
    //  visit the list of game history section of the config subreport
    virtual bool visitGameHistory(const GameTypeReportConfig::GameHistoryList& gameHistoryList, const GRECacheReport::GameHistoryList& greGameHistoryList, const GameReportContext& context, EA::TDF::Tdf& tdfOut) = 0;
    //  visit the map of stat updates for the subreport
    virtual bool visitStatUpdates(const GameTypeReportConfig::StatUpdatesMap& statUpdates, const GRECacheReport::StatUpdates& greStatUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) = 0;
    //  visit the map of Stats Service updates for the subreport
    virtual bool visitStatsServiceUpdates(const GameTypeReportConfig::StatsServiceUpdatesMap& statsServiceUpdates, const GRECacheReport::StatsServiceUpdates& greStatsServiceUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) = 0;
    //  visit the map of report values for the subreport
    virtual bool visitReportValues(const GameTypeReportConfig::ReportValueByAttributeMap& reportValues, const GRECacheReport::ReportValues& greReportValues, const GameReportContext& context, EA::TDF::Tdf& tdfOut) const = 0;
    //  visit the map of metric updates for this subreport.
    virtual bool visitMetricUpdates(const GameTypeReportConfig::ReportValueByAttributeMap& metricUpdates, const GRECacheReport::MetricUpdates& greMetricUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) = 0;
    //  visit the map of events for the subreport
    virtual bool visitEventUpdates(const GameTypeReportConfig::EventUpdatesMap& events, const GRECacheReport::EventUpdates& greEvents, const GameReportContext& context, EA::TDF::Tdf& tdfOut) = 0;
    //  visit the achievement for the subreport
    virtual bool visitAchievementUpdates(const GameTypeReportConfig::AchievementUpdates& achievements, const GRECacheReport::AchievementUpdates& greAchievements, const GameReportContext& context, EA::TDF::Tdf& tdfOut) = 0;
    //  visit the PSN Match report update section of the subreport
    virtual bool visitPsnMatchUpdates(const GameTypeReportConfig::PsnMatchUpdatesMap& psnMatchUpdates, const GRECacheReport::PsnMatchUpdates& grePsnMatchUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) = 0;

protected:
    ReportTdfVisitor();
    ~ReportTdfVisitor() override;

    void setVisitSuccess(bool success) {
        mVisitSuccess = success;
    }

    GameReportContext& createReportContext(const GameReportContext* parent);
    void flushReportContexts();

    //  EA::TDF::TdfVisitor methods
    struct VisitorState
    {
        enum ContainerType { NO_CONTAINER, LIST_CONTAINER, MAP_CONTAINER };
        VisitorState() : context(nullptr), config(nullptr), containerType(NO_CONTAINER), isMapKey(false) {}
        VisitorState(const GameTypeReportConfig* cfg, GameReportContext* ctx, ContainerType container): context(ctx), config(cfg), containerType(container) {
            isMapKey = containerType==MAP_CONTAINER;
        }
        VisitorState(const VisitorState& src): context(src.context), config(src.config), containerType(src.containerType), isMapKey(src.isMapKey) {
            if (src.containerType == LIST_CONTAINER) {
                container.vecBase = src.container.vecBase;
            }
            else if (src.containerType == MAP_CONTAINER) {
                container.mapBase = src.container.mapBase;
            }
        }
        bool hasContainer() const { return containerType != NO_CONTAINER; }
        bool hasContextContainer(const GameReportContext& grContext) const;
        GameReportContext* context;
        const GameTypeReportConfig* config;
        const GRECacheReport *greCache;
        ContainerType containerType;
        union
        {
            EA::TDF::TdfMapBase *mapBase;
            EA::TDF::TdfVectorBase *vecBase;
        }
        container;
        bool isMapKey;
    };

    //  manage a stack of visitor states while traversing the TDF
    //  using a fixed_vector to prevent resizing of the vector and enforcing a string limit.
    //  the node count is set relatively higher than the expected report depth.
    struct VisitorStateStack: private eastl::fixed_vector<VisitorState, 48, false>
    {
        VisitorState* push(bool copyTop=true);
        void pop() { pop_back(); }
        VisitorState& top() { return back(); }
        bool isEmpty() const { return empty(); }
        void reset() { clear(); }
        void printTop() const;
    };
    VisitorStateStack mStateStack;

protected:
    //  other possible overrides that aren't context specific or config specific.
    //  when overriding these methods, DO NOT visit the values as the ReportTdfVisitor should handle visiting into a value.
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override { return true; }
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue) override { return true; }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override {}

    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue) override { return true; }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false) override {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue) override { updateVisitorStateForInt((int8_t)value, (int8_t)referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0) override { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0) override { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0) override  { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0) override { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0) override  { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0) override { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0) override { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0) override { updateVisitorStateForInt(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue=0.0f) override { updateVisitorState(value, referenceValue); }
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0) override  { updateVisitorStateForString(value, referenceValue); }

    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue) override {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue) override  {}
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TimeValue &value, const EA::TDF::TimeValue &referenceValue, const EA::TDF::TimeValue defaultValue = 0) override  {}


protected:
    //  if visiting this TDF inside a container, then execute a new visit with config and context.
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue) override;

private:
    bool visitReportSection(const GameTypeReportConfig& reportConfig, const GRECacheReport& greReport, EA::TDF::Tdf& tdfOut, GameReportContext& context,  bool& traverseChildReports);

    //  updates common state within visit methods (primitives only!)
    //  Purpose is to update map keys if currently visiting a map
    void updateVisitorStateForInt(uint64_t value, const uint64_t referenceValue);
    void updateVisitorStateForString(EA::TDF::TdfString& value, const EA::TDF::TdfString& referenceValue);
    //  updates common state within visit methods (general primitives only for container management)
    template < typename T > void updateVisitorState(T& value, const T& referenceValue);

    //  using a fixed allocator instead of a vector since the number of report contexts increases relative to the complexity of the report
    //  and memory for each context must remain valid throughout the report visit call.  contexts should be flushed per visit.
    EA::Allocator::FixedAllocator<GameReportContext> mReportContexts;
    bool mVisitSuccess;

    static void* allocCore(size_t nSize, void *pContext);
    static void freeCore(void *pCore, void *pContext);
private:
    //  no-ops, functionality superceeded by other methods in this visitor, which is responsible for visiting the EA::TDF::Tdf based on configuration.
    bool visit(EA::TDF::Tdf &tdf, const EA::TDF::Tdf &referenceValue) override { return true; }
    bool visit(EA::TDF::TdfUnion &tdf, const EA::TDF::TdfUnion &referenceValue) override { return true; }
};

} //namespace Blaze
} //namespace GameReporting
#endif
