///////////////////////////////////////////////////////////////////////////////
// IGOWebBrowser.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOBROWSER_H
#define IGOBROWSER_H

#include <QDateTime>
#include <QtWebKitWidgets/QGraphicsWebView>
#include "IGOWebBrowserJsHelper.h"
#include "IGOWebBrowserContextMenu.h"
#include "services/plugin/PluginAPI.h"

namespace Ui{

    class IGOWebBrowser;

}

namespace Origin {
namespace UIToolkit {
    class OriginTabWidget;
}
}

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API BrowserWebPage : public QWebPage
{
Q_OBJECT

public:
    BrowserWebPage(IGOWebBrowser *parentBrowser, QObject *parent = 0);

    virtual bool supportsExtension(QWebPage::Extension extension) const;
    virtual bool extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output);
 
    void setShowNavigation(bool show) { mShowNavigation = show; }
    bool showNavigation() const { return mShowNavigation;  }

protected:
    //bool acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, NavigationType type);

protected:
    QString chooseFile(QWebFrame* parentFrame, const QString& suggestedFile);
    void javaScriptAlert(QWebFrame* frame, const QString& msg);
    bool javaScriptConfirm(QWebFrame* frame, const QString& msg);
    bool javaScriptPrompt(QWebFrame* frame, const QString& msg, const QString& defaultValue, QString* result);
    QWebPage *createWindow(WebWindowType type);
    bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type);

    IGOWebBrowser *mParentBrowser;
    bool mShowNavigation;
};


class ORIGIN_PLUGIN_API BrowserGraphicsWebView : public QGraphicsWebView
{
Q_OBJECT

public:
    BrowserGraphicsWebView();
    ~BrowserGraphicsWebView();

    virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void focusInEvent(QFocusEvent *event);
    int getCursorShape();
    void setCursorShape(const int& shape);

    void cursorLeave() { cursorChanged(Qt::ArrowCursor); }

        // Determines if the current frame is trusted
    // This currently means its hostname matches our entry URL
    bool isFrameTrusted() const;
    bool isTabBlank() const;

    void loadEntryUrl(const QUrl &, bool internalTrigger = false);
    void updateScrollbars(int delta, Qt::Orientation orientation);
    void setupContextMenu(QWidget* window, IGOWebBrowser *browser);
Q_SIGNALS:
    void leavingWebContent();
    void enteringWebContent();

    void cursorChanged(int shape);
    void scrollbarUpdated();
    void urlBarFocus(bool giveFocus);
    void webviewFocus();
    void createWebBrowser();
    void reloadContent();

protected:

    IGOWebBrowserContextMenu *mContextMenu;
    int mCurrentCursor;
    // All of our helpers are only enabled for this domain for security reasons
    QString mTrustedHost;
};


struct BrowserSslInfo
{
    bool secure;
    QString hostname;
    QStringList subjectOrganization;
    QStringList issuerOrganization;
    QDateTime effectiveDate;
    QDateTime expiryDate;
    int encryptionBits;
};

class ORIGIN_PLUGIN_API IGOWebBrowser : public QWidget
{
    Q_OBJECT

public:
    IGOWebBrowser(QWidget *parent = 0);
    virtual ~IGOWebBrowser();

    void setTabWidget(Origin::UIToolkit::OriginTabWidget *tabWidget) { mTabWidget = tabWidget; }
    Origin::UIToolkit::OriginTabWidget *tabWidget() const { return mTabWidget; }
    void setBlankPage();
    void setInitialURL(const QString &url);

    virtual void paintEvent(QPaintEvent *);

    void showNavigation(bool bShow);
    bool isNavigationVisible();

    void setReducedMode(bool reduced);
    bool reducedMode() const { return mReducedMode; }
    void setupContextMenu(QWidget* window);

    BrowserWebPage * page(){return mWebPage;}

public slots:
    void iconChanged();
    void titleChanged(const QString &title);
    void urlChanged(const QUrl &url);
    void loadStarted();
    void onLoadProgress(const int& progress);
    void loadFinished(bool ok);
    void statusBarMessage(const QString &msg);
    void linkHovered(const QString & link, const QString & title, const QString & textContent);

    void contentsSizeChanged(const QSize &size);
    void horzSliderMoved(int value);
    void vertSliderMoved(int value);
    void initialLayoutCompleted();
    void backButtonClicked();
    void forwardButtonClicked();
    void newURLEntered();
    void homeButtonClicked();

    QPointer<BrowserGraphicsWebView> browser() { return mWebView; }
signals:
    void closeTabRequested();
    void needFocusOnBrowser();
    void titleChanged(IGOWebBrowser*, const QString&);
    void iconChanged(IGOWebBrowser*, const QIcon&);
    void initializeFocus();
    
protected:
    void attachHelperOnWebPage();
    bool eventFilter(QObject *obj, QEvent *event);

    void resetSslInfo();
    void extractSslInformation( QNetworkReply * reply );
    bool compareHostName(QString hostname, QString subjectCommonName);

private:
    enum URL_SUFFIX
    {
        DOT_COM,
        DOT_NET,
        DOT_ORG
    };

private:

    Ui::IGOWebBrowser *ui;
    IGOWebBrowserJsHelper mJSHelper;
    QPointer<BrowserGraphicsWebView> mWebView;
    BrowserWebPage *mWebPage;
    QString mUrlSearch;
    QString mDefaultURLonIGO;
    BrowserSslInfo mSslInfo;
    Origin::UIToolkit::OriginTabWidget *mTabWidget;
    IGOWebBrowserContextMenu *mContextMenu;
    QWidget *mParentWindow;
    bool mReducedMode;
    
    bool handleKeyPress(QKeyEvent *keyEvent);
    bool handleUrlTextEvent(QKeyEvent* keyEvent);
    const QUrl fixURL(const QString &urlStr);
    void autoCompleteURLAndLoad(URL_SUFFIX suffix);

private slots:
    void updateScrollbar();
    void onJavaScriptWindowObjectCleared();
    void replyFinished(QNetworkReply*);
    void storeTwitchLogin(QString token);
    
    void onSslIconClicked(bool checked);
    void onUrlBarFocus(bool giveFocus);
    void onWebviewFocus();
    void onInitializeFocus();
    void onCreateWebBrowser();
    void onReloadContent();
};

} // Client
} // Origin

#endif
