/*************************************************************************************************/
/*!
    \file   fifashieldgamereporthelper.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifashieldgamereporthelper.h"

#include "heatshield/heatshieldslaveimpl.h"
#include "heatshield/tdf/heatshieldtypes.h"

#include "framework/usersessions/usersessionmanager.h"
namespace Blaze
{
namespace Gamereporting
{
namespace Shield
{

void doShieldClientChallenge(const char8_t* gameReportName)
{
	Blaze::HeatShield::HeatShieldSlave* heatShieldSlave = static_cast<Blaze::HeatShield::HeatShieldSlave*>(
		gController->getComponent(Blaze::HeatShield::HeatShieldSlave::COMPONENT_ID, false));

	if (heatShieldSlave != nullptr)
	{
		UserSession::SuperUserPermissionAutoPtr autoPtr(true);

		Blaze::HeatShield::ClientChallengeRequest clientChallengeReq;
		Blaze::HeatShield::HeatShieldResponse clientChallengeResp;

		clientChallengeReq.setGameTypeName(gameReportName);
		clientChallengeReq.setClientChallengeType(Blaze::HeatShield::CLIENT_CHALLENGE_MATCHEND);

		// Fire and forget
		heatShieldSlave->clientChallenge(clientChallengeReq, clientChallengeResp);
	}
}

} // namespace Shield
} // namespace GameReporting
} // namespace Blaze