/**************************************************************************************************/
/*!
    \file clubchampreportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/clubchampreport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"

#include "clubchampreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration ClubChampGameReporter::mOnline("gameType11", CustomGameReporterCreateFn(&ClubChampGameReporter::create), CustomGameReporterPickFn(&ClubChampGameReporter::pickGameType));
        

        CustomGameReporter* ClubChampGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW ClubChampGameReporter(gameReport);
        }


        bool ClubChampGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType11");    
            return true;
        }


        ClubChampGameReporter::ClubChampGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        ClubChampGameReporter::~ClubChampGameReporter()
        {
            /* empty */
        }


        void ClubChampGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
        {
            SportsOSDKGameReportBase::OSDKReport* osdkReport = static_cast<SportsOSDKGameReportBase::OSDKReport*>(mGameReport->getReport());

            SportsOSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport->getGameReport();
            osdkGameReport.setCategoryId(rand() % 5 + 1);
            osdkGameReport.setFinishedStatus(rand() % 5);
            osdkGameReport.setGameTime(rand() % 1000 + 1);
            osdkGameReport.setSponsoredEventId(rand() % 100 + 1);
            osdkGameReport.setRank(true);

            ClubChampReportBase::GameReport *clubChampGameReport = BLAZE_NEW ClubChampReportBase::GameReport;
            clubChampGameReport->setClubChampId(0);

            osdkGameReport.setCustomGameReport(*clubChampGameReport);
        }
    }    // Stress
}    // Blaze

