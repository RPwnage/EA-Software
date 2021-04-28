#include "EbisuSystemTray.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "CustomQMenu.h"
#include "originwindow.h"
#include <QApplication>

namespace Origin
{
namespace Client
{
static EbisuSystemTray* gInstance = NULL;

EbisuSystemTray::EbisuSystemTray(QObject *parent)
#if !defined(ORIGIN_MAC)
: QSystemTrayIcon(parent)
#else
: QObject(parent)
#endif
, mTrayIconMenu(NULL)
, mPrimaryWindow(NULL)
{
#if defined(ORIGIN_PC)
    // Set our icon
    showNormalSystrayIcon();
    setToolTip(tr("application_name"));
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    show();
#else 
    //we want the same jumplist menu to show up on the Dock icon
    
#endif
}


EbisuSystemTray::~EbisuSystemTray()
{
    if (mTrayIconMenu)
    {
        delete mTrayIconMenu;
        mTrayIconMenu = NULL;
    }
}


EbisuSystemTray* EbisuSystemTray::instance()
{
    if (!gInstance)
    {
        gInstance = new EbisuSystemTray(qApp);
    }

    return gInstance;
}


void EbisuSystemTray::showNormalSystrayIcon()
{
#if !defined(ORIGIN_MAC)
    setIcon(QIcon(":/origin.png"));
#endif
}


void EbisuSystemTray::showDeveloperSystrayIcon()
{
#if !defined(ORIGIN_MAC)
    setIcon(QIcon(":/origin_np.png"));
#endif
}


void EbisuSystemTray::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        showPrimaryWindow();
    }
}


void EbisuSystemTray::setPrimaryWindow(Origin::UIToolkit::OriginWindow* window)
{
    if (mPrimaryWindow != window)
    {
        mPrimaryWindow = window;

        if(mPrimaryWindow)
        {
            ORIGIN_VERIFY_CONNECT(mPrimaryWindow, SIGNAL(destroyed()), this, SLOT(onPrimaryWindowDestroyed()));
        }

        emit changedPrimaryWindow(mPrimaryWindow);
    }
}


void EbisuSystemTray::showPrimaryWindow()
{
    if (mPrimaryWindow)
    {
        // OriginWindow::showNormal resizes the window to the size it was before being 
        // hidden and makes in un-maximized. If it's already visible - don't change it. 
        if(mPrimaryWindow->isVisible() == false)
        {
            mPrimaryWindow->showNormal();
            // We need to reset the timeCreated so the dockicon behavior is correct.
            mPrimaryWindow->setCreationTime();
        }
        mPrimaryWindow->show();
        mPrimaryWindow->activateWindow();
        mPrimaryWindow->raise();

        // test to see if a modal dialog is active
        QWidget* modal = QApplication::activeModalWidget();
        if (modal)
        {	
            // if the primary window is not the active modal dialog, then activate it.
            if (mPrimaryWindow->content() != modal)
            {
                modal->show();
                modal->activateWindow();
                modal->raise();
            }
        }
    }
}


void EbisuSystemTray::onPrimaryWindowDestroyed()
{
    // We should never get here, but if our primary window gets destroyed, we need to revert back to NULL to avoid a crash.
    mPrimaryWindow = NULL;
}


void EbisuSystemTray::deleteInstance()
{	
    if ( gInstance != NULL )
    {
        delete gInstance;
        gInstance = NULL;
    }
}


void EbisuSystemTray::setTrayMenu(CustomQMenu* trayIconMenu)
{
    if (mTrayIconMenu)
    {
        mTrayIconMenu->disconnect();
        delete mTrayIconMenu;
        mTrayIconMenu = NULL;
    }
    mTrayIconMenu = trayIconMenu;
    
#if !defined(ORIGIN_MAC)
    // We don't want to set the menu bar jumplist or display the icon
    setContextMenu(mTrayIconMenu);
#else
    if(mTrayIconMenu)
    {
        mTrayIconMenu->setAsDockMenu();
    }
#endif
}

}
}