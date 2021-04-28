/*************************************************************************************************/
/*!
    \file clubotpreportgenerator.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_CUSTOM_CLUBOTP_REPORT_GENERATOR_H
#define BLAZE_STRESS_CUSTOM_CLUBOTP_REPORT_GENERATOR_H

#include "tools/stress/gamereportingutil.h"


namespace Blaze
{
    namespace Stress
    {
    ///////////////////////////////////////////////////////////////////////////////
    //    class SportsClubOTPGameReporter
    //    
    //    
        class SportsClubOTPGameReporter : public CustomGameReporter
        {
            NON_COPYABLE(SportsClubOTPGameReporter);

            public:
                static CustomGameReporter* create(GameReporting::GameReport& gameReport);
                static bool pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs);

            public:
                SportsClubOTPGameReporter(GameReporting::GameReport& report);
                ~SportsClubOTPGameReporter() override;

                //  allows custom code to fill in the game report during a game session (i.e. start of game, replay, post-game)
                void update(const Collections::AttributeMap* gameAttrs, const GameReportingUtil::PlayerAttributeMap* players) override; 

            private:
                static CustomGameReporterRegistration mOffline;
                static CustomGameReporterRegistration mOnline;
        };
    }    // Stress
}    // Blaze


#endif
