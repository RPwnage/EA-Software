/**************************************************************************************************/
/*!
    \file samplereportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"

#include "customcode/component/gamereporting/sample/tdf/samplecustomreport.h"
#include "customcode/component/gamereporting/integratedsample/tdf/integratedsample.h"

#include "samplereportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{

namespace Stress
{
    // FIFA SPECIFIC CODE START
    //CustomGameReporterRegistration SampleGameReporter::mOffline("sampleReportBase", CustomGameReporterCreateFn(&SampleGameReporter::create), CustomGameReporterPickFn(&SampleGameReporter::pickGameType));
    //CustomGameReporterRegistration SampleGameReporter::mOnline("integratedSample", CustomGameReporterCreateFn(&SampleGameReporter::create), CustomGameReporterPickFn(&SampleGameReporter::pickGameType));
    // FIFA SPECIFIC CODE END

CustomGameReporter* SampleGameReporter::create(GameReporting::GameReport& gameReport)
{
    return BLAZE_NEW SampleGameReporter(gameReport);
}


bool SampleGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
{
    if (gameAttrs == nullptr)
    {
        gameReportName.set("sampleReportBase");    
    }
    else
    {
        gameReportName.set("integratedSample");
    }
    return true;
}


SampleGameReporter::SampleGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
{
}


SampleGameReporter::~SampleGameReporter()
{
}


void SampleGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
{
    const char8_t* gameReportName = mGameReport->getGameReportName();
    if (blaze_stricmp(gameReportName, "sampleReportBase")==0)
    {
        SampleBase::Report *report = static_cast<SampleBase::Report*>(mGameReport->getReport());
        report->getGameAttrs().setMapId((rand() % 3)+1);
        report->getGameAttrs().setMode((rand() % 2)+1);
        if (report->getPlayerReports().empty())
        {
            report->getPlayerReports().reserve(players->size());
            SampleBase::Report::PlayerReportsMap& playerReportMap = report->getPlayerReports();
            for (GameReportingUtil::PlayerAttributeMap::const_iterator citPlayerIt = players->begin(), citPlayerItEnd = players->end();
                    citPlayerIt != citPlayerItEnd;
                    ++citPlayerIt)
            {
                SampleBase::PlayerReport *playerReport = playerReportMap.allocate_element();
                playerReport->setDeaths((uint16_t)(rand() % 20));
                playerReport->setKills((uint16_t)(rand() % 20));
                playerReport->setLongestTimeAlive((uint32_t)(rand() % 3600000));
                playerReport->setMoney((uint32_t)(rand() % 1000000));
                playerReportMap[citPlayerIt->first] = playerReport;          
            }
        }
    } 
    else if (blaze_stricmp(gameReportName, "integratedSample")==0)
    {
        IntegratedSample::Report *report = static_cast<IntegratedSample::Report*>(mGameReport->getReport());
        int32_t val;
        if (gameAttrs != nullptr)
            blaze_str2int(gameAttrs->find("ISmap")->second.c_str(), &val);
        else
            val = (rand() % 3) + 1;
        report->getGameAttrs().setMapId(val);

        if (gameAttrs != nullptr)
            blaze_str2int(gameAttrs->find("ISmode")->second.c_str(), &val);
        else
            val = (rand() % 2) + 1;
        report->getGameAttrs().setMode(val);

        if (report->getPlayerReports().empty())
        {
            report->getPlayerReports().reserve(players->size());
            IntegratedSample::Report::PlayerReportsMap& playerReportMap = report->getPlayerReports();
            for (GameReportingUtil::PlayerAttributeMap::const_iterator citPlayerIt = players->begin(), citPlayerItEnd = players->end();
                    citPlayerIt != citPlayerItEnd;
                    ++citPlayerIt)
            {
                IntegratedSample::PlayerReport *playerReport = playerReportMap.allocate_element();
                playerReport->setHits((uint16_t)(rand() % 20));
                playerReport->setMisses((uint16_t)(rand() % 20));
                playerReport->setTorchings((uint16_t)(rand() % 10));
                playerReportMap[citPlayerIt->first] = playerReport;          
            }
        }
    }
}

} //Stress

} //Blaze

