/**************************************************************************************************/
/*!
    \file clubreportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/clubreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkclubreport_server.h"

#include "clubreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration SportsClubGameReporter::mOnline("gameType13", CustomGameReporterCreateFn(&SportsClubGameReporter::create), CustomGameReporterPickFn(&SportsClubGameReporter::pickGameType));
        

        CustomGameReporter* SportsClubGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW SportsClubGameReporter(gameReport);
        }


        bool SportsClubGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType13");    
            return true;
        }


        SportsClubGameReporter::SportsClubGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        SportsClubGameReporter::~SportsClubGameReporter()
        {
            /* empty */
        }


        void SportsClubGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
        {
            uint64_t randomClubId = (uint64_t)(rand() % 1000) + 1;
            uint32_t randomClubSize = (uint32_t)(rand() % 4) + 1;

            SportsOSDKGameReportBase::OSDKReport* osdkReport = static_cast<SportsOSDKGameReportBase::OSDKReport*>(mGameReport->getReport());

            SportsOSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport->getGameReport();
            osdkGameReport.setCategoryId(rand() % 5 + 1);
            osdkGameReport.setFinishedStatus(rand() % 5);
            osdkGameReport.setGameTime(rand() % 1000 + 1);
            osdkGameReport.setSponsoredEventId(rand() % 100 + 1);
            osdkGameReport.setRank(true);

            SportsClubReportBase::SportsClubsGameReport* clubsGameReport = BLAZE_NEW SportsClubReportBase::SportsClubsGameReport;
            clubsGameReport->setMom("MY NAME");

            SportsOSDKClubGameReportBase::OSDKClubGameReport* osdkClubGameReport = BLAZE_NEW SportsOSDKClubGameReportBase::OSDKClubGameReport;
            osdkClubGameReport->setChallenge((uint32_t)(rand() % 10));
            osdkClubGameReport->setClubGameKey("RANDOM CLUB GAMEKEY");
            osdkClubGameReport->setMember((uint16_t)(rand() % 10));
            osdkClubGameReport->setCustomClubGameReport(*clubsGameReport);

            osdkGameReport.setCustomGameReport(*osdkClubGameReport);

            if (osdkReport->getPlayerReports().empty())
            {
                osdkReport->getPlayerReports().reserve(players->size());
                SportsOSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportMap = osdkReport->getPlayerReports();
                for (GameReportingUtil::PlayerAttributeMap::const_iterator citPlayerIt = players->begin(), citPlayerItEnd = players->end();
                    citPlayerIt != citPlayerItEnd;
                    ++citPlayerIt)
                {
                    SportsOSDKGameReportBase::OSDKPlayerReport *playerReport = playerReportMap.allocate_element();
                    playerReport->setAccountCountry((uint16_t)(rand() % 20001) + 10000);
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

                    SportsOSDKClubGameReportBase::OSDKClubPlayerReport* osdkClubPlayerReport = BLAZE_NEW SportsOSDKClubGameReportBase::OSDKClubPlayerReport;
                    osdkClubPlayerReport->setH2hPoints((uint32_t)(rand() % 10));
                    osdkClubPlayerReport->setPos((uint32_t)(rand() % 10));
                    osdkClubPlayerReport->setClubId(randomClubId);

                    SportsClubReportBase::SportsClubsPlayerReport* sportsClubPlayerReport = BLAZE_NEW SportsClubReportBase::SportsClubsPlayerReport;
                    sportsClubPlayerReport->setCleanSheetsAny((uint16_t)(rand() % 1));
                    sportsClubPlayerReport->setCleanSheetsDef((uint16_t)(rand() % 1));
                    sportsClubPlayerReport->setCleanSheetsGoalKeeper((uint16_t)(rand() % 1));
                    sportsClubPlayerReport->setManOfTheMatch((uint16_t)(rand() % 1));

                    Sports::CommonPlayerReport& commonPlayerReport = sportsClubPlayerReport->getCommonPlayerReport();
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

                    osdkClubPlayerReport->setCustomClubPlayerReport(*sportsClubPlayerReport);
                    playerReport->setCustomPlayerReport(*osdkClubPlayerReport);
                    playerReportMap[citPlayerIt->first] = playerReport;
                }
            }

            SportsOSDKClubGameReportBase::OSDKClubReport* osdkClubReport = BLAZE_NEW SportsOSDKClubGameReportBase::OSDKClubReport;
            SportsOSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& osdkClubsReportsMap = osdkClubReport->getClubReports();
            osdkClubsReportsMap.reserve(randomClubSize);

            for (uint16_t i = 0; i < randomClubSize; ++i)
            {
                SportsOSDKClubGameReportBase::OSDKClubClubReport* osdkClubClubReport = osdkClubsReportsMap.allocate_element();
                osdkClubClubReport->setClubId(randomClubId + i);
                osdkClubClubReport->setGameResult((uint16_t)(rand() % 10));
                osdkClubClubReport->setScore((uint16_t)(rand() % 10));
                osdkClubClubReport->setWinnerByDnf((uint16_t)(rand() % 1));
                osdkClubClubReport->setWins((uint16_t)(rand() % 3));
                osdkClubClubReport->setTies((uint16_t)(rand() % 3));
                osdkClubClubReport->setLosses((uint16_t)(rand() % 3));

                SportsClubReportBase::SportsClubsClubReport* sportsClubsClubReport = BLAZE_NEW SportsClubReportBase::SportsClubsClubReport;
                sportsClubsClubReport->setCleanSheets((uint16_t)(rand() % 10));
                sportsClubsClubReport->setGoals((uint16_t)(rand() % 10));
                sportsClubsClubReport->setGoalsAgainst((uint16_t)(rand() % 10));

                osdkClubClubReport->setCustomClubClubReport(*sportsClubsClubReport);
                osdkClubsReportsMap[osdkClubClubReport->getClubId()] = osdkClubClubReport;
            }

            osdkReport->setTeamReports(*osdkClubReport);
        }
    }    // Stress
}    // Blaze

