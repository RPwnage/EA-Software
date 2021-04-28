/*************************************************************************************************/
/*!
    \file   osdkgamereportmaster.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_CUSTOM_OSDKGAMREPORT_MASTER_H
#define BLAZE_CUSTOM_OSDKGAMREPORT_MASTER_H

#include "gamereporting/basicgamereportcollator.h"
#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereporting/tdf/gamereporting.h"
#include "gamereporting/tdf/gamereporting_server.h"
#include "gamereportcollator.h"
#include "util/collectorutil.h"

namespace Blaze
{
namespace GameReporting
{

class OsdkGameReportMaster
    : public BasicGameReportCollator
{
public:
    OsdkGameReportMaster(ReportCollateData& gameReport, GameReportingSlaveImpl& component)
        : BasicGameReportCollator(gameReport, component),
        mCachedGameInfo(gameReport.getGameInfo()) 
    {
    }

    static GameReportCollator* create(ReportCollateData& gameReport, GameReportingSlaveImpl& component)
    {
        return BLAZE_NEW_NAMED("OsdkGameReportMaster") OsdkGameReportMaster(gameReport, component);
    }

    //! We override these methods only to provide Arena-specific game reporting
    virtual CollatedGameReport& finalizeCollatedGameReport(ReportType collatedReportType);
    virtual ReportResult gameFinished(const GameInfo& gameInfo);
    virtual ReportResult timeout() const;

protected:
    GameInfo mCachedGameInfo;

private:
    void submitDnfReport() const;
};

} // end namespace GameReporting
} // end namespace Blaze

#endif // defined(BLAZE_CUSTOM_OSDKGAMREPORT_MASTER_H)
