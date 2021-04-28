/////////////////////////////////////////////////////////////////////////////
// CloudSyncView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CLOUDSYNCVIEW_H
#define CLOUDSYNCVIEW_H

#include <QWidget>
#include <QDateTime>
#include "engine/cloudsaves/RemoteSync.h"
#include "services/plugin/PluginAPI.h"

namespace Ui
{
    class CloudDialog;
}

namespace Origin
{
namespace UIToolkit
{
    class OriginTransferBar;
}

namespace Client
{

class ORIGIN_PLUGIN_API QuotaExceededFailureView : public QWidget
{
    Q_OBJECT
public:
    QuotaExceededFailureView(const QString& displayName, const QString& MBToClear, QWidget* parent = NULL);
signals:
    void freeSpace();
    void disableCloud();
private:
    Ui::CloudDialog* mUI;
};


class ORIGIN_PLUGIN_API SyncConflictDetectedView : public QWidget
{
    Q_OBJECT
public:
    SyncConflictDetectedView(const QDateTime& localLastModified, const QDateTime& remoteLastModified, QWidget* parent = NULL);
signals:
    void stateChosen(const Origin::Engine::CloudSaves::DelegateResponse& response);
private:
    Ui::CloudDialog* mUI;
};


class ORIGIN_PLUGIN_API CloudSyncLaunchView : public QWidget
{
    Q_OBJECT
public:
    CloudSyncLaunchView(QWidget* parent = NULL);

public slots:
    /// \brief Changes the cloud progress display.
    void onCloudProgressChanged(float progress);

private:
    UIToolkit::OriginTransferBar* mCloudProgressBar;
};

}
}

#endif