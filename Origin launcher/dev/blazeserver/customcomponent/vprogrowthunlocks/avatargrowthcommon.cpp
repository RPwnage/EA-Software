/*************************************************************************************************/
/*!
	\file   avatargrowthcommon.cpp

	\brief	Common class shared between vprogrowthunlocks on the server and
			AvatarGrowthService on the client.

	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#if !defined(FB_FIFA)
#include "framework/blaze.h"
#endif

#include "avatargrowthcommon.h"

#include "EASTL/array.h"
#include "EAJson/JsonDomReader.h"

using UnlockableId = Blaze::VProGrowthUnlocks::UnlockableId;


AvatarGrowthCommon::AvatarGrowthCommon(EA::Allocator::ICoreAllocator *allocator)
	: mAllocator(allocator)
	, mPlayerGrowthConfig(*allocator)
	, mXPOverrideTable(*allocator)
	, mSkillTree(*allocator)
	, mPerkSlots(*allocator)
	, mPerks(*allocator)
	, mPerkCategories(*allocator)
{
}

AvatarGrowthCommon::~AvatarGrowthCommon()
{
}

/**
 * Calculates XP rewards from match events.
 */
uint32_t AvatarGrowthCommon::calculateXPRewardsFromMatch(const Blaze::VProGrowthUnlocks::MatchEventsMap &matchEventsMap) const
{
	uint32_t xpReward = 0;

	// Calculate XP reward from match events
	for (const auto &matchEvent : matchEventsMap)
	{
		uint32_t id = matchEvent.first;
		uint32_t count = matchEvent.second;

		// Check the XP override table first
		auto iter = mXPOverrideTable.find(id);
		if (iter != mXPOverrideTable.end())
		{
			count = eastl::min(count, iter->second->getMaxEventsCount());
			xpReward += iter->second->getXPReward() * count;
		}
		else
		{
			// Check the player_growth.json table next
			iter = mPlayerGrowthConfig.getMatchEventXPRewards().find(id);
			if (iter != mPlayerGrowthConfig.getMatchEventXPRewards().end())
			{
				count = eastl::min(count, iter->second->getMaxEventsCount());
				xpReward += iter->second->getXPReward() * count;
			}
		}
	}

	return xpReward;
}

/**
 * Calculates player level based on their overall XP value.
 * PlayerLevelsMap maps overall XP -> player level.
 * To find the player level we find the upper bound of overall XP, which will be the next value
 * strictly greater than the current overallXP, then get the previous level.
 */
void AvatarGrowthCommon::calculatePlayerLevel(uint32_t overallXP, LevelInfo& levelInfo) const
{
	levelInfo.Clear();

	const Blaze::VProGrowthUnlocks::PlayerLevelsMap &playerLevels = mPlayerGrowthConfig.getPlayerLevelsMap();

	if (!playerLevels.empty())
	{
		const auto nextLevelIter = playerLevels.upper_bound(overallXP);
		const auto currentLevelIter = (nextLevelIter == playerLevels.begin()) ? nextLevelIter : eastl::prev(nextLevelIter);

		levelInfo.mLevel = currentLevelIter->second;
		if (nextLevelIter != playerLevels.end())
		{
			levelInfo.mXpToNextLevel = nextLevelIter->first - overallXP;

			uint32_t divisor = (nextLevelIter->first - currentLevelIter->first);
			levelInfo.mPercentComplete = (divisor > 0) ? (((overallXP - currentLevelIter->first) * 100) / divisor) : 0;
		}
	}
}

/**
 * Calculates the overall number of awarded skill points for a given player level.
 */
uint32_t AvatarGrowthCommon::calculateSkillPointsRewards(uint32_t playerLevel) const
{
	const Blaze::VProGrowthUnlocks::SkillPointRewardsMap &skillPointRewards = mPlayerGrowthConfig.getSkillPointRewardsMap();
	const auto iter = skillPointRewards.find(playerLevel);
	return iter != skillPointRewards.end() ? iter->second : 0;
}

/**
 * Returns max XP value (the last XP value in PlayerLevelsMap).
 */
uint32_t AvatarGrowthCommon::getMaxXP() const
{
	const Blaze::VProGrowthUnlocks::PlayerLevelsMap &playerLevels = mPlayerGrowthConfig.getPlayerLevelsMap();
	return !playerLevels.empty() ? playerLevels.rbegin()->first : 0;
}

