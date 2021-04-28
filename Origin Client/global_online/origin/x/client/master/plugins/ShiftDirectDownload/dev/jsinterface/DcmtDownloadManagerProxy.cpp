//  DcmtDownloadManagerProxy.cpp
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#include "DcmtDownloadManagerProxy.h"
#include "ShiftDirectDownload/DcmtDownloadManager.h"

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

namespace JsInterface
{

DcmtDownloadManagerProxy::DcmtDownloadManagerProxy()
{
}


DcmtDownloadManagerProxy::~DcmtDownloadManagerProxy()
{
}


void DcmtDownloadManagerProxy::startDownload(const QString &buildId, const QString& buildVersion)
{
    DcmtDownloadManager::instance()->startDownload(buildId, buildVersion);
}

} // namespace JsInterface

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin
