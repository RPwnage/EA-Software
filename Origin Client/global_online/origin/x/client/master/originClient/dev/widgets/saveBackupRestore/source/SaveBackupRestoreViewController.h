/////////////////////////////////////////////////////////////////////////////
// SaveBackupRestoreViewController.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef SAVEBACKUPRESTOREVIEWCONTROLLER_H
#define SAVEBACKUPRESTOREVIEWCONTROLLER_H

#include <QObject>
#include "engine/content/ContentTypes.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}

namespace Client
{
class ORIGIN_PLUGIN_API SaveBackupRestoreViewController : public QObject
{
	Q_OBJECT

public:
    SaveBackupRestoreViewController(Engine::Content::EntitlementRef entitlement, QObject* parent = 0, const bool& inDebug = false);
    ~SaveBackupRestoreViewController();

public slots:
    /// \brief Show message about restoring backed up save game was successful
    void showSaveBackupRestoreSuccessful();

    /// \brief Show message about restoring backed up save game failed
    void showSaveBackupRestoreFailed();

signals:
    void saveBackupRestoreFlowSuccess();
    void saveBackupRestoreFlowFailed();


private slots:
    void onLinkActivated(const QString& link);

private:
    void closeWindow();
    Engine::Content::EntitlementRef mEntitlement;
    UIToolkit::OriginWindow* mWindow;
    const bool mInDebug;
};
}
}

#endif