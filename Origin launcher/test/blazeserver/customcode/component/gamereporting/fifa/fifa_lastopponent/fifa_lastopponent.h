/*************************************************************************************************/
/*!
    \file   fifa_lastopponent.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_LASTOPPONENT_H
#define BLAZE_CUSTOM_FIFA_LASTOPPONENT_H

/*** Include files *******************************************************************************/
#include "fifa_lastopponentextensions.h"
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
    class FifaLastOpponent
*/
class FifaLastOpponent
{
public:
	FifaLastOpponent() {}
	~FifaLastOpponent() {}

	void initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport);
	void setExtension(IFifaLastOpponentExtension* lastOpponentExtension);

	void selectLastOpponentStats();
	void transformLastOpponentStats();

private:
	typedef eastl::list<Stats::UpdateRowKey> UpdateRowKeyList;

	void readLastOpponentStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap);
	void writeLastOpponentStats(StatValueUtil::StatUpdateMap& statUpdateMap);

	void updateLastOpponent(Stats::UpdateRowKey spLastOppKey, StatValueUtil::StatUpdateMap* statUpdateMap);
	
	static const int32_t NUM_LAST_MATCHES = 6;

	Stats::UpdateStatsRequestBuilder*	mBuilder;
    Stats::UpdateStatsHelper*           mUpdateStatsHelper;
	ProcessedGameReport*				mProcessedReport;
	
	IFifaLastOpponentExtension*			mLastOpponentExtension;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_LASTOPPONENT_H

