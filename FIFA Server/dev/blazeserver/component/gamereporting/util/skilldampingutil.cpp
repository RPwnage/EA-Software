/*************************************************************************************************/
/*!
    \file   skilldampingutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "skilldampingutil.h"

namespace Blaze
{
namespace GameReporting
{
    uint32_t SkillDampingUtil::lookupDampingPercent(uint32_t count, const char8_t* dampingTableName)
    {
        const SkillDampingTable *skillDampingTable = mComponent.getSkillDampingTable(dampingTableName);
        if (skillDampingTable == nullptr)
        {
            ERR_LOG("[SkillDampingUtil].lookupDampingPercent(): Damping table '" << dampingTableName << "' not found, returning 100.");
            return 100;
        }

        if (count < skillDampingTable->size())
        {
            TRACE_LOG("[SkillDampingUtil].lookupDampingPercent(): Damping table '" << dampingTableName << "' index " << count 
                      << " has value " << (*skillDampingTable)[count] << ".");
            return (*skillDampingTable)[count];
        }
        else
        {
            // return last defined value if number of rematches is greater than size of table.
            TRACE_LOG("[SkillDampingUtil].lookupDampingPercent(): Damping table '" << dampingTableName << "' index " << count 
                      << " past end of table, returning last defined value " << skillDampingTable->back() << ".");
            return skillDampingTable->back();
        }
 
    }

} //namespace GameReporting
} //namespace Blaze
