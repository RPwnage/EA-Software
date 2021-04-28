#include "MenuProxy.h"

#include <QCursor>
#include <QApplication>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebPage>

#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "qt/originwindow.h"
#include "CustomQMenu.h"


namespace Origin
{
namespace Client
{
namespace JsInterface
{

MenuProxy::MenuProxy(QWebPage *page, QObject *parent) 
    : QObject(parent)
    , mPage(page)
{
    bool inIGO = Origin::Engine::IGOController::instance()->isActive();
	mNativeMenu = new CustomQMenu(inIGO ? IGOScope : ClientScope);

    if (inIGO)
    {
        Origin::Engine::IGOController::instance()->igowm()->createContextMenu(mPage->view(), mNativeMenu, this, false);
    }
    ORIGIN_VERIFY_CONNECT(mNativeMenu, SIGNAL(aboutToShow()), this, SIGNAL(aboutToShow()));
    ORIGIN_VERIFY_CONNECT(mNativeMenu, SIGNAL(aboutToHide()), this, SIGNAL(aboutToHide()));
}

MenuProxy::~MenuProxy()
{
    delete mNativeMenu;
    delete mSubmenu;
}

QObject* MenuProxy::addAction(const QString &text)
{
	return mNativeMenu->addAction(text);
}
    
QObject* MenuProxy::addMenu(const QString &text)
{
    MenuProxy *proxy = new MenuProxy(mPage, this);

    proxy->mNativeMenu->setTitle(text);
    QAction* submenuAction = mNativeMenu->addMenu(proxy->mNativeMenu);

    mSubmenu = submenuAction->menu();
    onShowSubmenu();
    ORIGIN_VERIFY_CONNECT(mSubmenu, SIGNAL(aboutToShow()), this, SLOT(onShowSubmenu()));
    ORIGIN_VERIFY_CONNECT(mSubmenu, SIGNAL(aboutToHide()), this, SLOT(onHideSubmenu()));

    return proxy;
}

void MenuProxy::addObjectName(const QString &text)
{
    if (mNativeMenu)
        mNativeMenu->setObjectName(text);
}

void MenuProxy::onShowSubmenu()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        Origin::Engine::IGOController::instance()->igowm()->showContextSubMenu(mPage->view(), mSubmenu);
    }
}

void MenuProxy::onHideSubmenu()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        Origin::Engine::IGOController::instance()->igowm()->hideContextSubMenu(mPage->view());
    }
}

QObject *MenuProxy::addSeparator()
{
	return mNativeMenu->addSeparator();
}

void MenuProxy::popup()
{
    int minW = mNativeMenu->minimumWidth();
    QString objName = mNativeMenu->objectName();
    // because the IGO isn't always picking up the qss file, this is needed
    if (objName == "friendsContextMenu" && minW == 0)
        mNativeMenu->setMinimumWidth(191);
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        Origin::Engine::IIGOWindowManager::WindowProperties cmProperties;
        if (Origin::Engine::IGOController::instance()->igowm()->showContextMenu(mPage->view(), mNativeMenu, &cmProperties))
            mNativeMenu->popup(cmProperties.position());

        // Delay this signal because we are getting a immediate hide event from the creation
        // This hide event if coming from Qt patch 0052
        // This fixes https://developer.origin.com/support/browse/EBIBUGS-25252
        // Make this unique just incase we popup the same menu mulitple times ex:My Games
        connect(mNativeMenu, SIGNAL(aboutToHide()), this, SLOT(hiding()), Qt::UniqueConnection);
        return;
    }

    mNativeMenu->popup(QCursor::pos());
}

void MenuProxy::popup(QVariantMap pos)
{
    int minW = mNativeMenu->minimumWidth();
    QString objName = mNativeMenu->objectName();
    // because the IGO isn't always picking up the qss file, this is needed
    if (objName == "friendsContextMenu" && minW == 0)
        mNativeMenu->setMinimumWidth(191);
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        if (mPage != NULL)
        {
            QPoint offset = mPage->view()->mapTo(mPage->view()->topLevelWidget(), QPoint(0, 0));
            QPoint cmPos(pos["x"].toInt() + offset.x(), pos["y"].toInt() + offset.y());

            Origin::Engine::IIGOWindowManager::WindowProperties cmProperties;
            cmProperties.setPosition(cmPos);
            if (Origin::Engine::IGOController::instance()->igowm()->showContextMenu(mPage->view(), mNativeMenu, &cmProperties))
                mNativeMenu->popup(cmProperties.position());
        }
        return;
    }

    if (pos.contains("x") && pos.contains("y"))
    {
        QPoint nativePoint(pos["x"].toInt(), pos["y"].toInt());
             
        // Absolutely terrible
        // Try to find the web view the cursor is over
        QWidget *mouseWidget = qApp->widgetAt(QCursor::pos());

        if (dynamic_cast<QWebView*>(mouseWidget))
        {
            // Map the document coordinates to global coordinates
            mNativeMenu->popup(mouseWidget->mapToGlobal(nativePoint));
            return;
        }
    }

    popup();
}
    
void MenuProxy::onFocusChanged(QWidget* window, bool hasFocus)
{
    Q_UNUSED(window);
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        mNativeMenu->hide();
    }
}

void MenuProxy::onDeactivate()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        mNativeMenu->hide();
    }
}

void MenuProxy::hiding()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        Origin::Engine::IGOController::instance()->igowm()->hideContextMenu(mPage->view());
    }
}

void MenuProxy::hide()
{
	mNativeMenu->hide();
}

void MenuProxy::clear()
{
	mNativeMenu->clear();
}
    
void MenuProxy::addAction(QObject *obj)
{
   QAction *action = dynamic_cast<QAction*>(obj);

   if (action)
   {
       mNativeMenu->addAction(action);
   }
}

void MenuProxy::removeAction(QObject *obj)
{
   QAction *action = dynamic_cast<QAction*>(obj);

   if (action)
   {
       mNativeMenu->removeAction(action);
   }
}

QObject* MenuProxy::menuAction()
{
    return mNativeMenu->menuAction();
}

void MenuProxy::setMinWidth(int w)
{
    mNativeMenu->setStyleSheet(QString("QMenu { min-width: %1px; }").arg(w));
}

void MenuProxy::setVisible(bool visible)
{
    mNativeMenu->menuAction()->setVisible(visible);        
}

}
}
}
