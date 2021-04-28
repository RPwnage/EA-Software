/*************************************************************************************************/
/*!
    \file shooterreportgenerator.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_CUSTOM_SHOOTER_REPORT_GENERATOR_H
#define BLAZE_STRESS_CUSTOM_SHOOTER_REPORT_GENERATOR_H

#include "tools/stress/gamereportingutil.h"
#include "customcode/component/gamereporting/stress/shooter/tdf/shooterreport.h"


namespace Blaze
{

namespace Stress
{

///////////////////////////////////////////////////////////////////////////////
//  class ShooterGameReporter
//  
//  
class ShooterGameReporter : public CustomGameReporter
{
    NON_COPYABLE(ShooterGameReporter);

public:
    static CustomGameReporter* create(GameReporting::GameReport& gameReport);
    static bool pickGameType(GameManager::GameReportName& gameReportName, const Collections::AttributeMap* gameAttrs);

public:
    ShooterGameReporter(GameReporting::GameReport& report);
    ~ShooterGameReporter() override;

    //  allows custom code to fill in the game report during a game session (i.e. start of game, replay, post-game)
    void update(const Collections::AttributeMap* gameAttrs, const GameReportingUtil::PlayerAttributeMap* players) override; 

private:
    static CustomGameReporterRegistration mCoop;
    static CustomGameReporterRegistration mMulti;
    static Blaze::GameReporting::Shooter::StatsMapConfig mConfigTdf;
};


}   // Stress
}   // Blaze


#endif
