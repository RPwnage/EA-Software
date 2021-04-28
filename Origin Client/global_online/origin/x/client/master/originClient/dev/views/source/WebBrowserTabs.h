/////////////////////////////////////////////////////////////////////////////
// WebBrowserTabs.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef WEBBROWSERTABS_H
#define WEBBROWSERTABS_H

#include <QWidget>
#include <QPointer>

#include "services/plugin/PluginAPI.h"

class QGraphicsItem;

namespace Origin
{
namespace UIToolkit
{
    class OriginTabWidget;
    class OriginWindow;
}
namespace Client
{
    class IGOWebBrowser;
    class BrowserGraphicsWebView;

class ORIGIN_PLUGIN_API WebBrowserTabs : public QObject
{
    Q_OBJECT

public:
    WebBrowserTabs(QObject* parent = 0);
    ~WebBrowserTabs();

    UIToolkit::OriginTabWidget* view() {return mTabWidget;}
    int count() const;
    IGOWebBrowser* browser(int index) const;
    BrowserGraphicsWebView* browserWebView(int index) const;
    void setTabText(QWidget* widget, const QString &text);
    void setTabIcon(QWidget* widget, const QIcon &icon);

    bool isAddTabAllowed() const;
    void addTab(const QString& url, bool blank = false);
    void nextTab();
    void previousTab();
    void activateTab(int index);
    void closeTab();

    void setParentWindow(UIToolkit::OriginWindow* parentWindow) { mParentWindow = parentWindow; }
    void setEventFilterObject(QObject *eventFilterObject) { mEventFilterObject = eventFilterObject; }
    void setReducedMode(bool reduced);

signals:
    void tabChanged(QGraphicsItem*);
    void lastTabClosed();

public slots:
    void onTitleChanged(IGOWebBrowser* browser, const QString& title);
    void onIconChanged(IGOWebBrowser* browser, const QIcon& icon);
    void onTabChanged(int index);
    void onTabClosed(int index);
    void onAddTabClicked();
    void onCloseTabRequested();

private slots:
    void enteringWebContent();
    void leavingWebContent();
    void setCursor(int cursorShape);
    void needFocusOnBrowser();
    
private:
    UIToolkit::OriginTabWidget* mTabWidget;
    QPointer<QWidget> mTabWidgetGuard;

    UIToolkit::OriginWindow* mParentWindow; // used for setting title bar text of container window
    QObject* mEventFilterObject; // used for handling hotkeys when focus is on webview
    bool mReducedMode;
};
}
}
#endif