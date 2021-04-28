/*! ************************************************************************************************/
/*!
\file reteruleprovider.cpp

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/rete/reteruledefinition.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"

namespace Blaze
{
namespace GameManager
{
namespace Rete
{

void ReteRuleDefinition::setRuleDefinitionCollection(GameManager::Matchmaker::RuleDefinitionCollection& ruleDefinitionCollection)
{
    EA_ASSERT(mRuleDefinitionCollection == nullptr || mRuleDefinitionCollection == &ruleDefinitionCollection);
    mRuleDefinitionCollection = &ruleDefinitionCollection;
}

uint64_t ReteRuleDefinition::reteHash(const char8_t* str, bool garbageCollectable) const { return mRuleDefinitionCollection->getStringTable().reteHash(str, garbageCollectable); }

const char8_t* ReteRuleDefinition::reteUnHash(WMEAttribute value) const { return mRuleDefinitionCollection->getStringTable().getStringFromHash(value); }

StringTable& ReteRuleDefinition::getStringTable() const { return mRuleDefinitionCollection->getStringTable(); }

} // namespace Rete
} // namespace GameManager
} // namespace Blaze
