/*************************************************************************************************/
/*!
    \file   reportconfig.h
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_REPORTCONFIG_H
#define BLAZE_GAMEREPORTING_REPORTCONFIG_H

#include "gamereporting/osdk/tdf/gameosdkreport_server.h"

#define MAX_END_PHASE 2

namespace Blaze
{
namespace GameReporting
{

// ReportConfig does not exist in the OSDKGameReportBase namespace, but uses types from it
using namespace OSDKGameReportBase;

class ReportConfig
{
public:
    ReportConfig(const OSDKCustomGlobalConfig* customConfig);
    ~ReportConfig();

    static ReportConfig* createReportConfig(const OSDKCustomGlobalConfig* customConfig);

    uint16_t    getSkillLevel(uint32_t skillPoints);
	uint32_t	getNextSkillLevelPoints(uint32_t skillPoints);
	uint32_t	getPrevSkillLevelPoints(uint32_t skillPoints);
    uint32_t*   getEndPhaseMaxTime()    { return mMaxTime; }

private:
    typedef eastl::map<uint32_t, uint16_t> SkillMap;

    SkillMap                    mSkillMap;
    uint32_t                    mMaxTime[MAX_END_PHASE - 1];
};

} //namespace GameReporting
} //namespace Blaze

#endif //BLAZE_GAMEREPORTING_REPORTCONFIG_H
