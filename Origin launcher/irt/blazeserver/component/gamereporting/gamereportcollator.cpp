/*************************************************************************************************/
/*!
    \file   gamereportcollator.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "gamereportcollator.h"
#include "gamereportingslaveimpl.h"

namespace Blaze
{
namespace GameReporting
{

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  constructor
GameReportCollator::GameReportCollator(ReportCollateData& gameReport, GameReportingSlaveImpl& component)
:   mComponent(component),
    mGameReport(gameReport),
    mRefCount(0)
{
}

void GameReportCollator::onGameTypeChanged()
{
    clearCollectedGameReportMap();
}

void GameReportCollator::clearCollectedGameReportMap()
{
    getCollectedGameReportsMap().release();
}

const char8_t* GameReportCollator::getReportResultName(ReportResult reportResult)
{
    const char8_t* errReportResult = nullptr;
    switch(reportResult)
    {
    case RESULT_COLLATE_NO_REPORTS:
        {
            errReportResult = "RESULT_COLLATE_NO_REPORTS";
            break;
        }
    case RESULT_COLLATE_CONTINUE:
        {
            errReportResult = "RESULT_COLLATE_CONTINUE";
            break;
        }
    case RESULT_COLLATE_REJECTED:
        {
            errReportResult = "RESULT_COLLATE_REJECTED";
            break;
        }
    case RESULT_COLLATE_FAILED:
        {
            errReportResult = "RESULT_COLLATE_FAILED";
            break;
        }
    case RESULT_COLLATE_REJECT_DUPLICATE:
        {
            errReportResult = "RESULT_COLLATE_REJECT_DUPLICATE";
            break;
        }
    case RESULT_COLLATE_REJECT_INVALID_PLAYER:
        {
            errReportResult = "RESULT_COLLATE_REJECT_INVALID_PLAYER";
            break;
        }
    case RESULT_COLLATE_COMPLETE:
        {
            errReportResult = "RESULT_COLLATE_COMPLETE";
            break;
        }
    default:
        {
            errReportResult = "ReportResult does not exist";
            break;
        }
    }
    return errReportResult;
}

}   // namespace GameReporting
}   // namespace Blaze