uint32_t AvatarGrowthCommon::getXpRequiredToLevelUp(uint32_t level) const
{
	const Blaze::VProGrowthUnlocks::PlayerLevelsMap &playerLevels = mPlayerGrowthConfig.getPlayerLevelsMap();

	for (const auto& it : playerLevels)
	{
		if (it.second == level)
		{
			return it.first;
		}
	}

	return 0;
}

/**
 * Loads player_growth.json config file.
 */
AvatarGrowthCommon::Result AvatarGrowthCommon::loadPlayerGrowthConfigFile(const eastl::vector<char>& fileData)
{
	EA::Json::JsonDomDocument jsonDocument(mAllocator);
	Result result = loadJsonFile(fileData, &jsonDocument);

	if (result == Success)
	{
		bool loaded = true;

		loaded &= loadJsonMatchXPRewardsMap(&jsonDocument, &mPlayerGrowthConfig.getMatchEventXPRewards());
		loaded &= loadJsonKeyValueMap(&jsonDocument, &mPlayerGrowthConfig.getPlayerLevelsMap(), "/playerLevels", "/xpRequired", "/level");
		loaded &= loadJsonKeyValueMap(&jsonDocument, &mPlayerGrowthConfig.getSkillPointRewardsMap(), "/skillPointRewards", "/level", "/spReward");

		result = loaded ? Success : MissingJsonField;
	}

	return result;
}

/**
 * Loads skill_tree.json config file.
 */
AvatarGrowthCommon::Result AvatarGrowthCommon::loadSkillTreeConfigFile(const eastl::vector<char>& fileData)
{
	EA::Json::JsonDomDocument jsonDocument(mAllocator);
	Result result = loadJsonFile(fileData, &jsonDocument);

	if (result == Success)
	{
		result = loadJsonSkillTree(&jsonDocument, &mSkillTree) ? Success : MissingJsonField;
		if (result == Success)
		{
			result = validateSkillTree(&mSkillTree) ? Success : ValidationFailed;
		}
	}

	return result;
}

/**
 * Loads perks.json config file.
 */
AvatarGrowthCommon::Result AvatarGrowthCommon::loadPerksConfigFile(const eastl::vector<char>& fileData)
{
	EA::Json::JsonDomDocument jsonDocument(mAllocator);
	Result result = loadJsonFile(fileData, &jsonDocument);

	if (result == Success)
	{
		bool loaded = true;

		loaded &= loadJsonPerkSlots(&jsonDocument, &mPerkSlots);
		loaded &= loadJsonPerks(&jsonDocument, &mPerks);
		loaded &= loadJsonPerkCategories(&jsonDocument, &mPerkCategories);

		result = loaded ? Success : MissingJsonField;
	}

	return result;
}

/**
 * Sets XP override table (used on the client).
 */
void AvatarGrowthCommon::setXPOverrideTable(const Blaze::VProGrowthUnlocks::MatchEventXPRewardsMap &xpOverrideTable)
{
	xpOverrideTable.copyInto(mXPOverrideTable);
}

/**
 * Validates unlocked traits in the load-out received from the client.
 * Makes sure all the dependencies are unlocked and the number of consumed skill points is correct.
 */
bool AvatarGrowthCommon::validateLoadOutTraits(const Blaze::VProGrowthUnlocks::VProLoadOut *loadOut, uint32_t skillPointsGranted) const
{
	const eastl::set<UnlockableId> unlockedTraits = getUnlockedTraitsFromLoadOut(loadOut);
	uint32_t spConsumed = 0;

	// Validate skill tree dependencies
	for (UnlockableId traitId : unlockedTraits)
	{
		auto traitIter = mSkillTree.find(traitId);
		if (traitIter == mSkillTree.end())
		{
			// Trait ID not found in the skill tree
			return false;
		}

		for (UnlockableId dependencyId : traitIter->second->getDependencies())
		{
			if (unlockedTraits.count(dependencyId) == 0)
			{
				// Dependency is not unlocked
				return false;
			}
		}

		++spConsumed;
	}

	// Validate consumed skill points
	return spConsumed == loadOut->getSPConsumed() && spConsumed <= loadOut->getSPGranted() && spConsumed <= skillPointsGranted;
}

/**
 * Validates perks assignment in the load-out received from the client.
 * Makes sure all the perks and perk slots are unlocked.
 */
