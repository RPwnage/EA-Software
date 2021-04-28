/*************************************************************************************************/
/*!
    \file   fut_lastopponentextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

#include "gamereporting/fifa/tdf/futotpreport.h"
#include "gamereporting/fifa/fut_lastopponent/fut_lastopponentextensions.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

/*-------------------------------------------
OTP Play
---------------------------------------------*/

FutOtpLastOpponent::FutOtpLastOpponent()
{
}

FutOtpLastOpponent::~FutOtpLastOpponent()
{
}

void FutOtpLastOpponent::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

const char8_t* FutOtpLastOpponent::getStatCategory()
{
	return "SPOverallPlayerStats";
}

void FutOtpLastOpponent::getEntityIds(EntityList& ids)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		ids.push_back(playerIter->first);
	}
}

EntityId FutOtpLastOpponent::getOpponentId(EntityId entityId)
{
	int64_t entityTeamId = -1;
	EntityId opponentId = -1;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
	
	// Getting the team from the CustomPlayerReport because it appears to be inaccurate in OSDKPlayerReport
	const OSDKGameReportBase::OSDKPlayerReport* entityPlayerReport = OsdkPlayerReportsMap[entityId];
	if (entityPlayerReport)
	{
		const FutOTPReportBase::FutOTPPlayerReport* entityOtpPlayerReport = static_cast<const FutOTPReportBase::FutOTPPlayerReport*>(entityPlayerReport->getCustomPlayerReport());
		if (entityOtpPlayerReport && entityOtpPlayerReport->getIsCaptain())
		{
			entityTeamId = entityOtpPlayerReport->getTeamEntityId();
		}
	}

	if (entityTeamId != -1)
	{
		for (const auto& playerIter : OsdkPlayerReportsMap)
		{
			const OSDKGameReportBase::OSDKPlayerReport& playerReport = *(playerIter.second);
			const FutOTPReportBase::FutOTPPlayerReport& otpPlayerReport = static_cast<const FutOTPReportBase::FutOTPPlayerReport&>(*playerReport.getCustomPlayerReport());
			if (otpPlayerReport.getTeamEntityId() != entityTeamId)
			{
				if (otpPlayerReport.getIsCaptain())
				{
					opponentId = playerIter.first;
					break;
				}
			}
		}
	}
	return opponentId;
}

}   // namespace GameReporting
}   // namespace Blaze
