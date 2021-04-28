/*************************************************************************************************/
/*!
    \file   fifa_lastopponentextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_LASTOPPONENTEXTENSIONS_H
#define BLAZE_CUSTOM_FIFA_LASTOPPONENTEXTENSIONS_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/tdf/gameosdkreport.h"
#include "gamereporting/osdk/tdf/gameosdkclubreport.h"
#include "gamereporting/processedgamereport.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

/*!
    class FifaLastOpponentExtension
*/
class IFifaLastOpponentExtension
{
public:
	IFifaLastOpponentExtension() {}
	virtual ~IFifaLastOpponentExtension() {}

	virtual void setExtensionData(void* extensionData) = 0;

	virtual const char8_t* getStatCategory() = 0;
	typedef eastl::vector<int64_t> EntityList;
	virtual void getEntityIds(EntityList& ids) = 0;
	virtual EntityId getOpponentId(EntityId myEntityId) = 0;
};

class HeadToHeadLastOpponent : public IFifaLastOpponentExtension
{
public:
	typedef eastl::hash_set<GameManager::PlayerId> ResultSet;
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;
	};
	HeadToHeadLastOpponent();
	~HeadToHeadLastOpponent();

	virtual void setExtensionData(void* extensionData);

	virtual const char8_t* getStatCategory();
	virtual void getEntityIds(eastl::vector<int64_t>& ids);
	virtual EntityId getOpponentId(EntityId myEntityId);

private:
	ExtensionData mExtensionData;
};

class ClubsLastOpponent : public IFifaLastOpponentExtension
{
public:
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;
	};
	ClubsLastOpponent();
	~ClubsLastOpponent();

	virtual void setExtensionData(void* extensionData);

	virtual const char8_t* getStatCategory();
	virtual void getEntityIds(eastl::vector<int64_t>& ids);
	virtual EntityId getOpponentId(EntityId myEntityId);
	
	void UpdateEntityFilterList(eastl::vector<int64_t> filteredIds);

private:
	ExtensionData mExtensionData;
	bool NeedToFilter(int64_t entityId);

	eastl::vector<int64_t>	mEntityFilterList;

};
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_LASTOPPONENTEXTENSIONS_H

