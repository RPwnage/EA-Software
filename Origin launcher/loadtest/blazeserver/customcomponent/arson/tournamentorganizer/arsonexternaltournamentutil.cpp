/*! ************************************************************************************************/
/*!
    \file arsonexternaltournamentutil.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "framework/blaze.h"
#include "arson/tournamentorganizer/arsonexternaltournamentutilxboxone.h"
#include "arson/tournamentorganizer/arsonexternaltournamentutil.h"
#include "framework/controller/controller.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h" // for isMockPlatformEnabled in createExternalTournamentService

namespace Blaze
{
namespace Arson
{
    bool ArsonExternalTournamentUtil::isMock() const
    {
        return (getExternalSessionConfig().getUseMock() || gController->getUseMockConsoleServices());
    }

    /*! ************************************************************************************************/
    /*! \brief create the external tournament util.
    ***************************************************************************************************/
    ArsonExternalTournamentUtil* ArsonExternalTournamentUtil::createExternalTournamentService(const Blaze::Arson::ArsonTournamentOrganizerConfig& config)
    {
        // Iterate over the platforms supported, create the utils needed.
        for (ClientPlatformType curPlatform : gController->getHostedPlatforms())
        {
            switch (curPlatform)
            {
            case xbsx://tbd: support between these 2 platforms may differ. MS specs needed
            case xone:
                // If S2S APIs become available for other platforms, to support them, we'd return a list instead:
                return BLAZE_NEW ArsonExternalTournamentUtilXboxOne(curPlatform, config);
            default:
                continue;
            }
        }

        // return default util (Does nothing)
        return BLAZE_NEW ArsonExternalTournamentUtil(config);
    }

    /*! ************************************************************************************************/
    /*! \brief helper to sleep the current fiber for the specified seconds
        \param[in] logRetryNumber retry number for logging.
    ***************************************************************************************************/
    BlazeRpcError ArsonExternalTournamentUtil::waitSeconds(uint64_t seconds, const char8_t* context, size_t logRetryNumber)
    {
        TRACE_LOG(((context != nullptr)? context : "[ArsonExternalTournamentUtil].waitSeconds") << ": first party service unavailable/not-ready. Retry #" << logRetryNumber << " after sleeping " << seconds << "s.");
        TimeValue t;
        t.setSeconds(seconds);
        BlazeRpcError serr = Fiber::sleep(t, context);
        if (ERR_OK != serr)
        {
            ERR_LOG(((context != nullptr)? context : "[ArsonExternalTournamentUtil].waitSeconds") << ": failed to sleep fiber, error(" << ErrorHelp::getErrorName(serr) << ").");
        }
        return serr;
    }

}//ns Arson
}//ns Blaze

