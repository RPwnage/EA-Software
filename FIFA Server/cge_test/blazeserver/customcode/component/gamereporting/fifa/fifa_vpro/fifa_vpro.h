/*************************************************************************************************/
/*!
    \file   fifa_vpro.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_VPRO_H
#define BLAZE_CUSTOM_FIFA_VPRO_H

/*** Include files *******************************************************************************/
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
	class IFifaVproExtension;

/*!
    class FifaVpro
*/
class FifaVpro
{
public:

	enum VproCompetitions
	{
		VPRO_COMPETITION_INVALID = -1,
		VPRO_COMPETITION_ALL = 0,
		VPRO_COMPETITION_CLUBS, 
		VPRO_COMPETITION_CLUBSCUP,
		VPRO_COMPETITION_DROPIN,
		VPRO_COMPETITION_NUM
	};

	enum VproRole
	{
		VPRO_POS_INVALID = -1,
		VPRO_POS_GK = 1,
		VPRO_POS_DEF,
		VPRO_POS_MID,
		VPRO_POS_FWD,
		VPRO_POS_NUM = VPRO_POS_FWD
	};

	enum VproAttribute
	{
		VPRO_ATTRIBUTE_MAX = 34
	};

	FifaVpro() {}
	~FifaVpro() {}

	void initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport);
	void setExtension(IFifaVproExtension* vproExtension);

	enum KeyScopeType
	{
		KEYSCOPE_NONE,
		KEYSCOPE_POS,
		KEYSCOPE_CLUBID,
		KEYSCOPE_ATTRIBUTETYPE,
		KEYSCOPE_MAX
	};
	struct CategoryInfo 
	{
		const char* statCategory;
		KeyScopeType keyScopeType;
	};
	typedef eastl::vector<CategoryInfo> CategoryInfoVector;
	void updateVproStats(CategoryInfoVector& categoryInfoVector);

	void updateVproGameStats(CategoryInfoVector& categoryInfoVector);

	void updatePlayerGrowth();

	static int CountVProBits(const char* sourceStr);
	static bool CompareBitCount(const char* sourceStr, const char* targetStr);
	static bool CompareStatValues(const char* sourceStr, const char* targetStr);

	typedef bool (*ValidateFunction)(const char* src, const char* target);
	
	bool updateStatIfPresent(Stats::UpdateRowKey* key, Collections::AttributeMap& privateStringMap, const char *toStat, const char *fromStat, ValidateFunction validateFunc = NULL);
	void selectStats(CategoryInfoVector& categoryInfoVector);


private:
	Stats::UpdateStatsRequestBuilder*	mBuilder;
 	ProcessedGameReport*				mProcessedReport;
	IFifaVproExtension*					mVproExtension;
	Stats::UpdateStatsHelper*           mUpdateStatsHelper;
	
	static const char*					mXmlRoleTagMapping[VPRO_POS_NUM];

	void sendDataToXmsFiberCall(uint64_t nucleusId, const char* vproName, eastl::string vProStatsDataXml);
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_VPRO_H

