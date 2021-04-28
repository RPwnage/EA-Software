/*************************************************************************************************/
/*!
    \file   fifa_vproextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_VPROEXTENSIONS_H
#define BLAZE_CUSTOM_FIFA_VPROEXTENSIONS_H

/*** Include files *******************************************************************************/
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
class IFifaVproExtension
{
public:
	IFifaVproExtension() {}
	virtual ~IFifaVproExtension() {}

	virtual uint32_t getPosForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport) = 0;
	virtual Clubs::ClubId getClubForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport) = 0;
};

class ClubsVproExtension : public IFifaVproExtension
{
public:
	ClubsVproExtension();
	~ClubsVproExtension();

	virtual uint32_t getPosForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport);
	virtual Clubs::ClubId getClubForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport);
};

class OtpVproExtension : public IFifaVproExtension
{
public:
	OtpVproExtension();
	~OtpVproExtension();

	virtual uint32_t getPosForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport);
	virtual Clubs::ClubId getClubForPlayer(OSDKGameReportBase::OSDKPlayerReport* playerReport);
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_VPROEXTENSIONS_H

