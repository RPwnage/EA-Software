/*************************************************************************************************/
/*!
    \file   fifah2hfriendliesreportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFAH2HFRIENDLIESREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFAH2HFRIENDLIESREPORT_SLAVE_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifacustom.h"
#include "gamereporting/fifa/fifa_friendlies/fifa_friendlies.h"
#include "gamereporting/fifa/fifa_friendlies/fifa_friendliesextensions.h"

#include "gamereporting/fifa/fifah2hbasereportslave.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaH2HFriendliesReportSlave
*/
class FifaH2HFriendliesReportSlave: public FifaH2HBaseReportSlave
{
public:
    FifaH2HFriendliesReportSlave(GameReportingSlaveImpl& component);
    static GameReportProcessor* create(GameReportingSlaveImpl& component);

    virtual ~FifaH2HFriendliesReportSlave();

protected:
    /*! ****************************************************************************/
    /*! \Pure virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual const char8_t* getCustomH2HGameTypeName() const;
        virtual const char8_t* getCustomH2HStatsCategoryName() const;
		virtual const uint16_t getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const;

    /*! ****************************************************************************/
    /*! \Virtual functions from GameH2HReportsSlave class.
    ********************************************************************************/
        virtual void updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport);
        virtual void updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport);
        virtual void updateCustomNotificationReport(const char8_t* statsCategory);
    /* A custom hook for Game team to perform post stats update processing */
        virtual bool processCustomUpdatedStats() { return true;};
    /* A custom hook for Game team to add additional player keyscopes needed for stats update */
        virtual void updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};
	/* A custom hook  to set win/draw/loose in gamereport */	
		virtual void updatePlayerStats();  
	/* A custom hook for Game team to set parameters for the end game mail, return true if sending a mail */
       // virtual bool setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName);
    /* A custom hook for Game team to customize lobby skill info */
        virtual void setCustomLobbySkillInfo(GameCommonLobbySkill::LobbySkillInfo* skillInfo) {};
    /* A custom hook for Game team to aggregate additional player stats */
        virtual void aggregateCustomPlayerStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap) {};

		virtual void selectCustomStats();
		virtual void updateCustomTransformStats(const char8_t* statsCategory);

private:
	virtual void						readNrmlXmlData(const char8_t* statsCategory, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator & playerIter, StatUpdateList& statUpdates, Stats::StatDescs& getStatsDescsResponse);
	virtual void							customFillNrmlXmlData(uint64_t playerId, eastl::string &buffer) {}
	virtual const eastl::string				getXmsAssetName(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter);
	virtual	XmsRequestMetadata				getXmsRequestMetadata(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter);
	virtual const char8_t*					getXmsDataRootElementName();
	virtual bool							sendXmsDataForPlayer(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator& playerIter);

	FifaFriendlies					mFriendlies;
	HeadtoHeadFriendliesExtension	mFriendliesExtension;

	//key is blaze id of player, value is NucleusId of friend!
	typedef eastl::map<GameManager::PlayerId,AccountId> FriendNucleusIdMap;
	FriendNucleusIdMap mNucleausIdMap;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFAH2HFRIENDLIESREPORT_SLAVE_H

