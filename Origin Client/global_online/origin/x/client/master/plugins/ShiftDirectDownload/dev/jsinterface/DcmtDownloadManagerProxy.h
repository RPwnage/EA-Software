//  DcmtDownloadManagerProxy.h
//  Copyright 2014 Electronic Arts Inc. All rights reserved.

#ifndef _DCMT_DOWNLOAD_MANAGER_PROXY_H_
#define _DCMT_DOWNLOAD_MANAGER_PROXY_H_

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QString>
#include <QObject>

namespace Origin
{

namespace Plugins
{

namespace ShiftDirectDownload
{

namespace JsInterface
{

class DcmtDownloadManagerProxy : public QObject
{
    Q_OBJECT

public:
    DcmtDownloadManagerProxy();
    virtual ~DcmtDownloadManagerProxy();

    Q_INVOKABLE void startDownload(const QString &buildId, const QString& buildVersion);
};

} // namespace JsInterface

} // namespace ShiftDirectDownload

} // namespace Plugins

} // namespace Origin

#endif // _DCMT_DOWNLOAD_MANAGER_PROXY_H_
