/**************************************************************************************************/
/*!
    \file shooterreportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"

#include "framework/config/config_file.h"
#include "framework/config/configdecoder.h"

#include "shooterreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{

namespace Stress
{

    // Shooter game reports require game modes to be properly setup on the server and the shooterreport.cfg being setup before this can work properly.
    // Therefore, we do not want to enable shooter reports by default so the following lines are commented out to disable shooter reports by default.

    //CustomGameReporterRegistration ShooterGameReporter::mCoop("cooperative", CustomGameReporterCreateFn(&ShooterGameReporter::create), CustomGameReporterPickFn(&ShooterGameReporter::pickGameType));
    //CustomGameReporterRegistration ShooterGameReporter::mMulti("multiplayer", CustomGameReporterCreateFn(&ShooterGameReporter::create), CustomGameReporterPickFn(&ShooterGameReporter::pickGameType));
    Shooter::StatsMapConfig ShooterGameReporter::mConfigTdf;


CustomGameReporter* ShooterGameReporter::create(GameReporting::GameReport& gameReport)
{
    return BLAZE_NEW ShooterGameReporter(gameReport);
}


bool ShooterGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
{
    
    if (gameAttrs != nullptr &&
        gameAttrs->find("mode") != gameAttrs->end() &&
        blaze_stricmp(gameAttrs->find("mode")->second.c_str(), "Cooperative") == 0)
    {
        gameReportName.set("cooperative");
    }
    else
    {
        gameReportName.set("multiplayer");
    }
    return true;
}


ShooterGameReporter::ShooterGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
{
}


ShooterGameReporter::~ShooterGameReporter()
{
}


void ShooterGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
{
    const char8_t* gameReportName = mGameReport->getGameReportName();
    if (blaze_stricmp(gameReportName, "cooperative")==0)
    {
        Shooter::Report *report = static_cast<Shooter::Report*>(mGameReport->getReport());
        if (report->getPlayerReports().empty())
        {
            report->getPlayerReports().reserve(players->size());
            Shooter::PlayerReportMap& playerReportMap = report->getPlayerReports();
            for (GameReportingUtil::PlayerAttributeMap::const_iterator citPlayerIt = players->begin(), citPlayerItEnd = players->end();
                    citPlayerIt != citPlayerItEnd;
                    ++citPlayerIt)
            {
                Shooter::EntityReport *playerReport = playerReportMap.allocate_element();
                Shooter::StatsMap& statsMap = playerReport->getStats();
                int32_t gameLevelChoice = rand() % 3;
                const char8_t* gameLevel = "e"; //default to easy
                switch(gameLevelChoice)
                {
                case 1:
                    gameLevel = "m"; //medium
                case 2:
                    gameLevel = "h"; //hard
                }
                int32_t awardChoice = (rand() % 10) + 1;
                StringBuilder sb(nullptr);
                sb << "lccp" << gameLevel << "co" << awardChoice << "_0";
                char8_t buff[64];
                blaze_snzprintf(buff, 64, "%s%d", sb.get(), 0);
                statsMap[buff] = (float)(rand() % 3) + 1;
                blaze_snzprintf(buff, 64, "%s%d", sb.get(), 1);
                statsMap[buff] = (float)TimeValue::getTimeOfDay().getSec();

                switch(gameLevelChoice)
                {
                case 0:
                    gameLevel = "easy";
                case 1:
                    gameLevel = "medium";
                case 2:
                    gameLevel = "hard";
                }

                sb.reset();
                sb << "c_co" << awardChoice << "_coop" << gameLevel << "_lct_glva";

                statsMap[sb.get()] = (float)(rand() % 1000000);
                statsMap["c___k_g"] = (float)(rand() % 20);
                statsMap["c___hsh_g"] = (float)(rand() % 20);
                statsMap["c___de_g"] = (float)(rand() % 20);
                statsMap["sc_coop"] = (float)(rand() % 20);
                statsMap["rank"] = (float)(rand() % 1000000);

                playerReportMap[citPlayerIt->first] = playerReport;          
            }
        }
    } 
    else
    {
        Shooter::Report *report = static_cast<Shooter::Report*>(mGameReport->getReport());
        if (report->getPlayerReports().empty())
        {
            report->getPlayerReports().reserve(players->size());
            Shooter::PlayerReportMap& playerReportMap = report->getPlayerReports();
            for (GameReportingUtil::PlayerAttributeMap::const_iterator citPlayerIt = players->begin(), citPlayerItEnd = players->end();
                    citPlayerIt != citPlayerItEnd;
                    ++citPlayerIt)
            {
                Shooter::EntityReport *playerReport = playerReportMap.allocate_element();
                Shooter::StatsMap& statsMap = playerReport->getStats();

                Shooter::StatValueScopeMap::const_iterator itr = mConfigTdf.getStatValueScope().begin();
                Shooter::StatValueScopeMap::const_iterator end = mConfigTdf.getStatValueScope().end();
                for (;itr != end; ++itr)
                {
                    int64_t rangeStart = itr->second->begin()->first;
                    int64_t rangeEnd = itr->second->begin()->second;
                    statsMap[itr->first.c_str()] = (float)(rand() % (rangeStart + 1 - rangeEnd)) + rangeStart;
                }
                playerReportMap[citPlayerIt->first] = playerReport;          
            }
        }
    }
}

} //Stress

} //Blaze

