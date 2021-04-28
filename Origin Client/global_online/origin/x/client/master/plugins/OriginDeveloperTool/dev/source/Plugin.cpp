//  Plugin.cpp
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#include "OriginDeveloperTool/Plugin.h"
#include "OriginDeveloperTool/OdtActivation.h"
#include "../widgets/DeveloperToolViewController.h"
#include "services/debug/DebugService.h"

#include <QCoreApplication>
#include <QEventLoop>

// For some reason, EA packages require that we define these functions,
// but I cannot figure out how to export them, so include them here.
#include "EAPackageDefines.h"

#include <cassert>

namespace Origin
{

namespace Plugins
{

namespace DeveloperTool
{

// Required for Shift submissions.
const QString fingerprint("xa37dd45ffe100bfffcc9753aabac325f07cb3fa231144fe2e33ae4783feead2b8a73ff021fac326df0ef9753ab9cdf6573ddff0312fab0b0ff39779eaff312a4f5de65892ffee33a44569bebf21f66d22e54a22347efd375981188743afd99baacc342d88a99321235798725fedcbf43252669dade32415fee89da543bf23d4ex");

static bool odtActivated = false;

int init()
{
    DeveloperToolViewController::init();
    ORIGIN_ASSERT(DeveloperToolViewController::instance());   

    return 0;
}

int run()
{
    OdtActivation::FailureReason failureReason = OdtActivation::FailureReason_None;

    // Check for ODT activation on each launch. 
    odtActivated = OdtActivation::activate(&failureReason);

    if (odtActivated)
    {
        ORIGIN_ASSERT(DeveloperToolViewController::instance());
        DeveloperToolViewController::instance()->show();
    }

    return failureReason;
}

int shutdown()
{
    ORIGIN_ASSERT(DeveloperToolViewController::instance());
    DeveloperToolViewController::instance()->close();

    QCoreApplication::sendPostedEvents(DeveloperToolViewController::instance());
    DeveloperToolViewController::destroy();
    
    return 0;
}

} // namespace OriginDeveloperTool

} // namespace Plugins

} // namespace Origin
