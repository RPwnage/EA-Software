/*************************************************************************************************/
/*!
    \file   skilldampingutil.h


    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_SKILLDAMPINGUTIL
#define BLAZE_GAMEREPORTING_SKILLDAMPINGUTIL

#include "gamereporting/tdf/gamehistory.h"

namespace Blaze
{
namespace GameReporting
{

class GameReportingSlaveImpl;

class SkillDampingUtil
{
    NON_COPYABLE(SkillDampingUtil);

public:
    SkillDampingUtil(GameReportingSlaveImpl& component)
        : mComponent(component)
    {
    }

    ~SkillDampingUtil()
    {
    }

    uint32_t lookupDampingPercent(uint32_t count, const char8_t* dampingTableName);

private:
    GameReportingSlaveImpl& mComponent;
};

} //namespace GameReporting
} //namespace Blaze

#endif
