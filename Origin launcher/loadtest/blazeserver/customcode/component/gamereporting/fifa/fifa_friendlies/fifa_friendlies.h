/*************************************************************************************************/
/*!
    \file   fifa_friendlies.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_FRIENDLIES_H
#define BLAZE_CUSTOM_FIFA_FRIENDLIES_H

/*** Include files *******************************************************************************/
#include "fifa_friendliesextensions.h"
#include "gamereporting/fifa/fifastatsvalueutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace Stats
{
	class UpdateStatsRequestBuilder;
}

namespace GameReporting
{
	class ProcessedGameReport;

/*!
    class FifaFriendlies
*/
class FifaFriendlies
{
public:
	FifaFriendlies() {}
	~FifaFriendlies() {}

	void initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport);
	void setExtension(IFifaFriendliesExtension* seasonalPlayExtension);

	void selectFriendliesStats();
	void transformFriendliesStats();

private:
	typedef eastl::list<Stats::UpdateRowKey> UpdateRowKeyList;

	void readFriendliesStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap);
	void writeFriendliesStats(StatValueUtil::StatUpdateMap& statUpdateMap);

	int64_t update_possession(int64_t currentPossession, int64_t newPossession, int64_t numGames,int side);

	void updateFriendlySeasonStats(UpdateRowKeyList keys, StatValueUtil::StatUpdateMap *statUpdateMap);
	
	Stats::UpdateStatsRequestBuilder*	mBuilder;
    Stats::UpdateStatsHelper*           mUpdateStatsHelper;
	ProcessedGameReport*				mProcessedReport;
	
	IFifaFriendliesExtension*			mFriendliesExtension;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_FRIENDLIES_H

