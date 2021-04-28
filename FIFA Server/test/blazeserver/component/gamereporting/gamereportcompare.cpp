/*************************************************************************************************/
/*!
    \file   


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "gamereporting/gamereportcompare.h"
#include "gamereporting/gametype.h"

namespace Blaze
{
namespace GameReporting
{

//  used by classes inheriting from TdfCompare for custom EA::TDF::Tdf validation.
void GameReportCompare::setDifferenceFoundFlag(bool diff) 
{
    mDifferenceFound = diff;
}

bool GameReportCompare::getDifferenceFoundFlag() const 
{
    return mDifferenceFound;
}

void GameReportCompare::compareReports(const GameTypeReportConfig& gameTypeReportConfig, const GRECacheReport& greCache, EA::TDF::Tdf& report, const EA::TDF::Tdf& reportReference)
{
    //  visit with an empty context
    GameReportContext& context = ReportTdfVisitor::createReportContext(nullptr);
    context.tdf = &reportReference;
    ReportTdfVisitor::visit(gameTypeReportConfig, greCache, report, context, report);
}

bool GameReportCompare::compare(const GameType& gameType, EA::TDF::Tdf& report, const EA::TDF::Tdf& reportReference)
{
    compareReports(gameType.getConfig(), gameType.getGRECache(), report, reportReference);
    ReportTdfVisitor::flushReportContexts();
    return !mDifferenceFound;
}

//  visit top-level game type of the config subreport
bool GameReportCompare::visitSubreport(const GameTypeReportConfig& subReportConfig, const GRECacheReport& greSubReport, const GameReportContext& context, EA::TDF::Tdf& tdfOut, bool& traverseChildReports)
{   
    if (mDifferenceFound && !subReportConfig.getSkipComparison())
    {
        traverseChildReports = false;
        WARN_LOG("[GameReportCompare].visitSubreport(): differences found, compared tdf '" << tdfOut.getFullClassName() << "' within '" << context.tdf->getFullClassName() << "'");
        return false;       // stop tree traversal of the report
    }
    if (subReportConfig.getSkipComparison())
    {
        //  if differences found at this node but configured to ignore differences at this node, clear out difference found flag (since comparison is done before reaching this point.)
        mDifferenceFound = false;
        traverseChildReports = false;
        TRACE_LOG("[GameReportCompare].visitSubreport(): skipping comparision of node, tdf '" << tdfOut.getFullClassName() << "' within '" << context.tdf->getFullClassName() << "'");
        return true;        // continue parent node traversal (skipping children re: traverseChildReports)
    }
    TRACE_LOG("[GameReportCompare].visitSubreport(): compared tdf '" << tdfOut.getFullClassName() << "' within '" << context.tdf->getFullClassName() << "'");
    return true;
}

//  visit the list of game history section of the config subreport
bool GameReportCompare::visitGameHistory(const GameTypeReportConfig::GameHistoryList& gameHistoryList, const GRECacheReport::GameHistoryList& greGameHistoryList, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    return true;
}
    
//  visit the map of stat updates for the subreport
bool GameReportCompare::visitStatUpdates(const GameTypeReportConfig::StatUpdatesMap& statUpdates, const GRECacheReport::StatUpdates& greStatUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    return true;
};

//  visit the map of Stats Service updates for the subreport
bool GameReportCompare::visitStatsServiceUpdates(const GameTypeReportConfig::StatsServiceUpdatesMap& statsServiceUpdates, const GRECacheReport::StatsServiceUpdates& greStatsServiceUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    return true;
};

//  visit the map of report values for the subreport
bool GameReportCompare::visitReportValues(const GameTypeReportConfig::ReportValueByAttributeMap& reportValues, const GRECacheReport::ReportValues& greReportValues, const GameReportContext& context, EA::TDF::Tdf& tdfOut) const
{
    return true;
};

//  visit the map of metric updates for this subreport.
bool GameReportCompare::visitMetricUpdates(const GameTypeReportConfig::ReportValueByAttributeMap& metricUpdates, const GRECacheReport::MetricUpdates& greMetricUpdates, const GameReportContext& context, EA::TDF::Tdf& tdfOut)
{
    return true;
};

bool GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::Tdf &value, const EA::TDF::Tdf &referenceValue)
{   
    if (mStateStack.top().config->getSkipComparison())
    {
        TRACE_LOG("[GameReportCompare].visit(): skipping compare of '" << value.getFullClassName() << "' within '" << rootTdf.getFullClassName() << "'");
        return false;
    }

    TRACE_LOG("[GameReportCompare].visit(): comparing of '" << value.getFullClassName() << "' within '" << rootTdf.getFullClassName() << "'");

    //  thunk down to the parent's visit method which will traverse any subreports within this tdf - this is necessary
    //  to ensure that our current config on the stack matches the child subreport.
    return ReportTdfVisitor::visit(rootTdf, parentTdf, tag, value, referenceValue);
}

void GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfVectorBase &value, const EA::TDF::TdfVectorBase &referenceValue)
{
    //  only traverse the complex type if deep comparison is turned on or if not, then only if there are no differences so far.
    if (!mDifferenceFound)
    {
        if (mStateStack.top().config->getSkipComparison())
        {    
            return;
        }

        if (value.vectorSize() != referenceValue.vectorSize())
        {
            mDifferenceFound = true;
        }
        else if (value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_TDF && value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE)
        {
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
        }
    }
};

void GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfMapBase &value, const EA::TDF::TdfMapBase &referenceValue)
{
    //  only traverse the complex type if deep comparison is turned on or if not, then only if there are no differences so far.
    if (!mDifferenceFound)
    {
        if (mStateStack.top().config->getSkipComparison())
        {    
            return;
        }

        if (value.mapSize() != referenceValue.mapSize())
        {
            mDifferenceFound = true;
        }
        else if (value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_TDF && value.getValueType() != EA::TDF::TDF_ACTUAL_TYPE_VARIABLE)
        {
            value.visitMembers(*this, rootTdf, parentTdf, tag, referenceValue);
        }
    }
}

bool GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::GenericTdfType &value, const EA::TDF::GenericTdfType &referenceValue)
{
    //  only traverse the complex type if deep comparison is turned on or if not, then only if there are no differences so far.
    if (!mDifferenceFound)
    {
        if (mStateStack.top().config->getSkipComparison())
        {    
            return false;
        }
        
        visitReference(rootTdf, parentTdf, tag, value.getValue(), &referenceValue.getValue());
    }
    return true;
}

void GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfString &value, const EA::TDF::TdfString &referenceValue, const char8_t *defaultValue, const uint32_t maxLength)
{
    if (blaze_strcmp(referenceValue.c_str(), value.c_str())!=0)
    {
        mDifferenceFound = true;
    }
}

bool GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfUnion &value, const EA::TDF::TdfUnion &referenceValue)
{ 
    //  only traverse the complex type if deep comparison is turned on or if not, then only if there are no differences so far.
    if (!mDifferenceFound)
    {
        if (mStateStack.top().config->getSkipComparison())
        {    
           return false;
        }
        value.visit(*this, rootTdf, referenceValue); 
    }
    return true;
}

void GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBlob &value, const EA::TDF::TdfBlob &referenceValue)
{
    if (mStateStack.top().config->getSkipComparison())
    {
       return;
    }

    if (value.getCount() != referenceValue.getCount() || value.getSize() != referenceValue.getSize())
    {
        mDifferenceFound=true;
    }
    //  NOTE: should deep compare actually dig into blob data?
}

bool GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::VariableTdfBase &value, const EA::TDF::VariableTdfBase &referenceValue)
{ 
    //  variable tdfs are typically subreports - subreports are processed within the ReportTdfVisitor::visit() call.
    return true;
}

void GameReportCompare::visit(EA::TDF::Tdf &rootTdf, EA::TDF::Tdf &parentTdf, uint32_t tag, EA::TDF::TdfBitfield &value, const EA::TDF::TdfBitfield &referenceValue)
{
    if (mStateStack.top().config->getSkipComparison())
    {    
       return;
    }
    if (value.getBits()!=referenceValue.getBits())
    {
        mDifferenceFound=true;
    }
}

} //namespace Blaze
} //namespace GameReporting
