/////////////////////////////////////////////////////////////////////////////
// WebBrowserViewController.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef WEBBROWSERVIEWCONTROLLER_H
#define WEBBROWSERVIEWCONTROLLER_H

#include <QObject>
#include <QPointer>
#include <QDialog>

#include "services/plugin/PluginAPI.h"

#ifdef ORIGIN_MAC
#include "services/platform/PlatformService.h"
#endif

#include "originwindow.h"
#include "engine/igo/IGOController.h"

class QGraphicsProxyWidget;
class QGraphicsItem;
class QKeyEvent;
class QFocusEvent;

namespace Origin
{
namespace Client
{
    class IGOWebBrowser;
    class BrowserGraphicsWebView;
    class BrowserWebPage;

class WebBrowserTabs;

/// \brief Controller for the avatar window. Helps load avatar web page,
/// display, and log debug info.
#ifdef ORIGIN_MAC
class ORIGIN_PLUGIN_API WebBrowserViewController : public QObject, public Origin::Engine::IWebBrowserViewController, public Origin::Services::PlatformService::KICWatchListener
#else
class ORIGIN_PLUGIN_API WebBrowserViewController : public QObject, public Origin::Engine::IWebBrowserViewController
#endif
{
    Q_OBJECT
    Q_INTERFACES(Origin::Engine::IWebBrowserViewController);
    
public:
    /// \brief Constructor
    /// \param scope TBD.
    /// \param parent The parent of the WebBrowserViewController.
    WebBrowserViewController(const int& scope, Origin::Engine::IWebBrowserViewController::BrowserFlags creationFlags, QObject* parent = 0);

    /// \brief Destructor
    ~WebBrowserViewController();

    // IWebBrowserViewController impl
    virtual QWidget* ivcView() { return mWindow; }
    virtual bool ivcPreSetup(Origin::Engine::IIGOWindowManager::WindowProperties* properties, Origin::Engine::IIGOWindowManager::WindowBehavior* behavior);
    virtual void ivcPostSetup() {}

    virtual QWebPage* page();
    
    virtual int tabCount() const;
    virtual bool isTabBlank(int idx) const;
    virtual void setTabInitialUrl(int idx, const QString& url);
    virtual bool isAddTabAllowed() const;
    virtual void addTab(const QString& url, bool blank);
    virtual void showNavigation(bool bShow);
    virtual void setID(const QString& id);
    virtual QString id();
    virtual Origin::Engine::IWebBrowserViewController::BrowserFlags creationFlags() const { return mCreationFlags; }
    
    bool isNavigationVisible();

    UIToolkit::OriginWindow* window() const {return mWindow;}

    BrowserGraphicsWebView* browserWebView(int index);

    virtual bool eventFilter( QObject * watched, QEvent * event );
    
#ifdef ORIGIN_MAC
    virtual void KICWatchEvent(int key, Qt::KeyboardModifiers modifiers);
#endif
    
signals:
    void tabChanged(QGraphicsItem*);
    
private slots:
    void closeAllTabs();
    void moveTabToFront(QGraphicsItem* tab);

private:
    void init(const QString &url, bool reducedMode = false );
    IGOWebBrowser* browser(int index);
    bool handleKeyPressEvent(QKeyEvent* keyEvent);

private:
    UIToolkit::OriginWindow* mWindow;
    QPointer<QDialog> mWindowGuard;
    QPointer<WebBrowserTabs> mWebBrowserTabs;

    QString mID;

    const int mOigScope;
    Origin::Engine::IWebBrowserViewController::BrowserFlags mCreationFlags;

#ifdef ORIGIN_MAC
    void* mKICWatcher;
#endif
    bool mInitialized;
    bool mDisableMultiTab;
};
}
}
#endif