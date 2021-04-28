/*************************************************************************************************/
/*!
    \file   fifa_vproextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "fifa_vproextensions.h"
#include "customcode/component/gamereporting/fifa/tdf/fifaclubreport.h"
#include "customcode/component/gamereporting/fifa/tdf/fifaotpreport.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{


ClubsVproExtension::ClubsVproExtension()
{
}

ClubsVproExtension::~ClubsVproExtension()
{
}

uint32_t ClubsVproExtension::getPosForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport)
{
	OSDKClubGameReportBase::OSDKClubPlayerReport* clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport*>(playerReport->getCustomPlayerReport());

	return clubPlayerReport->getPos();
}

Clubs::ClubId ClubsVproExtension::getClubForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport)
{
	OSDKClubGameReportBase::OSDKClubPlayerReport* clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport*>(playerReport->getCustomPlayerReport());

	return clubPlayerReport->getClubId();

}

OtpVproExtension::OtpVproExtension()
{
}

OtpVproExtension::~OtpVproExtension()
{
}

uint32_t OtpVproExtension::getPosForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport)
{
	FifaOTPReportBase::PlayerReport* fifaPlayerReport = static_cast<FifaOTPReportBase::PlayerReport*>(playerReport->getCustomPlayerReport());

	return fifaPlayerReport->getPos();
}

Clubs::ClubId OtpVproExtension::getClubForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport)
{
	//no valid club id for OTP
	return 0;
}

}   // namespace GameReporting
}   // namespace Blaze