bool AvatarGrowthCommon::validateLoadOutPerks(const Blaze::VProGrowthUnlocks::VProLoadOut *loadOut, uint32_t playerLevel) const
{
	if (loadOut->getPerksAssignment().size() != mPerkSlots.size())
	{
		// Number of perk slots in the load-out doesn't match the config
		return false;
	}

	for (auto iter = mPerkSlots.begin(); iter != mPerkSlots.end(); ++iter)
	{
		if (loadOut->getPerksAssignment()[iter->first] != Blaze::VProGrowthUnlocks::UNASSIGNED_UNLOCKABLE_ID && playerLevel < iter->second->getUnlockLevel())
		{
			// Perk slot is not unlocked
			return false;
		}
	}

	for (UnlockableId assignedPerkId : loadOut->getPerksAssignment())
	{
		if (assignedPerkId != Blaze::VProGrowthUnlocks::UNASSIGNED_UNLOCKABLE_ID)
		{
			auto iter = mPerks.find(assignedPerkId);
			if (iter == mPerks.end() || playerLevel < iter->second->getUnlockLevel())
			{
				// Perk ID not found or perk is not unlocked
				return false;
			}
		}
	}

	return true;
}

/**
 * Converts bit masks in the load-out to a list of unlocked traits IDs.
 */
eastl::set<Blaze::VProGrowthUnlocks::UnlockableId> AvatarGrowthCommon::getUnlockedTraitsFromLoadOut(const Blaze::VProGrowthUnlocks::VProLoadOut *loadOut)
{
	eastl::set<UnlockableId> unlockedTraits;

	eastl::array<uint64_t, Blaze::VProGrowthUnlocks::VPRO_LOADOUT_UNLOCKS_COUNT> unlocksArray
		{ { loadOut->getUnlocks1(), loadOut->getUnlocks2(), loadOut->getUnlocks3() } };

	UnlockableId unlockableId = 0;
	for (uint64_t unlocks : unlocksArray)
	{
		for (int i = 0; i < 64; ++i)
		{
			if ((unlocks & 1) != 0)
			{
				unlockedTraits.insert(unlockableId);
			}
			unlocks >>= 1;
			++unlockableId;
		}
	}

	return unlockedTraits;
}

AvatarGrowthCommon::UnlockableIdList AvatarGrowthCommon::convertUnlocksBitMaskToIdList(const UnlocksBitMask& unlocksBitMask)
{
	UnlockableIdList unlocksList;

	UnlockableId unlockableId = 0;
	for (uint64_t unlocks : unlocksBitMask)
	{
		for (int i = 0; i < 64; ++i)
		{
			if ((unlocks & 1) != 0)
			{
				unlocksList.push_back(unlockableId);
			}
			unlocks >>= 1;
			++unlockableId;
		}
	}

	return unlocksList;
}

AvatarGrowthCommon::UnlocksBitMask AvatarGrowthCommon::convertUnlocksIdListToBitMask(const UnlockableIdList& unlocksList)
{
	UnlocksBitMask unlocksBitMask;
	unlocksBitMask.fill(0);

	for (UnlockableId unlockableId : unlocksList)
	{
		int index = unlockableId / 64;
		int position = unlockableId % 64;
		uint64_t bit = 1ULL << position;

		EA_ASSERT_FORMATTED(index < Blaze::VProGrowthUnlocks::VPRO_LOADOUT_UNLOCKS_COUNT, ("AvatarGrowthCommon::convertUnlocksIdListToBitMask - Unlocks overflow: unlockableId [%d]", unlockableId));

		unlocksBitMask[index] |= bit;
	}

	return unlocksBitMask;
}

/**
 * Loads JSON file into EA::Json::JsonDomDocument structure.
 */
AvatarGrowthCommon::Result AvatarGrowthCommon::loadJsonFile(const eastl::vector<char> &fileData, EA::Json::JsonDomDocument *jsonDocument)
{
	EA::Json::JsonDomReader reader(mAllocator);
	reader.SetString(fileData.data(), fileData.size(), false);
	EA::Json::Result parseResult = reader.Build(*jsonDocument);
	return parseResult == EA::Json::kSuccess ? Success : InvalidJson;
}

/**
 * Loads a list of UnlockableIds from a JSON document.
 */
