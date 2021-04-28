/*************************************************************************************************/
/*!
\file   skillconfig.cpp


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereporting/skillconfig.h"

namespace Blaze
{
namespace GameReporting
{

SkillDampingTableCollection::~SkillDampingTableCollection()
{
    mSkillDampingTableMap.clear();
}

bool SkillDampingTableCollection::init(const SkillInfoConfig& skillInfoConfig)
{
    mSkillDampingTableMap.clear();

    const DampingTableList &dampingTableList = skillInfoConfig.getDampingTables();
    for (DampingTableList::const_iterator it = dampingTableList.begin(); it != dampingTableList.end(); it++)
    {
        const char8_t* tableName = (*it)->getName();
        const SkillDampingTable& dampingTable = (*it)->getTable();
        mSkillDampingTableMap.insert(eastl::make_pair(tableName, &dampingTable));
    }

    return true;
}

const SkillDampingTable* SkillDampingTableCollection::getSkillDampingTable(const char8_t* name)
{
    const SkillDampingTable* skillDampingTable = nullptr;

    SkillDampingTableMap::iterator tableIter = mSkillDampingTableMap.find(name);
    if(tableIter != mSkillDampingTableMap.end())
    {
        skillDampingTable = tableIter->second;
    }
    else
    {
        //need to make a temp table and log error
        ERR_LOG("[SkillDampingTableCollection].getSkillDampingTable: skillDampingTable " << name << " not present in map.");
    }

    return skillDampingTable;
}

}
}

