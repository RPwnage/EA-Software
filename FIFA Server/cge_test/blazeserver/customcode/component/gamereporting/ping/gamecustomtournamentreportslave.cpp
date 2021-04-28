/*************************************************************************************************/
/*!
    \file   gamecustomtournamentreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomtournamentreportslave.h"
#include "ping/tdf/gametournamentreport.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomTournamentReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomTournamentReportSlave::GameCustomTournamentReportSlave(GameReportingSlaveImpl& component) :
GameTournamentReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomTournamentReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomTournamentReportSlave::~GameCustomTournamentReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomTournamentReportSlave::Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomTournamentReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomTournamentReportSlave") GameCustomTournamentReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomTournamentGameTypeName
    Return the game type name for tournament game used in gamereporting.cfg

    \return - the tournament game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* GameCustomTournamentReportSlave::getCustomTournamentGameTypeName() const
{
    return "gameType10";
}

/*************************************************************************************************/
/*! \brief getCustomTournamentStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for tournament game
*/
/*************************************************************************************************/
const char8_t* GameCustomTournamentReportSlave::getCustomTournamentStatsCategoryName() const
{
    return "TournamentTeamGameStats";
}

/*************************************************************************************************/
/*! \brief getCustomTournamentId
    Fill in the tournament Id parameter and return true if it's a valid tournament Id

    \return - true if the tournament Id extracted from game report is valid
*/
/*************************************************************************************************/
bool GameCustomTournamentReportSlave::getCustomTournamentId(OSDKGameReportBase::OSDKGameReport& osdkGameReport, OSDKTournaments::TournamentId& tournamentId)
{
    GameTournamentReportBase::GameReport& gameReport = static_cast<GameTournamentReportBase::GameReport&>(*osdkGameReport.getCustomGameReport());
    tournamentId = gameReport.getTournamentId();

    if(tournamentId > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/*************************************************************************************************/
/*! \brief updateCustomTournamentTeamStats
    Update the tournament custom team stats that depends on the game custom game report
*/
/*************************************************************************************************/
void GameCustomTournamentReportSlave::updateCustomTournamentTeamStats(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap)
{
    int32_t iAttribute[MAX_TOURNAMENT_TEAM];
    int32_t iScore[MAX_TOURNAMENT_TEAM];

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(int32_t index = 0; playerIter != playerEnd; ++playerIter, ++index)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        GameTournamentReportBase::PlayerReport& tournamentPlayerReport = static_cast<GameTournamentReportBase::PlayerReport&>(*playerReport.getCustomPlayerReport());

        iAttribute[index] = tournamentPlayerReport.getMetaAttribute();  // SCOPENAME_METAATTRIBUTE
        iScore[index] = playerReport.getScore();
    }

    if (iAttribute[0] != 0 && iAttribute[1] != 0)
    {
        if (iAttribute[0] == iAttribute[1])
        {
            mBuilder.startStatRow(getCustomTournamentStatsCategoryName(), iAttribute[0], 0);
            mBuilder.assignStat(STATNAME_ATTRIBUTE, iAttribute[0]);
            mBuilder.incrementStat(STATNAME_POINTS, iScore[0] + iScore[1]);
            mBuilder.completeStatRow();
        }
        else
        {
            for (int32_t index = 0; index < MAX_TOURNAMENT_TEAM; index++)
            {
                mBuilder.startStatRow(getCustomTournamentStatsCategoryName(), iAttribute[index], 0);
                mBuilder.assignStat(STATNAME_ATTRIBUTE, iAttribute[index]);
                mBuilder.incrementStat(STATNAME_POINTS, iScore[index]);
                mBuilder.completeStatRow();
            }
        }
    }
}

/*************************************************************************************************/
/*! \brief processCustomUpdatedStats
    A custom hook to allow game team to process custom stats update and return false if process 
        failed and should stop further game report processing

    \return - true if the post-game custom processing is performed successfully
*/
/*************************************************************************************************/
bool GameCustomTournamentReportSlave::processCustomUpdatedStats() 
{
    return true;
}

/*************************************************************************************************/
/*! \brief updateCustomPlayerKeyscopes
    A custom hook for Game team to add additional player keyscopes needed for stats update
*/
/*************************************************************************************************/
void GameCustomTournamentReportSlave::updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap)
{
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = osdkPlayerReportMap.begin();
    playerEnd = osdkPlayerReportMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        GameTournamentReportBase::PlayerReport& tournamentPlayerReport = static_cast<GameTournamentReportBase::PlayerReport&>(*playerReport.getCustomPlayerReport());

        OSDKTournaments::TournamentMemberData memberData;

        if (mTournamentsUtil.getMemberStatus(playerId, memberData) == DB_ERR_OK)
        {
            uint32_t metaAttribute = 0;
            if (NULL != memberData.getTournAttribute())
            {
                //char8_t* strNonInt = 
                blaze_str2int(memberData.getTournAttribute(), &metaAttribute);
            }
            tournamentPlayerReport.setMetaAttribute(metaAttribute);
        }
    }
}

}   // namespace GameReporting

}   // namespace Blaze

