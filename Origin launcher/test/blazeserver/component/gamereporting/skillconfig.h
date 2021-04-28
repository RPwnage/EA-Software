/*************************************************************************************************/
/*!
\file   skillconfig.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_SKILL_CONFIG_H
#define BLAZE_GAMEREPORTING_SKILL_CONFIG_H

/*** Include files *******************************************************************************/
#include "EASTL/map.h"
#include "EASTL/vector.h"
#include "framework/util/shared/blazestring.h"
/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace GameReporting
{

typedef eastl::map<const char8_t*, const SkillDampingTable*, CaseInsensitiveStringLessThan> SkillDampingTableMap;

class SkillDampingTableCollection
{
public:
    SkillDampingTableCollection() {};

    ~SkillDampingTableCollection();

    bool init(const SkillInfoConfig& skillInfoConfig);

    const SkillDampingTable* getSkillDampingTable(const char8_t* name);
    const SkillDampingTableMap* getSkillDampingTableMap() const
    {
        return &mSkillDampingTableMap;
    }

private:
    SkillDampingTableMap mSkillDampingTableMap;

};

}
}

#endif //BLAZE_GAMEREPORTING_SKILL_CONFIG_H