bool AvatarGrowthCommon::loadJsonArray(EA::Json::JsonDomNode *jsonNode, Blaze::VProGrowthUnlocks::UnlockableIdList *vector, const char *vectorName)
{
	const EA::Json::JsonDomArray *array = jsonNode->GetArray(vectorName);
	bool loaded = array != nullptr && !array->mJsonDomNodeArray.empty();

	if (loaded)
	{
		for (EA::Json::JsonDomNode *node : array->mJsonDomNodeArray)
		{
			const EA::Json::JsonDomInteger *integer = node->AsJsonDomInteger();
			loaded = integer != nullptr;
			if (loaded)
			{
				vector->push_back(static_cast<UnlockableId>(integer->mValue));
			}
		}
	}

	return loaded;
}

/**
 * Loads a key-value map from a JSON document.
 * Both key and value should be uint32_t.
 */
bool AvatarGrowthCommon::loadJsonKeyValueMap(EA::Json::JsonDomNode *jsonNode, EA::TDF::TdfPrimitiveMap<uint32_t, uint32_t> *map,
	const char *mapName, const char *mapKeyName, const char *mapValueName)
{
	const EA::Json::JsonDomArray *array = jsonNode->GetArray(mapName);
	bool loaded = array != nullptr && !array->mJsonDomNodeArray.empty();

	if (loaded)
	{
		map->clear();

		for (EA::Json::JsonDomNode *node : array->mJsonDomNodeArray)
		{
			const EA::Json::JsonDomInteger *key = node->GetInteger(mapKeyName);
			const EA::Json::JsonDomInteger *value = node->GetInteger(mapValueName);

			loaded = key != nullptr && value != nullptr;
			if (loaded)
			{
				uint32_t uKey = static_cast<uint32_t>(key->mValue);
				uint32_t uValue = static_cast<uint32_t>(value->mValue);

				map->insert(eastl::make_pair(uKey, uValue));
			}
		}
	}

	return loaded;
}

/**
 * Loads matchEventXPRewards table from player_growth.json file.
 */
bool AvatarGrowthCommon::loadJsonMatchXPRewardsMap(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::MatchEventXPRewardsMap *map)
{
	const EA::Json::JsonDomArray *array = jsonDocument->GetArray("/matchEventXPRewards");
	bool loaded = array != nullptr && !array->mJsonDomNodeArray.empty();

	if (loaded)
	{
		map->clear();

		for (EA::Json::JsonDomNode *node : array->mJsonDomNodeArray)
		{
			const EA::Json::JsonDomInteger *eventId = node->GetInteger("/eventId");
			const EA::Json::JsonDomInteger *xpReward = node->GetInteger("/xpReward");
			const EA::Json::JsonDomInteger *maxEventsCount = node->GetInteger("/maxEventsCount");
			const EA::Json::JsonDomString *stringId = node->GetString("/stringId");

			loaded = eventId != nullptr && xpReward != nullptr && maxEventsCount != nullptr && stringId != nullptr;
			if (loaded)
			{
				Blaze::VProGrowthUnlocks::MatchEventXPReward *matchEventXPReward = map->allocate_element();

				uint32_t uEventId = static_cast<uint32_t>(eventId->mValue);

				matchEventXPReward->setEventId(uEventId);
				matchEventXPReward->setXPReward(static_cast<uint32_t>(xpReward->mValue));
				matchEventXPReward->setMaxEventsCount(static_cast<uint32_t>(maxEventsCount->mValue));
				matchEventXPReward->setStringId(stringId->mValue.c_str());

				map->insert(eastl::make_pair(uEventId, matchEventXPReward));
			}
		}
	}

	return loaded;
}

/**
 * Loads Skill Tree from skill_tree.json file.
 */
