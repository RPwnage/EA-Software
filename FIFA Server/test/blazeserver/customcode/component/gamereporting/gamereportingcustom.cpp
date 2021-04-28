/*************************************************************************************************/
/*!
    \file   gamereportingcustom.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifdef TARGET_gamereporting

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

// FIFA SPECIFIC CODE START
#include "gamereportprocessor.h"
// FIFA SPECIFIC CODE END
#include "gamereportingslaveimpl.h"
#include "integratedsample/tdf/integratedsample.h"
#include "sample/tdf/samplecustomreport.h"

#ifdef TARGET_arson
#include "arson/tdf/arsonclub.h"
#include "arson/tdf/arsonclubgameks.h"
#include "arson/tdf/arsonctf.h"
#include "arson/tdf/arsonctfcommon.h"
#include "arson/tdf/arsonctfderived.h"
#include "arson/tdf/arsonctfcustom.h"
#include "arson/tdf/arsonctfendgame.h"
#include "arson/tdf/arsonctfgsa.h"
#include "arson/tdf/arsonctfgsacommon.h"
#include "arson/tdf/arsonctfgsaderived.h"
#include "arson/tdf/arsonctfks.h"
#include "arson/tdf/arsonctfkscommon.h"
#include "arson/tdf/arsonctfksderived.h"
#include "arson/tdf/arsonctfmidgame.h"
#include "arson/tdf/arsongamehistoryreport.h"
#include "arson/tdf/arsongamehistoryreportbasic.h"
#include "arson/tdf/arsonleague.h"
#include "arson/tdf/arsonleaguegameks.h"
#include "arson/tdf/arsonmultikeyscopes.h"
#include "arson/tdf/arsonmultistatupdates.h"
#include "arson/tdf/arsonmultistatupdatesks.h"
#include "arson/tdf/arsonctfdimstats.h"
#endif

namespace Blaze
{
namespace GameReporting
{

//  add custom TDF registration methods here
void registerCustomReportTdfs()
{
    IntegratedSample::integratedsampleRegisterTdfs();
    SampleBase::samplecustomreportRegisterTdfs();
#ifdef TARGET_arson
    ArsonClub::arsonclubRegisterTdfs();
    ArsonClubGameKeyscopes_NonDerived::arsonclubgameksRegisterTdfs();
    ArsonCTF_Common::arsonctfcommonRegisterTdfs();
    ArsonCTF_Derived::arsonctfderivedRegisterTdfs();
    ArsonCTF_NonDerived::arsonctfRegisterTdfs();
    ArsonCTF_Custom::arsonctfcustomRegisterTdfs();
    ArsonCTF_EndGame::arsonctfendgameRegisterTdfs();
    ArsonCTF_GSA_Common::arsonctfgsacommonRegisterTdfs();
    ArsonCTF_GSA_Derived::arsonctfgsaderivedRegisterTdfs();
    ArsonCTF_GSA_NonDerived::arsonctfgsaRegisterTdfs();
    ArsonCTF_KS_Common::arsonctfkscommonRegisterTdfs();
    ArsonCTF_KS_Derived::arsonctfksderivedRegisterTdfs();
    ArsonCTF_KS_NonDerived::arsonctfksRegisterTdfs();
    ArsonCTF_MidGame::arsonctfmidgameRegisterTdfs();
    GameHistoryBasic::arsongamehistoryreportbasicRegisterTdfs();
    GameHistoryClubs_NonDerived::arsongamehistoryreportRegisterTdfs();
    ArsonLeague::arsonleagueRegisterTdfs();
    ArsonLeagueGameKeyscopes::arsonleaguegameksRegisterTdfs();
    ArsonMultiKeyscopes::arsonmultikeyscopesRegisterTdfs();
    ArsonMultiStatUpdates::arsonmultistatupdatesRegisterTdfs();
    ArsonMultiStatUpdatesKeyscopes::arsonmultistatupdatesksRegisterTdfs();
    ArsonCTF_DimensionalStats::arsonctfdimstatsRegisterTdfs();
#endif
}

//  add custom TDF de-registration methods here
void deregisterCustomReportTdfs()
{
}

}   // namespace GameReporting
}   // namespace Blaze

#endif // TARGET_gamereporting