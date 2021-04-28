/**************************************************************************************************/
/*!
    \file skillreportgenerator.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/**************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/locales.h"

#include "customcode/component/gamereporting/stress/sports/tdf/skillgamereport_server.h"
#include "customcode/component/gamereporting/stress/sports/tdf/osdkreport_server.h"

#include "skillreportgenerator.h"


using namespace Blaze::GameReporting;

namespace Blaze
{
    namespace Stress
    {
        CustomGameReporterRegistration SportsSkillGameReporter::mOffline("gameType21", CustomGameReporterCreateFn(&SportsSkillGameReporter::create), CustomGameReporterPickFn(&SportsSkillGameReporter::pickGameType));
        

        CustomGameReporter* SportsSkillGameReporter::create(GameReporting::GameReport& gameReport)
        {
            return BLAZE_NEW SportsSkillGameReporter(gameReport);
        }


        bool SportsSkillGameReporter::pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs)
        {
            gameReportName.set("gameType21");    
            return true;
        }


        SportsSkillGameReporter::SportsSkillGameReporter(Blaze::GameReporting::GameReport& gameReport) : CustomGameReporter(gameReport)
        {
            /* empty */
        }
        SportsSkillGameReporter::~SportsSkillGameReporter()
        {
            /* empty */
        }


        void SportsSkillGameReporter::update(const Collections::AttributeMap *gameAttrs, const GameReportingUtil::PlayerAttributeMap *players)
        {
            Sports::SkillGameReport* skillGameReport = static_cast<Sports::SkillGameReport*>(mGameReport->getReport());
            skillGameReport->setSkillgame((uint16_t)(rand() % 40 + 1));
            skillGameReport->setScore((uint16_t)(rand() % 10));
            if (players->begin() != players->end())
            {
                GameManager::PlayerId playerId = players->begin()->first;
                skillGameReport->setPlayerid(playerId);
            }
        }
    }    // Stress
}    // Blaze

