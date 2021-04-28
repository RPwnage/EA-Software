/*************************************************************************************************/
/*!
    \file   gametype.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_REPORT_COMPARE_H
#define BLAZE_GAMEREPORTING_REPORT_COMPARE_H

#include "gamereporting/gamereporttdf.h"
#include "gamereporting/gamereportingconfig.h"
#include "EATDF/tdf.h"

namespace Blaze
{
namespace GameReporting
{

////////////////////////////////////////////////////////////////////////////////////////////////////
//  Utility class to compare 2 reports based on the supplied config. 
//  return false if the reports differs or if any one of the reports not matching the config structure

class GameReportCompare: public ReportTdfVisitor
{
public:
    GameReportCompare() : mDifferenceFound(false) {}
    ~GameReportCompare() override {}

    //  returns false if the two GameReports differ.
    bool compare(const GameType& gameType, EA::TDF::Tdf& report, const EA::TDF::Tdf& reportReference);

protected:
    void compareReports(const GameTypeReportConfig& gameTypeReportConfig, const GRECacheReport& greCache, EA::TDF::Tdf& report, const EA::TDF::Tdf& reportReference);

    //  used by classes inheriting from TdfCompare for custom EA::TDF::Tdf validation.
    void setDifferenceFoundFlag(bool diff);
    bool getDifferenceFoundFlag() const; 

    //  visit top-level game type of the config subreport
    bool visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports) override;
    //  visit the list of game history section of the config subreport
    bool visitGameHistory(const GameTypeReportConfig::GameHistoryList& gameHistoryList, const GRECacheReport::GameHistoryList& greGameHistoryList, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of stat updates for the subreport
    bool visitStatUpdates(const GameTypeReportConfig::StatUpdatesMap& statUpdates, const GRECacheReport::StatUpdates& greStatUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of Stats Service updates for the subreport
    bool visitStatsServiceUpdates(const GameTypeReportConfig::StatsServiceUpdatesMap& statsServiceUpdates, const GRECacheReport::StatsServiceUpdates& greStatsServiceUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of report values for the subreport
    bool visitReportValues(const GameTypeReportConfig::ReportValueByAttributeMap& reportValues, const GRECacheReport::ReportValues& greReportValues, const GameReportContext& context, EA::TDF::Tdf& tdfOut) const override;
    //  visit the map of metric updates for this subreport.
    bool visitMetricUpdates(const GameTypeReportConfig::ReportValueByAttributeMap& metricUpdates, const GRECacheReport::MetricUpdates& greMetricUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override;
    //  visit the map of events for the subreport
    bool visitEventUpdates(const GameTypeReportConfig::EventUpdatesMap& events, const GRECacheReport::EventUpdates& greEvents, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }
    //  visit the map of achievements for the subreport
    bool visitAchievementUpdates(const GameTypeReportConfig::AchievementUpdates& achievements, const GRECacheReport::AchievementUpdates& greAchievements, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }
    //  visit the PSN Match report update section of the subreport
    bool visitPsnMatchUpdates(const GameTypeReportConfig::PsnMatchUpdatesMap& psnMatchUpdates, const GRECacheReport::PsnMatchUpdates& grePsnMatchUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut) override { return true; }

protected:
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue) override;
    void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue) override;
    bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue) override;

     bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue) override;
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, bool &value, const bool referenceValue, const bool defaultValue = false) override { 
         compareValue(value, referenceValue); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }

     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, char8_t &value, const char8_t referenceValue, const char8_t defaultValue) override {
         compareValue(value, referenceValue); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }

     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int8_t &value, const int8_t referenceValue, const int8_t defaultValue = 0) override { 
         compareValue(value, referenceValue); ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint8_t &value, const uint8_t referenceValue, const uint8_t defaultValue = 0) override { 
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int16_t &value, const int16_t referenceValue, const int16_t defaultValue = 0) override  {
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint16_t &value, const uint16_t referenceValue, const uint16_t defaultValue = 0) override {
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const int32_t defaultValue = 0) override {
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint32_t &value, const uint32_t referenceValue, const uint32_t defaultValue = 0) override {
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int64_t &value, const int64_t referenceValue, const int64_t defaultValue = 0) override  {
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, uint64_t &value, const uint64_t referenceValue, const uint64_t defaultValue = 0) override  {
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, float &value, const float referenceValue, const float defaultValue=0.0f) override {
         compareValue(value, referenceValue);  ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue, defaultValue);
     }

     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue = "", const uint32_t maxLength = 0) override; 
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue) override;
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue) override;
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, int32_t &value, const int32_t referenceValue, const EA::TDF::TdfEnumMap *enumMap, const int32_t defaultValue = 0 ) override  {compareValue(value, referenceValue);}
     void visit(EA::TDF::Tdf &rootTd, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectType &value, const EA::TDF::ObjectType &referenceValue, const EA::TDF::ObjectType defaultValue) override {compareValue(value, referenceValue);}
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::ObjectId &value, const EA::TDF::ObjectId &referenceValue, const EA::TDF::ObjectId defaultValue) override  {compareValue(value, referenceValue);}
     void visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, TimeValue &value, const TimeValue &referenceValue, const TimeValue defaultValue = 0) override  {compareValue(value, referenceValue);}

protected: 
   bool visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue) override;

private:
    bool mDifferenceFound;

    template <typename A, typename B> inline void compareValue(const A& a, const B& b) 
    {
        // if skipping comparison for this subreport, do not allow traversal into child subreports of this subreport node.
        if (mStateStack.top().config->getSkipComparison())
        {    
            return;
        }
        if (a != b)
        {
            mDifferenceFound = true;
        }
    }

};

}; //namespace GameReporting

}; //namespace Blaze

#endif
