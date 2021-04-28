/**************************************************************************************************/
/*!
    \file h2hreportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/h2hreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"

#include "h2hreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{

namespace Stress
{

    CustomGameReporterRegistration SportsH2HGameReporter::mOnline("gameType0", CustomGameReporterCreateFn(&SportsH2HGameReporter::create), CustomGameReporterPickFn(&SportsH2HGameReporter::pickGameType));


CustomGameReporter* SportsH2HGameReporter::create(GameReporting::GameReport& gameReport)
{
    return BLAZE_NEW SportsH2HGameReporter(gameReport);
}


bool SportsH2HGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
{
    gameReportName.set("gameType0");    
    return true;
}


SportsH2HGameReporter::SportsH2HGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
{
}


SportsH2HGameReporter::~SportsH2HGameReporter()
{
}


void SportsH2HGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
{
    SportsOSDKGameReportBase::OSDKReport* osdkReport = static_cast<SportsOSDKGameReportBase::OSDKReport*>(mGameReport->getReport());

    SportsOSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport->getGameReport();
    osdkGameReport.setCategoryId(rand() % 5 + 1);
    osdkGameReport.setFinishedStatus(rand() % 5);
    osdkGameReport.setGameTime(rand() % 1000 + 1);
    osdkGameReport.setSponsoredEventId(rand() % 100 + 1);
    osdkGameReport.setRank(true);

    if (osdkReport->getPlayerReports().empty())
    {
        osdkReport->getPlayerReports().reserve(players->size());
        SportsOSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportMap = osdkReport->getPlayerReports();
        for (GameReportingUtil::PlayerAttributeMap::const_iterator citPlayerIt = players->begin(), citPlayerItEnd = players->end();
            citPlayerIt != citPlayerItEnd;
            ++citPlayerIt)
        {
            SportsOSDKGameReportBase::OSDKPlayerReport *playerReport = playerReportMap.allocate_element();
            playerReport->setAccountCountry(COUNTRY_DEFAULT);
            playerReport->setClientScore((uint16_t)(rand() % 20));
            playerReport->setFinishReason((uint32_t)(rand() % 3));
            playerReport->setWins((uint32_t)(rand() % 2));
            playerReport->setLosses((uint32_t)(rand() % 2));
            playerReport->setTies((uint32_t)(rand() % 2));
            playerReport->setCustomDnf((uint32_t)(rand() % 2));
            playerReport->setScore((uint32_t)(rand() % 30));
            playerReport->setTeam((uint32_t)(rand() % 100));
            playerReport->setGameResult((uint32_t)(rand() % 10));
            playerReport->setWinnerByDnf((uint32_t)(rand() % 2));

            Sports::H2HPlayerReport* h2hPlayerReport = BLAZE_NEW Sports::H2HPlayerReport;

            Sports::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport->getH2HCustomPlayerData();
            h2hCustomPlayerData.setControls((uint16_t)(rand() % 2) + 1);
            h2hCustomPlayerData.setGoalAgainst((uint16_t)(rand() % 20));
            h2hCustomPlayerData.setNbGuests((uint16_t)(rand() % 20));
            h2hCustomPlayerData.setShotsAgainst((uint16_t)(rand() % 20));
            h2hCustomPlayerData.setShotsAgainstOnGoal((uint16_t)(rand() % 20));
            h2hCustomPlayerData.setGoalAgainst((uint16_t)(rand() % 20));
            h2hCustomPlayerData.setTeam((uint16_t)(rand() % 20));
            h2hCustomPlayerData.setTeamrating((uint16_t)(rand() % 100));
            h2hCustomPlayerData.setWentToPk((uint16_t)(rand() % 20));
 
            Sports::CommonPlayerReport& commonPlayerReport = h2hPlayerReport->getCommonPlayerReport();
            commonPlayerReport.setAssists((uint16_t)(rand() % 10));
            commonPlayerReport.setCorners((uint16_t)(rand() % 10));
            commonPlayerReport.setFouls((uint16_t)(rand() % 10));
            commonPlayerReport.setGoals((uint16_t)(rand() % 10));
            commonPlayerReport.setGoalsConceded((uint16_t)(rand() % 10));
            commonPlayerReport.setOffsides((uint16_t)(rand() % 10));
            commonPlayerReport.setOwnGoals((uint16_t)(rand() % 10));
            commonPlayerReport.setPassAttempts((uint16_t)(rand() % 10));
            commonPlayerReport.setPassesMade((uint16_t)(rand() % 10));
            commonPlayerReport.setPkGoals((uint16_t)(rand() % 10));
            commonPlayerReport.setPossession((uint16_t)(rand() % 10));
            commonPlayerReport.setRating((uint16_t)(rand() % 10));
            commonPlayerReport.setRedCard((uint16_t)(rand() % 10));
            commonPlayerReport.setSaves((uint16_t)(rand() % 10));
            commonPlayerReport.setShots((uint16_t)(rand() % 10));
            commonPlayerReport.setShotsOnGoal((uint16_t)(rand() % 10));
            commonPlayerReport.setTackleAttempts((uint16_t)(rand() % 10));
            commonPlayerReport.setTacklesMade((uint16_t)(rand() % 10));
            commonPlayerReport.setYellowCard((uint16_t)(rand() % 10));

            playerReport->setCustomPlayerReport(*h2hPlayerReport);
            playerReportMap[citPlayerIt->first] = playerReport;
        }
    }
}

} //Stress

} //Blaze

