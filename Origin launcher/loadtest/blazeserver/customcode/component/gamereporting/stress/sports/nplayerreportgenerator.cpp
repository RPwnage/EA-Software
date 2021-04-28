/**************************************************************************************************/
/*!
    \file nplayerreportgenerator.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/nplayerreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"

#include "nplayerreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration NPlayerGameReporter::mOnline("gameType6", CustomGameReporterCreateFn(&NPlayerGameReporter::create), CustomGameReporterPickFn(&NPlayerGameReporter::pickGameType));
        

        CustomGameReporter* NPlayerGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW NPlayerGameReporter(gameReport);
        }


        bool NPlayerGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType6");    
            return true;
        }


        NPlayerGameReporter::NPlayerGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        NPlayerGameReporter::~NPlayerGameReporter()
        {
            /* empty */
        }


        void NPlayerGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
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

                        NPlayerReportBase::PlayerReport *multiPlayerReport = BLAZE_NEW NPlayerReportBase::PlayerReport;
                        multiPlayerReport->setHits((uint32_t)(rand() % 10));
                        multiPlayerReport->setMisses((uint32_t)(rand() % 10));
                        multiPlayerReport->setServes((uint32_t)(rand() % 10));

                        playerReport->setCustomPlayerReport(*multiPlayerReport);
                        playerReportMap[citPlayerIt->first] = playerReport;
                    }
                }
        }
    }    // Stress
}    // Blaze

