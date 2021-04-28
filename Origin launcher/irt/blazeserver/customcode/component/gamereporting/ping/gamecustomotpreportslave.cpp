/*************************************************************************************************/
/*!
    \file   gamecustomotpreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomotpreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomOTPReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomOTPReportSlave::GameCustomOTPReportSlave(GameReportingSlaveImpl& component) :
GameOTPReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomOTPReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomOTPReportSlave::~GameCustomOTPReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomOTPReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomOTPReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomOTPReportSlave") GameCustomOTPReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomOTPGameTypeName
    Return the game type name for OTP game used in gamereporting.cfg

    \return - the OTP game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* GameCustomOTPReportSlave::getCustomOTPGameTypeName() const
{
    return "gameType5";
}

/*************************************************************************************************/
/*! \brief GameCustomOTPReportSlave
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for OTP game
*/
/*************************************************************************************************/
const char8_t* GameCustomOTPReportSlave::getCustomOTPStatsCategoryName() const
{
    return "OTPGameStats";
}

/*************************************************************************************************/
/*! \brief updateCustomPlayerStats
    Update common stats regardless of the game result
*/
/*************************************************************************************************/
void GameCustomOTPReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    GameOTPReportBase::PlayerReport& OTPPlayerReport = static_cast<GameOTPReportBase::PlayerReport&>(*playerReport.getCustomPlayerReport());
    OTPPlayerReport.setOtpGames(1);
}

/*************************************************************************************************/
/*! \brief updateClubFreeAgentPlayerStats
    Update the club free agent players' stats (here position is assigned)
*/
/*************************************************************************************************/
void GameCustomOTPReportSlave::updateCustomClubFreeAgentPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    GameOTPReportBase::PlayerReport& OTPPlayerReport = static_cast<GameOTPReportBase::PlayerReport&>(*playerReport.getCustomPlayerReport());
    mBuilder.assignStat(STATNAME_POSITION, OTPPlayerReport.getPos());
}

/*************************************************************************************************/
/*! \brief initCustomGameParams
    Initialize game parameters that are needed for later processing
*/
/*************************************************************************************************/
void GameCustomOTPReportSlave::initCustomGameParams()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    GameOTPReportBase::GameReport& OTPGameReport = static_cast<GameOTPReportBase::GameReport&>(*OsdkReport.getGameReport().getCustomGameReport());
    OTPGameReport.setMember(mPlayerReportMapSize);
}


/*************************************************************************************************/
/*! \brief updateCustomPlayerKeyscopes
    Update game-specific custom keyscopes (here position is used as an OTP keyscope)
*/
/*************************************************************************************************/
void GameCustomOTPReportSlave::updateCustomPlayerKeyscopes()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        GameOTPReportBase::PlayerReport& OTPPlayerReport = static_cast<GameOTPReportBase::PlayerReport&>(*playerReport.getCustomPlayerReport());

        if (mUpdateClubsUtil.checkUserInClub(playerId) == false)
        {
            Stats::ScopeNameValueMap* indexMap = new Stats::ScopeNameValueMap();
            (*indexMap)[SCOPENAME_POS] = OTPPlayerReport.getPos();
            mFreeAgentPlayerKeyscopes[playerId] = indexMap;
        }
    }
}

}   // namespace GameReporting

}   // namespace Blaze

