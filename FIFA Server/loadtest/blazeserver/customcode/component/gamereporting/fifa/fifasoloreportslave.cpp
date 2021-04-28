/*************************************************************************************************/
/*!
    \file   fifasoloreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifasoloreportslave.h"

#include "fifa/tdf/fifasoloreport.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FifaSoloReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaSoloReportSlave::FifaSoloReportSlave(GameReportingSlaveImpl& component) :
GameSoloReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief FifaSoloReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaSoloReportSlave::~FifaSoloReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaSoloReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaSoloReportSlave") FifaSoloReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomSoloGameTypeName
    Return the game type name for solo game used in gamereporting.cfg

    \return - the Solo game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FifaSoloReportSlave::getCustomSoloGameTypeName() const
{
    return "gameType7";
}

/*************************************************************************************************/
/*! \brief getCustomSoloStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for Solo game
*/
/*************************************************************************************************/
const char8_t* FifaSoloReportSlave::getCustomSoloStatsCategoryName() const
{
    return "SoloGameStats";
}

/*************************************************************************************************/
/*! \brief setCustomEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FifaSoloReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
	return false;
}
*/
/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FifaSoloReportSlave::updateCustomNotificationReport()
{
	// Obtain the custom data for report notification
	OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	Fifa::SoloNotificationCustomGameData *gameCustomData = BLAZE_NEW Fifa::SoloNotificationCustomGameData();

	// Set Report Id for FUT.
	gameCustomData->setGameReportingId(mProcessedReport->getGameReport().getGameReportingId());

	// Set the gameCustomData to the OsdkReportNotification
	OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

}   // namespace GameReporting

}   // namespace Blaze

