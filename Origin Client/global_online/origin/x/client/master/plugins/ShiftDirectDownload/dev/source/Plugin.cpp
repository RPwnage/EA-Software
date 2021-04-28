//  Plugin.cpp
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#include "ShiftDirectDownload/DcmtServiceClient.h"
#include "ShiftDirectDownload/Plugin.h"
#include "ShiftDirectDownload/DcmtDownloadManager.h"
#include "../widgets/ShiftLoginViewController.h"
#include "../widgets/ShiftOfferListViewController.h"
#include "../widgets/DcmtDownloadViewController.h"

// For some reason, EA packages require that we define these functions,
// but I cannot figure out how to export them, so include them here.
#include "EAPackageDefines.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

int init()
{
    ShiftLoginViewController::init();
    ShiftOfferListViewController::init();
    DcmtServiceClient::init();
    return 0;
}


int run()
{
    if (!DcmtServiceClient::instance()->isLoggedIn())
    {
        DcmtServiceClient::instance()->loginAsync();
    }
    else
    {
        ShiftOfferListViewController::instance()->show();
    }
    return 0;
}


int shutdown()
{
    DcmtDownloadManager::destroy();
    DcmtDownloadViewController::destroy();
    ShiftOfferListViewController::destroy();
    ShiftLoginViewController::destroy();
    DcmtServiceClient::release();
    return 0;
}

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
