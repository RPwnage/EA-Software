/*************************************************************************************************/
/*!
    \file   ssfreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_SSFREPORT_SLAVE_H
#define BLAZE_CUSTOM_SSFREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/osdk/gameotpreportslave.h"
#include "fifa/tdf/ssfseasonsreport.h"
#include "gamereporting/fifa/ssfgameresulthelper.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class SSFReportSlave
    This class has pure virtual functions and cannot be instantiated.
*/
class SSFReportSlave: public GameOTPReportSlave,
					public SSFGameResultHelper<SSFReportSlave>
{
public:
    SSFReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~SSFReportSlave();

    /*! ****************************************************************************/
    /*! \brief Implementation performs simple validation and if necessary modifies 
            the game report.
 
        On success report may be submitted to master for collation or direct to 
        processing for offline or trusted reports. Behavior depends on the calling RPC.

        \param report Incoming game report from submit request
        \return ERR_OK on success. GameReporting specific error on failure.
    ********************************************************************************/
    	virtual BlazeRpcError validate(GameReport& report) const;
 
    /*! ****************************************************************************/
    /*! \brief Triggered on server reconfiguration.
    ********************************************************************************/
    	virtual void reconfigure() const;

	// SSFGameResultHelper requirement implementation
	OSDKGameReportBase::OSDKReport& getOsdkReport();
	bool& getTieGame() { return mTieGame; }
	ResultSet& getTieSet() { return mTieSet; }
	ResultSet& getWinnerSet() { return mWinnerSet; }
	ResultSet& getLoserSet() { return mLoserSet; }

protected:

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameCommonReportSlave class.
    ********************************************************************************/
        virtual void initGameParams();
        virtual void determineGameResult();
        virtual void updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updatePlayerKeyscopes();
        virtual void updatePlayerStats();
        virtual void updatePlayerStatsConclusionType() {};
        virtual bool processUpdatedStats();
        virtual void updateNotificationReport() {};
        virtual void sendEndGameMail(GameManager::PlayerIdList& playerIds) {};

    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameOTPReportSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomOTPGameTypeName() const;
        virtual const char8_t* getCustomOTPStatsCategoryName() const;

    /*! ****************************************************************************/
    /*! \Pure virtual functions from FifaCReportSalve, which needs to be implemented by custom class.
    ********************************************************************************/
        virtual const char8_t* getCustomSquadGameTypeName() const = 0;
 
    /*! ****************************************************************************/
    /*! \Virtual functions provided by SSFReportSlave that can be overwritten with custom functionalities
    ********************************************************************************/
        virtual void initCustomGameParams() {};
        virtual void updateCustomPlayerKeyscopes() {};
        virtual void updateCustomSquadKeyscopes() {};
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport) {};
    /* A custom hook for game team to preform custom process for post stats update, ie. awards based on win streak*/
        virtual bool processCustomUpdatedStats() {return true;};
    /* A custom hook for game team to update custom stats per team */
		virtual void updateCustomSquadStatsPerSquad(int64_t teamEnttiyId, SSFSeasonsReportBase::SSFTeamReport& teamReport) {};
    /* A 'catch-all' custom hook for game team to update custom stats */
        virtual void updateCustomSquadStats(OSDKGameReportBase::OSDKReport& OsdkReport) {};
        virtual void updateCustomTransformStats() {};

    // Useful when overriding process(). Game teams may wish to call into these directly. 
    void updateSquadKeyscopes();
    bool updateSquadStats();
	void updateSquadResults();

	using GameOTPReportSlave::transformStats;
    BlazeRpcError transformStats();

private:
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_SSFREPORT_SLAVE_H
