/**************************************************************************************************/
/*!
    \file clubotpreportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/clubreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkclubreport_server.h"

#include "clubotpreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration SportsClubOTPGameReporter::mOnline("gameType9", CustomGameReporterCreateFn(&SportsClubOTPGameReporter::create), CustomGameReporterPickFn(&SportsClubOTPGameReporter::pickGameType));
        

        CustomGameReporter* SportsClubOTPGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW SportsClubOTPGameReporter(gameReport);
        }


        bool SportsClubOTPGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType9");    
            return true;
        }


        SportsClubOTPGameReporter::SportsClubOTPGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        SportsClubOTPGameReporter::~SportsClubOTPGameReporter()
        {
            /* empty */
        }


        void SportsClubOTPGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
        {
            uint64_t randomClubId = (uint64_t)(rand() % 1000) + 1;
            uint32_t randomClubSize = (uint32_t)(rand() % 4) + 1;

            SportsOSDKGameReportBase::OSDKReport* osdkReport = static_cast<SportsOSDKGameReportBase::OSDKReport*>(mGameReport->getReport());

            SportsOSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport->getGameReport();
            osdkGameReport.setCategoryId((uint16_t)(rand() % 5 + 1));
            osdkGameReport.setFinishedStatus((uint16_t)(rand() % 5));
            osdkGameReport.setGameTime((uint16_t)(rand() % 1000 + 1));
            osdkGameReport.setSponsoredEventId((uint16_t)(rand() % 100 + 1));
            osdkGameReport.setRank(true);

            SportsOSDKClubGameReportBase::OSDKClubGameReport* osdkClubGameReport = BLAZE_NEW SportsOSDKClubGameReportBase::OSDKClubGameReport;
            osdkClubGameReport->setChallenge((uint16_t)(rand() % 10));
            osdkClubGameReport->setClubGameKey("RANDOM CLUB GAMEKEY");
            osdkClubGameReport->setMember((uint16_t)(rand() % 10));

            SportsClubReportBase::SportsClubsGameReport* customClubGameReport = BLAZE_NEW SportsClubReportBase::SportsClubsGameReport;
            customClubGameReport->setMom("RANDOM WINNER");
            osdkClubGameReport->setCustomClubGameReport(*customClubGameReport);

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

                    SportsOSDKClubGameReportBase::OSDKClubPlayerReport *osdkClubPlayerReport = BLAZE_NEW SportsOSDKClubGameReportBase::OSDKClubPlayerReport;
                    osdkClubPlayerReport->setChallenge((uint32_t)(rand() % 10));
                    osdkClubPlayerReport->setChallengePoints((uint32_t)(rand() % 30));
                    osdkClubPlayerReport->setClubGames((uint32_t)(rand() % 10));
                    osdkClubPlayerReport->setClubId((uint32_t)(rand() % randomClubSize) + randomClubId);
                    osdkClubPlayerReport->setH2hPoints((uint32_t)(rand() % 10));
                    osdkClubPlayerReport->setMinutes((uint32_t)(rand() % 60));
                    osdkClubPlayerReport->setPos((uint32_t)(rand() % 20));

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
            SportsOSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& osdkClubReportsMap = osdkClubReport->getClubReports();
            osdkClubReportsMap.reserve(randomClubSize);

            for (uint32_t i = 0; i < randomClubSize; ++i)
            {
                SportsOSDKClubGameReportBase::OSDKClubClubReport* osdkClubClubReport = osdkClubReportsMap.allocate_element();
                osdkClubClubReport->setGameResult((uint16_t)(rand() % 10));
                osdkClubClubReport->setClubId(randomClubId + i);
                osdkClubClubReport->setClubRegion((uint16_t)(rand() % 20001) + 10000);
                osdkClubClubReport->setH2hPoints((uint16_t)(rand() % 10));
                osdkClubClubReport->setHome((uint16_t)(rand() % 1) != 0);
                osdkClubClubReport->setLosses((uint16_t)(rand() % 1));
                osdkClubClubReport->setScore((uint16_t)(rand() % 10));
                osdkClubClubReport->setSkill((uint16_t)(rand() % 10));
                osdkClubClubReport->setSkillPoints((uint16_t)(rand() % 20));
                osdkClubClubReport->setTeam((uint16_t)(rand() % 1));
                osdkClubClubReport->setTies((uint16_t)(rand() % 1));
                osdkClubClubReport->setWinnerByDnf((uint16_t)(rand() % 1));
                osdkClubClubReport->setWins((uint16_t)(rand() % 1));

                SportsClubReportBase::SportsClubsClubReport* sportsClubsClubReport = BLAZE_NEW SportsClubReportBase::SportsClubsClubReport;
                sportsClubsClubReport->setCleanSheets((uint16_t)(rand() % 10));
                sportsClubsClubReport->setGoals((uint16_t)(rand() % 10));
                sportsClubsClubReport->setGoalsAgainst((uint16_t)(rand() % 10));

                osdkClubClubReport->setCustomClubClubReport(*sportsClubsClubReport);
                osdkClubReportsMap[osdkClubClubReport->getClubId()] = osdkClubClubReport;
            }
            osdkReport->setTeamReports(*osdkClubReport);
        }
    }    // Stress
}    // Blaze

