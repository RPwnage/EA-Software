/////////////////////////////////////////////////////////////////////////////
// MenuFlowWindowViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MENMUFLOWINDOWVIEWCONTROLLER_H
#define MENMUFLOWINDOWVIEWCONTROLLER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"
#include "engine/content/Entitlement.h"

class QMenu;
class QAction;

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Client
{
/// \brief Controller for 
class ORIGIN_PLUGIN_API MenuFlowWindowViewController : public QObject
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent - The parent of the View Controller.
    MenuFlowWindowViewController(QObject* parent = 0);

    /// \brief Destructor
    ~MenuFlowWindowViewController();
    
    QMenu* menu() const {return mMenu;}

signals:
    void nextStep();

private slots:
    void setEntitlement(Engine::Content::EntitlementRef ent = Engine::Content::EntitlementRef());
    void startPlayFlow();
    void startCloudSync();
    void startBroadcastFlow();
    void startDownloadFlow();
    void startDLCDownloadFlow();
    void triggerDownloadError();
    void triggerDownloadErrorRetry();
    void clearCompleteQueue();
    void startUpdate();
    void startRepair();
    void showGameNetPromoter();
    void showOriginNetPromoter();
    void showPromoOriginStarted();
    void showPromoGameplayFinished();
    void showPromoDownloadStarted();
    void showCloudSyncFlowWindows();
    void showPlayFlowWindows();
    void showDownloadFlowWindows();
    void showDownloadFlowErrorWindows();
    void showEntitlementWindows();
    void showRTPWindows();
    void showITOWindows();
    void showFirstLaunchWindows();
    void showMessageAreaWindows();
    void showGroipWindows();
    void showGroipChatError();
    void showSaveBackupRestore();
    void showCrashWindow();
    void showBroadcastWindows();


private:
    void addAction(QAction* newAction, const QString& title, const char* slot);
    void addAction(QMenu* menuParent, QAction* newAction, const QString& title, const char* slot);

    Engine::Content::EntitlementRef mEntitlement;
    const QString mProductID;
    QMenu* mMenu;
};

}
}
#endif