bool AvatarGrowthCommon::loadJsonSkillTree(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::SkillTree *skillTree)
{
	const EA::Json::JsonDomArray *array = jsonDocument->mJsonDomNodeArray[0]->AsJsonDomArray();
	bool loaded = array != nullptr && !array->mJsonDomNodeArray.empty();

	if (loaded)
	{
		skillTree->clear();

		for (EA::Json::JsonDomNode *node : array->mJsonDomNodeArray)
		{
			const EA::Json::JsonDomInteger *id = node->GetInteger("/id");
			const EA::Json::JsonDomInteger *type = node->GetInteger("/type");
			const EA::Json::JsonDomInteger *category = node->GetInteger("/category");
			const EA::Json::JsonDomInteger *cost = node->GetInteger("/cost");
			const EA::Json::JsonDomString  *name = node->GetString("/name");
			const EA::Json::JsonDomString  *desc = node->GetString("/desc");
			const EA::Json::JsonDomInteger *tree = node->GetInteger("/tree");
			const EA::Json::JsonDomInteger *column = node->GetInteger("/column");
			const EA::Json::JsonDomInteger *row = node->GetInteger("/row");
			const EA::Json::JsonDomInteger *connector = node->GetInteger("/connector");
			const EA::Json::JsonDomInteger *iconId = node->GetInteger("/iconId");

			loaded = id != nullptr && type != nullptr && category != nullptr && cost != nullptr && name != nullptr && desc != nullptr
				&& tree != nullptr && column != nullptr && row != nullptr && connector != nullptr && iconId != nullptr;

			if (loaded)
			{
				Blaze::VProGrowthUnlocks::Trait *trait = skillTree->allocate_element();

				uint32_t uId = static_cast<uint32_t>(id->mValue);

				trait->setId(uId);
				trait->setType(static_cast<Blaze::VProGrowthUnlocks::TraitTypeEnum>(type->mValue));
				trait->setCategory(static_cast<uint32_t>(category->mValue));
				trait->setCost(static_cast<uint32_t>(cost->mValue));
				trait->setName(name->mValue.c_str());
				trait->setDesc(desc->mValue.c_str());
				trait->setTree(static_cast<uint32_t>(tree->mValue));
				trait->setColumn(static_cast<uint32_t>(column->mValue));
				trait->setRow(static_cast<uint32_t>(row->mValue));
				trait->setConnector(static_cast<uint32_t>(connector->mValue));
				trait->setIconId(static_cast<uint32_t>(iconId->mValue));

				for (uint32_t i = 0; i < Blaze::VProGrowthUnlocks::TRAIT_MAX_EFFECT_COUNT; ++i)
				{
					eastl::string subtypeKey(eastl::string::CtorSprintf(), "/subtype%u", i);
					eastl::string effectKey(eastl::string::CtorSprintf(), "/effect%u", i);

					const EA::Json::JsonDomInteger *subtype = node->GetInteger(subtypeKey.c_str());
					const EA::Json::JsonDomInteger *effect = node->GetInteger(effectKey.c_str());

					if (subtype != nullptr && effect != nullptr)
					{
						uint32_t uSubtype = static_cast<uint32_t>(subtype->mValue);
						uint32_t uValue = static_cast<uint32_t>(effect->mValue);

						trait->getEffects().insert(eastl::make_pair(uSubtype, uValue));
					}
				}

				loadJsonArray(node, &trait->getDependencies(), "/dependency");
				loadJsonArray(node, &trait->getChildren(), "/children");

				skillTree->insert(eastl::make_pair(uId, trait));
			}
		}
	}

	return loaded;
}

/**
 * Loads Perk Slots from perks.json file.
 */
bool AvatarGrowthCommon::loadJsonPerkSlots(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::PerkSlotsMap *perkSlots)
{
	const EA::Json::JsonDomArray *array = jsonDocument->GetArray("/perkSlots");
	bool loaded = array != nullptr && !array->mJsonDomNodeArray.empty();

	if (loaded)
	{
		perkSlots->clear();

		for (EA::Json::JsonDomNode *node : array->mJsonDomNodeArray)
		{
			const EA::Json::JsonDomInteger *id = node->GetInteger("/id");
			const EA::Json::JsonDomInteger *unlockLevel = node->GetInteger("/unlockLevel");

			loaded = id != nullptr && unlockLevel != nullptr;
			if (loaded)
			{
				Blaze::VProGrowthUnlocks::PerkSlot *perkSlot = perkSlots->allocate_element();

				UnlockableId uId = static_cast<UnlockableId>(id->mValue);

				perkSlot->setId(uId);
				perkSlot->setUnlockLevel(static_cast<uint32_t>(unlockLevel->mValue));

				perkSlots->insert(eastl::make_pair(uId, perkSlot));
			}
		}
	}

	return loaded;
}

/**
 * Loads Perks from perks.json file.
 */
