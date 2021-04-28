/**************************************************************************************************/
/*!
    \file otpreportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/otpreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"

#include "otpreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration SportsOTPGameReporter::mOnline("gameType5", CustomGameReporterCreateFn(&SportsOTPGameReporter::create), CustomGameReporterPickFn(&SportsOTPGameReporter::pickGameType));
        

        CustomGameReporter* SportsOTPGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW SportsOTPGameReporter(gameReport);
        }


        bool SportsOTPGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType5");    
            return true;
        }


        SportsOTPGameReporter::SportsOTPGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        SportsOTPGameReporter::~SportsOTPGameReporter()
        {
            /* empty */
        }


        void SportsOTPGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
        {
                SportsOSDKGameReportBase::OSDKReport* osdkReport = static_cast<SportsOSDKGameReportBase::OSDKReport*>(mGameReport->getReport());

                SportsOSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport->getGameReport();
                osdkGameReport.setCategoryId(rand() % 5 + 1);
                osdkGameReport.setFinishedStatus(rand() % 5);
                osdkGameReport.setGameTime(rand() % 1000 + 1);
                osdkGameReport.setSponsoredEventId(rand() % 100 + 1);
                osdkGameReport.setRank(true);

                SportsOTPReportBase::GameReport *customGameReport = BLAZE_NEW SportsOTPReportBase::GameReport;
                customGameReport->setMember(rand() % 20 + 1);
                customGameReport->setMom("RANDOM WINNER");
                osdkGameReport.setCustomGameReport(*customGameReport);

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

                        SportsOTPReportBase::PlayerReport *sportsOTPPlayerReport = BLAZE_NEW SportsOTPReportBase::PlayerReport;
                        sportsOTPPlayerReport->setCleanSheetsAny((uint32_t)(rand() % 5));
                        sportsOTPPlayerReport->setCleanSheetsDef((uint32_t)(rand() % 5));
                        sportsOTPPlayerReport->setCleanSheetsGoalKeeper((uint32_t)(rand() % 5));
                        sportsOTPPlayerReport->setManOfTheMatch((uint32_t)(rand() % 2));
                        sportsOTPPlayerReport->setOtpGames((uint32_t)(rand() % 10));
                        sportsOTPPlayerReport->setPos((uint32_t)(rand() % 10 + 1));

                        Sports::CommonPlayerReport& commonPlayerReport = sportsOTPPlayerReport->getCommonPlayerReport();
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
                        
                        playerReport->setCustomPlayerReport(*sportsOTPPlayerReport);
                        playerReportMap[citPlayerIt->first] = playerReport;
                    }
                }
        }
    }    // Stress
}    // Blaze

