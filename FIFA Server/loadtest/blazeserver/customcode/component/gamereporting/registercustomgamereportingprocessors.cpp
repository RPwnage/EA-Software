/*************************************************************************************************/
/*!
    \file   registercustomgamereportprocessors.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "gamereportprocessor.h"
#include "gamereportingslaveimpl.h"

// FIFA SPECIFIC CODE START
#include "fifa/fifah2hreportslave.h"
#include "fifa/fifah2hcupsreportslave.h"
#include "fifa/fifah2hfriendliesreportslave.h"
#include "fifa/fifah2horganizedtournamentreportslave.h"
#include "fifa/fifaclubreportslave.h"
#include "fifa/fifaclubcupreportslave.h"
#include "fifa/fifaotpreportslave.h"
#include "fifa/fifacoopseasonsreportslave.h"
#include "fifa/fifacoopseasonscupreportslave.h"
#include "fifa/ssfseasonsreportslave.h"
#include "fifa/ssfsoloreportslave.h"
#include "fifa/fifasoloreportslave.h"
#include "fifa/futh2hreportslave.h"
#include "fifa/futsoloreportslave.h"
#include "fifa/futotpreportslave.h"
#include "fifa/futh2horganizedtournamentreportslave.h"

//#include "ping/gamecustomh2hreportslave.h"
//#include "ping/gamecustomotpreportslave.h"
//#include "ping/gamecustomclubreportslave.h"
//#include "ping/gamecustomtournamentreportslave.h"
//#include "ping/gamecustomclubplayoffreportslave.h"
//#include "ping/gamecustomh2hplayoffreportslave.h"
//#include "ping/gamecustomsoloreportslave.h"
//#include "ping/gamecustomclubchampreportslave.h"
//#include "ping/gamecustomclubintrah2hreportslave.h"

#include "osdk/gamecoopreportslave.h"
#include "osdk/gamenplayerreportslave.h"
// FIFA SPECIFIC CODE END


namespace Blaze
{
namespace GameReporting
{

void GameReportingSlaveImpl::registerCustomGameReportProcessors()
{

    // game team should register their own GameReportSlave here
    // FIFA SPECIFIC CODE START
    registerGameReportProcessor("gameH2H", &FifaH2HReportSlave::create);
    registerGameReportProcessor("gameH2HCups", &FifaH2HCupsReportSlave::create);
    registerGameReportProcessor("gameH2HFriendlies", &FifaH2HFriendliesReportSlave::create);
    registerGameReportProcessor("gameH2HOrganizedTournament", &FifaH2HOrganizedTournamentReportSlave::create);
    registerGameReportProcessor("gameOTP", &FifaOtpReportSlave::create);
    registerGameReportProcessor("gameClub",&FIFA::FifaClubReportSlave::create);
//    registerGameReportProcessor("gameTournament",&GameCustomTournamentReportSlave::create);
    registerGameReportProcessor("gameClubCup",&FIFA::FifaClubCupReportSlave::create);
//    registerGameReportProcessor("gameH2HPlayoff",&GameCustomH2HPlayoffReportSlave::create);
    registerGameReportProcessor("gameSolo", &FifaSoloReportSlave::create);
//    registerGameReportProcessor("gameClubChamp",&GameCustomClubChampReportSlave::create);
//    registerGameReportProcessor("gameClubIntraH2H",&GameCustomClubIntraH2HReportSlave::create);
    registerGameReportProcessor("gameFutH2H",&FutH2HReportSlave::create);
    registerGameReportProcessor("gameFutSolo",&FutSoloReportSlave::create);
	registerGameReportProcessor("gameFutOTP", &FutOtpReportSlave::create);
	registerGameReportProcessor("gameFutH2HOrganizedTournament", &FutH2HOrganizedTournamentReportSlave::create);
    registerGameReportProcessor("gameFifaCoopSeason",&FIFA::FifaCoopSeasonsReportSlave::create);
    registerGameReportProcessor("gameFifaCoopCup",&FIFA::FifaCoopSeasonsCupReportSlave::create);
    registerGameReportProcessor("gameSSFSeason", &FIFA::SSFSeasonsReportSlave::create);
    registerGameReportProcessor("gameSSFSolo", &SSFSoloReportSlave::create);
	registerGameReportProcessor("gameSSFMini", &SSFMinigameReportSlave::create);
    registerGameReportProcessor("gameCoop", &GameCoopReportSlave::create);
    registerGameReportProcessor("gameNPlayer", &GameNPlayerReportSlave::create);  
    // FIFA SPECIFIC CODE END
}

}   // namespace GameReporting
}   // namespace Blaze