bool AvatarGrowthCommon::loadJsonPerks(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::PerksMap *perks)
{
	const EA::Json::JsonDomArray *array = jsonDocument->GetArray("/perks");
	bool loaded = array != nullptr && !array->mJsonDomNodeArray.empty();

	if (loaded)
	{
		perks->clear();

		for (EA::Json::JsonDomNode *node : array->mJsonDomNodeArray)
		{
			const EA::Json::JsonDomInteger *id = node->GetInteger("/id");
			const EA::Json::JsonDomInteger *unlockLevel = node->GetInteger("/unlockLevel");
			const EA::Json::JsonDomString *name = node->GetString("/name");
			const EA::Json::JsonDomString *desc = node->GetString("/desc");
			const EA::Json::JsonDomInteger *category = node->GetInteger("/category");
			const EA::Json::JsonDomInteger *position = node->GetInteger("/position");
			const EA::Json::JsonDomInteger *iconId = node->GetInteger("/iconId");

			loaded = id != nullptr && unlockLevel != nullptr && name != nullptr && desc != nullptr
				&& category != nullptr && position != nullptr && iconId != nullptr;

			if (loaded)
			{
				Blaze::VProGrowthUnlocks::Perk *perk = perks->allocate_element();

				UnlockableId uId = static_cast<UnlockableId>(id->mValue);

				perk->setId(uId);
				perk->setUnlockLevel(static_cast<uint32_t>(unlockLevel->mValue));
				perk->setName(name->mValue.c_str());
				perk->setDesc(desc->mValue.c_str());
				perk->setCategory(static_cast<uint32_t>(category->mValue));
				perk->setPosition(static_cast<uint32_t>(position->mValue));
				perk->setIconId(static_cast<uint32_t>(iconId->mValue));

				loaded = loadJsonArray(node, &perk->getGameplayPerkIds(), "/gameplayPerkIds");
				loaded &= !perk->getGameplayPerkIds().empty();

				perks->insert(eastl::make_pair(uId, perk));
			}
		}
	}

	return loaded;
}

/**
 * Loads Perks Categories map from perks.json file.
 */
bool AvatarGrowthCommon::loadJsonPerkCategories(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::PerkCategoriesMap *perkCategories)
{
	const EA::Json::JsonDomArray *array = jsonDocument->GetArray("/perkCategories");
	bool loaded = array != nullptr && !array->mJsonDomNodeArray.empty();

	if (loaded)
	{
		perkCategories->clear();

		for (EA::Json::JsonDomNode *node : array->mJsonDomNodeArray)
		{
			const EA::Json::JsonDomInteger *id = node->GetInteger("/id");
			const EA::Json::JsonDomString *name = node->GetString("/name");

			loaded = id != nullptr && name != nullptr;
			if (loaded)
			{
				perkCategories->insert(eastl::make_pair(static_cast<uint32_t>(id->mValue), name->mValue.c_str()));
			}
		}
	}

	return loaded;
}

/**
 * Validates Skill Tree for consistency.
 */
bool AvatarGrowthCommon::validateSkillTree(const Blaze::VProGrowthUnlocks::SkillTree *skillTree)
{
	for (auto iter = skillTree->begin(); iter != skillTree->end(); ++iter)
	{
		const Blaze::VProGrowthUnlocks::TraitPtr &trait = iter->second;

		// Check trait fields for sanity
		if (trait->getType() < 0 || trait->getType() >= Blaze::VProGrowthUnlocks::TRAIT_TYPE_COUNT || trait->getCost() == 0
			|| trait->getEffects().empty() || trait->getEffects().size() > Blaze::VProGrowthUnlocks::TRAIT_MAX_EFFECT_COUNT)
		{
			return false;
		}

		// Check that each dependency has this node as a child
		for (UnlockableId dependencyId : trait->getDependencies())
		{
			auto dependencyIter = skillTree->find(dependencyId);
			if (dependencyIter == skillTree->end())
			{
				const Blaze::VProGrowthUnlocks::TraitPtr &dependency = dependencyIter->second;
				bool ok = dependency->getCategory() == trait->getCategory() && dependency->getTree() == trait->getTree()
					&& eastl::find(dependency->getChildren().begin(), dependency->getChildren().end(), trait->getId()) != dependency->getChildren().end();

				if (!ok)
				{
					return false;
				}
			}
		}

		// Check that each child has this node as a dependency
		for (UnlockableId childId : trait->getChildren())
		{
			auto childIter = skillTree->find(childId);
			if (childIter == skillTree->end())
			{
				const Blaze::VProGrowthUnlocks::TraitPtr &child = childIter->second;
				bool ok = child->getCategory() == trait->getCategory() && child->getTree() == trait->getTree()
					&& eastl::find(child->getDependencies().begin(), child->getDependencies().end(), trait->getId()) != child->getDependencies().end();

				if (!ok)
				{
					return false;
				}
			}
		}
	}
	return true;
}
