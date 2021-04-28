/*************************************************************************************************/
/*!
    \file   fifa_friendliesextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_FRIENDLIESEXTENSIONS_H
#define BLAZE_CUSTOM_FIFA_FRIENDLIESEXTENSIONS_H

/*** Include files *******************************************************************************/
#include "fifa_friendliesdefines.h"
#include "gamereporting/osdk/tdf/gameosdkreport.h"
#include "component/stats/tdf/stats.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

/*!
    class FifaSeasonalPlayExtension
*/
class IFifaFriendliesExtension
{
public:
	IFifaFriendliesExtension() {}
	virtual ~IFifaFriendliesExtension() {}

	virtual void setExtensionData(void* extensionData) = 0;

	struct FriendyPair 
	{
		const char* category;
		int64_t entities[SIDE_NUM];
	};
	typedef eastl::vector<FriendyPair> FriendlyList;
	virtual void getCategoryInfo(FriendlyList& friendlyList) = 0;
	virtual void generateAttributeMap(EntityId id, Collections::AttributeMap& map) = 0;
};

class HeadtoHeadFriendliesExtension : public IFifaFriendliesExtension
{
public:
	typedef eastl::hash_set<GameManager::PlayerId> ResultSet;
	struct ExtensionData
	{
		ProcessedGameReport*	mProcessedReport;
		
		ResultSet*				mWinnerSet;
		ResultSet*				mLoserSet;
		ResultSet*				mTieSet;
	};
	HeadtoHeadFriendliesExtension();
	~HeadtoHeadFriendliesExtension();

	virtual void setExtensionData(void* extensionData);

	virtual void getCategoryInfo(FriendlyList& friendlyList);
	virtual void generateAttributeMap(EntityId id, Collections::AttributeMap& map);

private:
	OSDKGameReportBase::OSDKPlayerReport* getOsdkPlayerReport(EntityId entityId);
	char* convertToString(int64_t value, eastl::string& stringResult);

	ExtensionData mExtensionData;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_FRIENDLIESEXTENSIONS_H

