/*************************************************************************************************/
/*!
    \file   ssfminigamereportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CUSTOM_SSFMINIGAMEREPORT_SLAVE_H
#define BLAZE_CUSTOM_SSFMINIGAMEREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "fifa/tdf/ssfminigamesreport.h"

#include "gamereporting/osdk/gameotpreportslave.h"

/*** Defines / Macros / Constants / Typedefs ***********************************************************/
namespace Blaze
{
namespace GameReporting
{
class SSFMinigameReportSlave : public GameOTPReportSlave
{
public:
    SSFMinigameReportSlave(GameReportingSlaveImpl &component);
	~SSFMinigameReportSlave();

    static GameReportProcessor *create(GameReportingSlaveImpl &component);
	
    /*! ****************************************************************************/
    /*! \brief Implementation performs simple validation and if necessary modifies 
            the game report.
 
        On success report may be submitted to master for collation or direct to 
        processing for offline or trusted reports. Behavior depends on the calling RPC.

        \param report Incoming game report from submit request
        \return ERR_OK on success. GameReporting specific error on failure.
    ********************************************************************************/
	BlazeRpcError validate(GameReport& report) const override;
 
protected:
	/*! ****************************************************************************/
	/*! \Pure virtual functions from GameOTPReportSlave class.
	********************************************************************************/
	const char8_t *getCustomOTPGameTypeName		() const override;
	const char8_t *getCustomOTPStatsCategoryName() const override;

	void			determineGameResult				()																override;
	BlazeRpcError	process							(ProcessedGameReport &, GameManager::PlayerIdList &)			override;
	bool			processUpdatedStats				()																override;
	void			updateCommonStats				(GameManager::PlayerId, OSDKGameReportBase::OSDKPlayerReport &)	override;
	void			updateCustomPlayerKeyscopes		()																override;
	void			updateGameModeLossPlayerStats	(GameManager::PlayerId, OSDKGameReportBase::OSDKPlayerReport &)	override;
	void			updateGameModeTiePlayerStats	(GameManager::PlayerId, OSDKGameReportBase::OSDKPlayerReport &)	override;
	void			updateGameModeWinPlayerStats	(GameManager::PlayerId, OSDKGameReportBase::OSDKPlayerReport &)	override;
	void			updateNotificationReport		()																override;
	void			updatePlayerStats				()																override;

	OSDKGameReportBase::OSDKReport &						getOsdkReport			();
	SSFMinigamesReportBase::ParticipantReportMap &			getParticipantReports	();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap &	getPlayerReports		();

	void updatePlayerStatsConclusionType() override {};
 
private:
	using GameOTPReportSlave::transformStats;
	BlazeRpcError transformStats();

	int16_t mWinScore;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_SSFREPORT_SLAVE_H
