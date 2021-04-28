/*************************************************************************************************/
/*!
    \file   fifaotpreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFAOTPREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFAOTPREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameotpreportslave.h"

#include "gamereporting/fifa/tdf/fifaotpreport.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaOtpReportSlave
*/
class FifaOtpReportSlave: public GameOTPReportSlave
{
public:
    FifaOtpReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaOtpReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomOTPGameTypeName() const;
        virtual const char8_t* getCustomOTPStatsCategoryName() const;
    /* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
        //virtual bool setCustomOTPEndGameMailParam(Mail::HttpParamList* mailParamList, const char8_t* mailTemplateName);


    /*! ****************************************************************************/
    /*! \Virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerKeyscopes() {};
        virtual void updateCustomClubFreeAgentPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
    /* A custom hook for game team to preform custom process for post stats update, return true if process success */
        virtual bool processCustomUpdatedStats() {return true;};
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
		virtual void updatePlayerStats();
		virtual void updatePlayerStatsConclusionType() {};
		virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);

		virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
		virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
		virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);

		virtual void updateCustomTransformStats();
		virtual void selectCustomStats();
		virtual bool processPostStatsCommit();
		
		virtual void CustomizeGameReportForEventReporting(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds);

		void updateCleanSheets(uint32_t pos, OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaOTPReportBase::PlayerReport* fifaPlayerReport);
		void updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaOTPReportBase::PlayerReport* fifaPlayerReport);
		void updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaOTPReportBase::PlayerReport* fifaPlayerReport);

		bool didAllPlayersFinish();

private:
	FifaVpro			mFifaVpro;
	OtpVproExtension	mFifaVproExtension;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFAOTPREPORT_SLAVE_H
