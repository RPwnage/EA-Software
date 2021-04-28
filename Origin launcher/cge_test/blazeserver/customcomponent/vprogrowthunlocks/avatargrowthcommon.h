/*************************************************************************************************/
/*!
	\file   avatargrowthcommon.h

	\brief	Common class shared between vprogrowthunlocks on the server and
			AvatarGrowthService on the client.

	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#pragma once

#include "EASTL/array.h"
#include "EASTL/set.h"
#include "EASTL/vector.h"

#if defined(FB_FIFA)
#include "fifaonline/component/vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"
#else
#include "vprogrowthunlocks/tdf/vprogrowthunlockstypes.h"
#endif

namespace EA
{
	namespace Json
	{
		class JsonDomDocument; 
		class JsonDomNode;  
	}
}


class AvatarGrowthCommon
{
public:
	AvatarGrowthCommon(EA::Allocator::ICoreAllocator *allocator);
	~AvatarGrowthCommon();

    struct LevelInfo
    {
		LevelInfo()
		{
			Clear();
		}

		void Clear()
		{
			mLevel = 1;
			mXpToNextLevel = 0;
			mPercentComplete = 100;
		}

        uint32_t mLevel;
        uint32_t mXpToNextLevel;
        uint32_t mPercentComplete;
    };

	// Player growth calculations
	uint32_t calculateXPRewardsFromMatch(const Blaze::VProGrowthUnlocks::MatchEventsMap &matchEventsMap) const;
	void calculatePlayerLevel(uint32_t overallXP, LevelInfo& levelInfo) const;
	uint32_t calculateSkillPointsRewards(uint32_t playerLevel) const;
	uint32_t getMaxXP() const;
	uint32_t getXpRequiredToLevelUp(uint32_t level) const;


	// Config file loading
	enum Result { Success, CannotOpenFile, CannotReadFile, InvalidJson, MissingJsonField, ValidationFailed };

	Result loadPlayerGrowthConfigFile(const eastl::vector<char> &fileData);
	Result loadSkillTreeConfigFile(const eastl::vector<char> &fileData);
	Result loadPerksConfigFile(const eastl::vector<char> &fileData);

	Blaze::VProGrowthUnlocks::PlayerGrowthConfig &getPlayerGrowthConfig() { return mPlayerGrowthConfig; }
	Blaze::VProGrowthUnlocks::MatchEventXPRewardsMap &getXPOverrideTable() { return mXPOverrideTable; }
	Blaze::VProGrowthUnlocks::SkillTree &getSkillTree() { return mSkillTree; }
	Blaze::VProGrowthUnlocks::PerkSlotsMap &getPerkSlots() { return mPerkSlots; }
	Blaze::VProGrowthUnlocks::PerksMap &getPerks() { return mPerks; }
	Blaze::VProGrowthUnlocks::PerkCategoriesMap &getPerkCategories() { return mPerkCategories; }

	void setXPOverrideTable(const Blaze::VProGrowthUnlocks::MatchEventXPRewardsMap &xpOverrideTable);

	
	// Validate user data
	bool validateLoadOutTraits(const Blaze::VProGrowthUnlocks::VProLoadOut *loadOut, uint32_t skillPointsGranted) const;
	bool validateLoadOutPerks(const Blaze::VProGrowthUnlocks::VProLoadOut *loadOut, uint32_t playerLevel) const;
	static eastl::set<Blaze::VProGrowthUnlocks::UnlockableId> getUnlockedTraitsFromLoadOut(const Blaze::VProGrowthUnlocks::VProLoadOut *loadOut);


	typedef eastl::array<uint64_t, Blaze::VProGrowthUnlocks::VPRO_LOADOUT_UNLOCKS_COUNT> UnlocksBitMask;
	typedef eastl::vector<Blaze::VProGrowthUnlocks::UnlockableId> UnlockableIdList;

	static UnlockableIdList convertUnlocksBitMaskToIdList(const UnlocksBitMask& unlocksBitMask);
	static UnlocksBitMask convertUnlocksIdListToBitMask(const UnlockableIdList& unlocksList);


private:
	// Parse JSON config files
	Result loadJsonFile(const eastl::vector<char> &fileData, EA::Json::JsonDomDocument *jsonDocument);
	static bool loadJsonArray(EA::Json::JsonDomNode *jsonNode, Blaze::VProGrowthUnlocks::UnlockableIdList *vector, const char *vectorName);
	static bool loadJsonKeyValueMap(EA::Json::JsonDomNode *jsonNode, EA::TDF::TdfPrimitiveMap<uint32_t, uint32_t> *map,
		const char *mapName, const char *mapKeyName, const char *mapValueName);
	static bool loadJsonMatchXPRewardsMap(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::MatchEventXPRewardsMap *map);
	static bool loadJsonSkillTree(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::SkillTree *skillTree);
	static bool loadJsonPerkSlots(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::PerkSlotsMap *perkSlots);
	static bool loadJsonPerks(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::PerksMap *perks);
	static bool loadJsonPerkCategories(EA::Json::JsonDomDocument *jsonDocument, Blaze::VProGrowthUnlocks::PerkCategoriesMap *perkCategories);


	// Validate loaded structures
	static bool validateSkillTree(const Blaze::VProGrowthUnlocks::SkillTree *skillTree);


private:
	EA::Allocator::ICoreAllocator *mAllocator;

	Blaze::VProGrowthUnlocks::PlayerGrowthConfig mPlayerGrowthConfig;
	Blaze::VProGrowthUnlocks::MatchEventXPRewardsMap mXPOverrideTable;
	Blaze::VProGrowthUnlocks::SkillTree mSkillTree;
	Blaze::VProGrowthUnlocks::PerkSlotsMap mPerkSlots;
	Blaze::VProGrowthUnlocks::PerksMap mPerks;
	Blaze::VProGrowthUnlocks::PerkCategoriesMap mPerkCategories;
};
