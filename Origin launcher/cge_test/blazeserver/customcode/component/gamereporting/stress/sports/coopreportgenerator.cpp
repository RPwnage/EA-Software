/**************************************************************************************************/
/*!
    \file coopreportgenerator.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/coopreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"

#include "coopreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration CoopGameReporter::mOnline("gameType3", CustomGameReporterCreateFn(&CoopGameReporter::create), CustomGameReporterPickFn(&CoopGameReporter::pickGameType));
        

        CustomGameReporter* CoopGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW CoopGameReporter(gameReport);
        }


        bool CoopGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType3");    
            return true;
        }


        CoopGameReporter::CoopGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        CoopGameReporter::~CoopGameReporter()
        {
            /* empty */
        }


        void CoopGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
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

                        CoopReportBase::PlayerReport *coopPlayerReport = BLAZE_NEW CoopReportBase::PlayerReport;
                        coopPlayerReport->setHits((uint32_t)(rand() % 20));
                        coopPlayerReport->setMisses((uint32_t)(rand() % 20));
                        coopPlayerReport->setServes((uint32_t)(rand() % 20));

                        playerReport->setCustomPlayerReport(*coopPlayerReport);
                        playerReportMap[citPlayerIt->first] = playerReport;
                    }
                }
        }
    }    // Stress
}    // Blaze

