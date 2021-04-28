/**************************************************************************************************/
/*!
    \file clubintrareportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/h2hreportbase_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"

#include "clubintrareportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration ClubIntraH2HStatsGameReporter::mOnline("gameType14", CustomGameReporterCreateFn(&ClubIntraH2HStatsGameReporter::create), CustomGameReporterPickFn(&ClubIntraH2HStatsGameReporter::pickGameType));
        

        CustomGameReporter* ClubIntraH2HStatsGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW ClubIntraH2HStatsGameReporter(gameReport);
        }


        bool ClubIntraH2HStatsGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType14");    
            return true;
        }


        ClubIntraH2HStatsGameReporter::ClubIntraH2HStatsGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        ClubIntraH2HStatsGameReporter::~ClubIntraH2HStatsGameReporter()
        {
            /* empty */
        }


        void ClubIntraH2HStatsGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
        {
            SportsOSDKGameReportBase::OSDKReport* osdkReport = static_cast<SportsOSDKGameReportBase::OSDKReport*>(mGameReport->getReport());

            SportsOSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport->getGameReport();
            osdkGameReport.setCategoryId(rand() % 5 + 1);
            osdkGameReport.setFinishedStatus(rand() % 5);
            osdkGameReport.setGameTime(rand() % 1000 + 1);
            osdkGameReport.setSponsoredEventId(rand() % 100 + 1);
            osdkGameReport.setRank(true);

            H2HReportBase::GameReport* h2hGameReport = BLAZE_NEW H2HReportBase::GameReport;
            h2hGameReport->setClubId((uint64_t)(rand() % 1000) + 1);
            osdkGameReport.setCustomGameReport(*h2hGameReport);

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
                    playerReport->setPointsAgainst((uint32_t)(rand() % 2));
                    playerReport->setSkill((uint32_t)(rand() % 2));

                    H2HReportBase::PlayerReport *clubintraPlayerReport = BLAZE_NEW H2HReportBase::PlayerReport;
                    clubintraPlayerReport->setHits((uint16_t)(rand() % 1));
                    clubintraPlayerReport->setMisses((uint16_t)(rand() % 1));
                    clubintraPlayerReport->setServes((uint16_t)(rand() % 1));

                    playerReport->setCustomPlayerReport(*clubintraPlayerReport);
                    playerReportMap[citPlayerIt->first] = playerReport;
                }
            }
        }
    }    // Stress
}    // Blaze

