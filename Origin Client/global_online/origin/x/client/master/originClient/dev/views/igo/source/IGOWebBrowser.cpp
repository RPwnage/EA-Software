///////////////////////////////////////////////////////////////////////////////
// IGOWebBrowser.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOWebBrowser.h"
#include "ui_IGOWebBrowser.h"
#include "IGOWebSslInfo.h"
#include "IGOUIFactory.h"

#include <QDir>
#include <QGraphicsSceneWheelEvent>
#include <QtWebKitWidgets/QWebFrame>
#include <QNetworkReply>
#include <QtWebKit/QWebHistory>
#include <QSslConfiguration>
#include <QSslKey>
#include <QSslCipher>
#include <QRegExp>
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "services/platform/PlatformService.h"
#include "services/network/NetworkAccessManager.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/settings/SettingsManager.h"
#include "Qt/originwindow.h"
#include "Qt/originwindowmanager.h"
#include "Qt/originmsgbox.h"
#include "Qt/originwebview.h"
#include "utilities/LocalizedDateFormatter.h"
#include "widgets/igo/source/WebBrowserViewController.h"
#include "jshelpers/source/CommonJsHelper.h"
//
#include "TelemetryAPIDLL.h"

#define DEFAULT_FAVICON QIcon(":/Resources/GUI/browser/default_page.png")
#define DONE "ebisu_igo_browser_done"
#define LOADING "ebisu_igo_browser_loading"
#define WEB_SSL_LEFTEDGE_TO_ARROW_PIXELS 35
#define WEB_SSL_X_ADJUST 13
#define WEB_SSL_Y_ADJUST 10
#define WEB_SSL_LOCK_ICON QIcon(":/GUI/IGO/ssl_icon.png")

namespace Origin
{
namespace Client
{

// This class is used to remove the "flash" when right-clicking on the URL bar selection to show the context menu. This is due to
// the "intermediates" we have in OIG -> when right-clicking, we actually momentarily lose focus (and the selection) and gain it back
// -> after losing focus, we re-select the text and also change the "inactive" selection color to match the "active" selection color.
// We depend on the IGOWebBrowser to properly clear the selection depending on the user's activities!
class IGOBrowserLineEdit : public QLineEdit
{
public:
    IGOBrowserLineEdit(QWidget* parent) : QLineEdit(parent)
    {
        _activeHighlightColor = palette().color(QPalette::Active, QPalette::Highlight);
        _inactiveHighlightColor = palette().color(QPalette::Inactive, QPalette::Highlight);
    }
    
    virtual void focusInEvent(QFocusEvent* evt)
    {
        // Restore proper inactive selection color
        QString newColor = QString("selection-background-color:%1").arg(_inactiveHighlightColor.rgba());
        setStyleSheet(newColor);
        
        QLineEdit::focusInEvent(evt);
    }
    virtual void focusOutEvent(QFocusEvent* evt)
    {
        // Save current selection
        int selectionStart = -1;
        int selectionLength = 0;
        
        if (hasSelectedText())
        {
            selectionStart = this->selectionStart();
            selectionLength = this->selectedText().length();
        }
        
        // Apply normal behavior
        QLineEdit::focusOutEvent(evt);

        // Restore selection/use active selection color when inactive
        if (selectionStart >= 0)
        {
            setStyleSheet(QString("selection-background-color:%1").arg(_activeHighlightColor.rgba()));
            setSelection(selectionStart, selectionLength);
        }
    }
    
private:
    QColor _activeHighlightColor;
    QColor _inactiveHighlightColor;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// browser helper
BrowserGraphicsWebView::BrowserGraphicsWebView()
    :  mContextMenu(NULL), mCurrentCursor(Qt::ArrowCursor)

    
{
}

BrowserGraphicsWebView::~BrowserGraphicsWebView()
{
    if( mContextMenu )
    {
        delete mContextMenu;
        mContextMenu = NULL;
    }
}

int BrowserGraphicsWebView::getCursorShape()
{
    return mCurrentCursor;
}

void BrowserGraphicsWebView::setCursorShape(const int& shape)
{
    mCurrentCursor = shape;
    emit cursorChanged(mCurrentCursor);
}

QVariant BrowserGraphicsWebView::itemChange(GraphicsItemChange change, const QVariant& value)
{

    if(change==ItemCursorChange)
    {
        if( value.type() == QVariant::Cursor )
        {
            int shape = qvariant_cast<QCursor>(value).shape();
            setCursorShape(shape);
        }
    }

    return QGraphicsWebView::itemChange(change, value);
}

void BrowserGraphicsWebView::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    updateScrollbars(event->delta(), event->orientation());
    QGraphicsWebView::wheelEvent(event);
}

// Reset the cursor when leaving the view + notify the container so that we can disable the window resize/move
// (this is to avoid conflicts when setting the cursor shape)
void BrowserGraphicsWebView::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    Qt::CursorShape currCursor = cursor().shape();
    if (currCursor != Qt::ArrowCursor)
    {
        setCursorShape(Qt::ArrowCursor);
    }
    
    QGraphicsWebView::hoverEnterEvent(event);
    
    emit enteringWebContent();
}

// Reset the cursor when leaving the view + notify the container so that we can re-enable the window resize/move
// (this is to avoid conflicts when setting the cursor shape)
void BrowserGraphicsWebView::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    Qt::CursorShape currCursor = cursor().shape();
    if (currCursor != Qt::ArrowCursor)
    {
        setCursorShape(Qt::ArrowCursor);
    }
    
    QGraphicsWebView::hoverLeaveEvent(event);

    // Reset any "highlights" (for example www.google.com, where the cursor selects one of the "Search, Images, Mail,... " tabs and
    // moves upwards/outside the web view -> send a fake mouse event outside the viewport
    QGraphicsSceneMouseEvent viewEvent(QEvent::GraphicsSceneMouseMove);
    viewEvent.setPos(QPointF(-100, -100));
    mouseMoveEvent(&viewEvent);
    
    emit leavingWebContent();
}

// Skip the "synthetic" context menu event we create for the buddylist when right-clicking on the window, otherwise
// a little popup shows up that takes over the focus/stalls OIG and the client.
void BrowserGraphicsWebView::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    mContextMenu->popup();
    return;
}

void BrowserGraphicsWebView::updateScrollbars(int delta, Qt::Orientation orientation)
{
    QWebFrame *mainFrame = page()->mainFrame();
    
#ifdef ORIGIN_MAC
    if (orientation == Qt::Vertical)
        mainFrame->scroll(0, -delta);
    
    else
        mainFrame->scroll(-delta, 0);
#else
    mainFrame->scroll(0, -delta);
#endif
    
    scrollbarUpdated();
}

void BrowserGraphicsWebView::setupContextMenu(QWidget* window, IGOWebBrowser *browser)
{
    mContextMenu = new IGOWebBrowserContextMenu(window, browser);
}

void BrowserGraphicsWebView::loadEntryUrl(const QUrl &url, bool internalTrigger)
{
    if(!url.isEmpty())
    {
        // If trigger by user-interaction (ie the user cliked on the home button, entered a new URL, etc) - give the webpage focus
        if (!internalTrigger)
            emit webviewFocus();
        
        mTrustedHost = url.host();
        setUrl(url);
    }
}

bool BrowserGraphicsWebView::isFrameTrusted() const
{
    QString hostname = page()->mainFrame()->url().host();
    bool isTrusted = (hostname == mTrustedHost) || 
        hostname.endsWith(QString(".ea.com"), Qt::CaseInsensitive) ||
        hostname.endsWith(QString(".secure.force.com"), Qt::CaseInsensitive) ||
        hostname.endsWith(QString(".origin.com"), Qt::CaseInsensitive);

    return isTrusted;
}

bool BrowserGraphicsWebView::isTabBlank() const
{
    return url().isEmpty();
}

void BrowserGraphicsWebView::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key_PageUp:
        {
            QWebFrame *mainFrame = page()->mainFrame();
            mainFrame->scroll(0, -size().height());

            scrollbarUpdated();
        }
        break;

    case Qt::Key_PageDown:
        {
            QWebFrame *mainFrame = page()->mainFrame();
            mainFrame->scroll(0, size().height());

            scrollbarUpdated();
        }
        break;

    // handle focus url
    case Qt::Key_L:
        {
            if (event->modifiers() & Qt::ControlModifier)
            {
                emit urlBarFocus(true);
            }
        }
        break;
    // handle copy
    case Qt::Key_C:
        {
            if (event->modifiers() & Qt::ControlModifier)
            {
                triggerPageAction(QWebPage::Copy);
            }
        }
        break;
        
    // create new window
    case Qt::Key_N:
        {
            if (event->modifiers() & Qt::ControlModifier)
            {
                emit createWebBrowser();
            }
        }
        break;
        
    case Qt::Key_F5:
        {
            emit reloadContent();
        }
        break;

    case Qt::Key_Delete:
        {
            triggerPageAction(QWebPage::SelectNextChar);
            if (!this->page()->selectedText().isEmpty())
            {
                QKeyEvent backspace(QEvent::KeyPress, Qt::Key_Backspace,Qt::NoModifier);
                QGraphicsWebView::keyPressEvent(&backspace);
                return;
            }

        }
        
    default:
        break;
    }
    
    QGraphicsWebView::keyPressEvent(event);
}

void BrowserGraphicsWebView::focusInEvent(QFocusEvent *event)
{
    // Make sure the URL bar doesn't have focus when the webview does!
    // This is really to try and have a safety net
    emit urlBarFocus(false);
    
    QGraphicsWebView::focusInEvent(event);
}

BrowserWebPage::BrowserWebPage(IGOWebBrowser *parentBrowser, QObject *parent): QWebPage(parent)
{
    Origin::Services::NetworkAccessManager *igoNetworkAccessManager = NULL;
    mParentBrowser = parentBrowser;
    mShowNavigation = false;

    // Make a dedicated network access manager for us that sets hints
    // that our requests are user generated. This can affect logging and error
    // handling
    // don't share the network access manager across multiple pages, this would result in multiple "replyFinished" executions!!!
    igoNetworkAccessManager = Origin::Services::NetworkAccessManagerFactory::instance()->createNetworkAccessManager();
    igoNetworkAccessManager->setRequestSource(Origin::Services::NetworkAccessManager::UserRequestSourceHint);

    setNetworkAccessManager(igoNetworkAccessManager);
    QObject::disconnect(networkAccessManager(), SIGNAL(finished(QNetworkReply*)),parentBrowser, SLOT(replyFinished(QNetworkReply*)));
    ORIGIN_VERIFY_CONNECT(networkAccessManager(), SIGNAL(finished(QNetworkReply*)),parentBrowser, SLOT(replyFinished(QNetworkReply*)));


    // There's a bug with accelerated compositing with at least 
    // Qt 4.7.1/QtWebKit 2.0 and Qt 4.7.3/QtWebKit 2.1 where it will sometimes
    // remove compositing layers while a draw is in progress which will cause
    // Qt to try to read from a freed QGraphicsItem
    // Work around this by disabling compositing
    
    // Re-Enabled it for Qt 4.8.2 to speed up web page rendering EBIBUGS-15658

    // Disabled it for EBIBUGS-21139
    settings()->setAttribute(QWebSettings::AcceleratedCompositingEnabled, false);
    
    // Make it clear that we are running in IGO for any "external" component that needs to know - for example, the HTMLVideoElement is going to
    // use this information to disable fullscreen for html5 movies
    setProperty("OIG", true);
}

bool BrowserWebPage::supportsExtension(QWebPage::Extension extension) const
{
    if (extension == QWebPage::ErrorPageExtension)
        return true;
    return false;
}

bool BrowserWebPage::extension(Extension extension, const ExtensionOption* option, ExtensionReturn* output)
{
    const QWebPage::ErrorPageExtensionOption* info = static_cast<const QWebPage::ErrorPageExtensionOption*>(option);
    QWebPage::ErrorPageExtensionReturn* errorPage = static_cast<QWebPage::ErrorPageExtensionReturn*>(output);

    QString html = tr("ebisu_igo_browser_error_page").arg(info->url.host());
    //html.append("<br/>");
    //html.append(info->errorString.toUtf8());
    errorPage->content = QString("<html>%1</html>").arg(html).toUtf8();
    errorPage->baseUrl = info->url;

    return true;
}

QString BrowserWebPage::chooseFile(QWebFrame* parentFrame, const QString& suggestedFile)
{
    Q_UNUSED(parentFrame);
    Q_UNUSED(suggestedFile);
    qWarning() << "BrowserWebPage::" << __FUNCTION__ << "not implemented" << "Returning empty string";
    return QString();
}

void BrowserWebPage::javaScriptAlert(QWebFrame* frame, const QString& msg)
{
   using namespace Origin::UIToolkit;
    OriginWindow* messageBox = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok, OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
    messageBox->msgBox()->setup(OriginMsgBox::NoIcon, frame->title().toUpper(), msg);

    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(finished(int)), messageBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), messageBox, SLOT(reject()));

    Origin::Engine::IGOController::instance()->alert(messageBox);
    messageBox->exec();
}

bool BrowserWebPage::javaScriptConfirm(QWebFrame* frame, const QString& msg)
{
    using namespace Origin::UIToolkit;
    OriginWindow* messageBox = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel, OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
    messageBox->msgBox()->setup(OriginMsgBox::NoIcon, frame->title().toUpper(), msg);

    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(finished(int)), messageBox, SLOT(close()));
    ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), messageBox, SLOT(accept()));
    ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), messageBox, SLOT(reject()));

    Origin::Engine::IGOController::instance()->alert(messageBox);
    
    bool isConfirmed = messageBox->exec() == QDialog::Accepted;
    return isConfirmed;
}

bool BrowserWebPage::javaScriptPrompt(QWebFrame* frame, const QString& msg, const QString& defaultValue, QString* result)
{
    Q_UNUSED(frame);
    Q_UNUSED(msg);
    Q_UNUSED(defaultValue);
    Q_UNUSED(result);
    qWarning() << "BrowserWebPage::" << __FUNCTION__ << "not implemented" << msg << defaultValue << "returning false";
    return false;
}

QWebPage *BrowserWebPage::createWindow(WebWindowType type)
{
    if (type == WebBrowserWindow)
    {
        Origin::Engine::IWebBrowserViewController::BrowserFlags flags = Origin::Engine::IWebBrowserViewController::IGO_BROWSER_SHOW_VIEW;
        if (mParentBrowser && mParentBrowser->isNavigationVisible())
            flags = Origin::Engine::IWebBrowserViewController::IGO_BROWSER_SHOW_NAV;
        
        Origin::Engine::IIGOWindowManager::WindowProperties settings;
        Origin::Engine::IIGOWindowManager::WindowBehavior behavior;
        Origin::Engine::IWebBrowserViewController* controller = Origin::Engine::IGOController::instance()->createWebBrowser("", flags, settings, behavior);
        if (controller)
        {
            Origin::Engine::IGOController::instance()->igowm()->maximizeWindow(controller->ivcView());
            return controller->page();
        }

        return this;
    }
    return this;
}

bool BrowserWebPage::acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest &request, NavigationType type)
{
    /* NOT IN USE, BUT KEPT AROUND IN CASE THINGS CHANGE
    if (!mShowNavigation)
    {
        // When not showing the navigation controls, don't allow any of the default shortcuts (backspace for example)
        // This is for the support to the browser miniapps (for example The Sims4) - if you need to enable this
        // for certain cases, use the IGO_BROWSER_MINI_APP flag instead to decide whether to allow this.
        if (type == QWebPage::NavigationTypeBackOrForward || type == QWebPage::NavigationTypeReload)
            return false;
    }
    */

    return QWebPage::acceptNavigationRequest(frame, request, type);
}

// igo browser

IGOWebBrowser::IGOWebBrowser(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::IGOWebBrowser)
    , mJSHelper( this )
    , mTabWidget( NULL )
    , mContextMenu( NULL )
    , mParentWindow( NULL )
    , mReducedMode(false)
{
    ui->setupUi(this);
    ui->backButton->setEnabled(false);
    ui->forwardButton->setEnabled(false);

    // set the default favicon
    ui->urlIcon->setIcon(DEFAULT_FAVICON);
    
    // For now, change the URL bar control to use our local version that keeps the selection when losing focus
    {
        int myIndex = ui->horizontalLayout_3->indexOf(ui->urlText);
        if (myIndex >= 0)
        {
            ui->horizontalLayout_3->removeWidget(ui->urlText);
            delete ui->urlText;
            
            ui->urlText = new IGOBrowserLineEdit(ui->urlProgress);
            ui->urlText->setObjectName(QStringLiteral("urlText"));
            
            ui->horizontalLayout_3->insertWidget(myIndex, ui->urlText);
        }
    }
    
    // create graphicswebview
    mWebView = new BrowserGraphicsWebView();
    // Set user stylesheet so we can override the webkit scrollbars
    if (mWebView->settings())
    {
        // Inject scrollbar CSS
        mWebView->settings()->setUserStyleSheetUrl(QUrl("data:text/css;charset=utf-8;base64," +
            Origin::UIToolkit::OriginWebView::getStyleSheet(true, true).toUtf8().toBase64()));
    }
    mWebPage = new BrowserWebPage(this);
    mWebView->setPage(mWebPage);
    mWebView->setCacheMode(QGraphicsItem::NoCache);

    // create style for browser scrollbars
    QWebFrame *mainFrame = mWebView->page()->mainFrame();
    mainFrame->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    mainFrame->setScrollBarPolicy(Qt::Horizontal, Qt::ScrollBarAlwaysOff);
    ORIGIN_VERIFY_CONNECT(mWebView->page()->mainFrame() ,SIGNAL(javaScriptWindowObjectCleared () ), this,SLOT(onJavaScriptWindowObjectCleared()) ); 
    attachHelperOnWebPage();
    // set browser settings
    mWebView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    mWebView->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
    mWebView->settings()->setAttribute(QWebSettings::DnsPrefetchEnabled, true);
    mWebView->settings()->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
    mWebView->settings()->setAttribute(QWebSettings::JavascriptCanOpenWindows, true);
    mWebView->settings()->setAttribute(QWebSettings::TiledBackingStoreEnabled, false);  // increases performance, but causes rendering issues:
                                                                                        // https://developer.origin.com/support/browse/EBIBUGS-28657 
                                                                                        // https://developer.origin.com/support/browse/EBIBUGS-28656
                                                                                        // https://developer.origin.com/support/browse/EBIBUGS-28655
                                                                                        // https://developer.origin.com/support/browse/EBIBUGS-28654
                                                                                        // https://developer.origin.com/support/browse/EBIBUGS-28395

    // where do you put this?
    QString tempPath = QDir::tempPath();
    mWebView->settings()->setIconDatabasePath(tempPath);
    mWebView->settings()->setLocalStoragePath(tempPath);
    // setup signals
    ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(iconChanged()), this, SLOT(iconChanged()));
    ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(titleChanged(const QString&)), this, SLOT(titleChanged(const QString &)));
    ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));
    ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(loadStarted()), this, SLOT(loadStarted()));
    ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(loadFinished(bool)), this, SLOT(loadFinished(bool)));
    ORIGIN_VERIFY_CONNECT(mWebView->page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString,QString)));
    ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(loadProgress(int)), this, SLOT(onLoadProgress(const int&)));
    ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(scrollbarUpdated()), this, SLOT(updateScrollbar()));
    ORIGIN_VERIFY_CONNECT_EX(mWebView, SIGNAL(urlBarFocus(bool)), this, SLOT(onUrlBarFocus(bool)), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mWebView, SIGNAL(webviewFocus()), this, SLOT(onWebviewFocus()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mWebView, SIGNAL(createWebBrowser()), this, SLOT(onCreateWebBrowser()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(mWebView, SIGNAL(reloadContent()), this, SLOT(onReloadContent()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT_EX(this, SIGNAL(initializeFocus()), this, SLOT(onInitializeFocus()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT(ui->backButton, SIGNAL(clicked()), this, SLOT(backButtonClicked()));
    ORIGIN_VERIFY_CONNECT(ui->forwardButton, SIGNAL(clicked()), this, SLOT(forwardButtonClicked()));
    ORIGIN_VERIFY_CONNECT(ui->HomeButton, SIGNAL(clicked()), this, SLOT(homeButtonClicked()));

    ORIGIN_VERIFY_CONNECT(mWebView->page()->mainFrame(), SIGNAL(contentsSizeChanged(QSize)), this, SLOT(contentsSizeChanged(QSize)));
    ORIGIN_VERIFY_CONNECT(mWebView->page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(initialLayoutCompleted())); 

    ORIGIN_VERIFY_CONNECT(ui->stopButton, SIGNAL(clicked()), mWebView, SLOT(stop()));
    ORIGIN_VERIFY_CONNECT(ui->reloadButton, SIGNAL(clicked()), mWebView, SLOT(reload()));
    ORIGIN_VERIFY_CONNECT(ui->reloadButton_reduced, SIGNAL(clicked()), mWebView, SLOT(reload()));

    // connect scrollbar
#ifdef ORIGIN_MAC
    ui->verticalScrollBar->installEventFilter(this);
    ui->horizontalScrollBar->installEventFilter(this);
#endif
    ORIGIN_VERIFY_CONNECT(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzSliderMoved(int)));
    ORIGIN_VERIFY_CONNECT(ui->verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(vertSliderMoved(int)));

    // allow the java script to close the whole IGO browser
    ORIGIN_VERIFY_CONNECT(&mJSHelper, SIGNAL(closeIGOBrowserTab()), this, SIGNAL(closeTabRequested()));
    ORIGIN_VERIFY_CONNECT(&mJSHelper, SIGNAL(closeIGOBrowserByTwitch(QString)), this, SLOT(storeTwitchLogin(QString)));

    // We need our own context menu to handle copy/paste
    ui->urlText->setContextMenuPolicy(Qt::NoContextMenu);
    ui->graphicsView->installEventFilter(this);

    ui->sslIcon->setIcon(WEB_SSL_LOCK_ICON);
    ORIGIN_VERIFY_CONNECT(ui->sslIcon, SIGNAL(clicked(bool)), this, SLOT(onSslIconClicked(bool)));
}


void IGOWebBrowser::storeTwitchLogin(QString token)
{
#ifdef ORIGIN_PC
    Engine::IGOController::instance()->twitch()->storeTwitchLogin(token, Origin::Engine::IIGOCommandController::CallOrigin_CLIENT);
#endif
    this->close();
}

void IGOWebBrowser::onSslIconClicked(bool /*checked*/)
{
    QPoint iconPos = ui->sslIcon->pos();
    QPoint browserPos;
    
    Origin::Engine::IIGOWindowManager::WindowProperties wndProperties;
    if (Origin::Engine::IGOController::instance()->igowm()->windowProperties(mParentWindow, &wndProperties))
        browserPos = wndProperties.position();
    
    iconPos = ui->sslIcon->mapTo(mParentWindow, iconPos);
    int height = ui->sslIcon->height();
    int x = browserPos.x() + iconPos.x() - WEB_SSL_LEFTEDGE_TO_ARROW_PIXELS - WEB_SSL_X_ADJUST;
    int y = browserPos.y() + iconPos.y() + height - WEB_SSL_Y_ADJUST;

    IGOWebSslInfo *igoWebSslInfo = new IGOWebSslInfo();
    if( igoWebSslInfo )
    {
        igoWebSslInfo->setHostName(mSslInfo.hostname);
        igoWebSslInfo->setSubject(mSslInfo.subjectOrganization.join(';'));
        igoWebSslInfo->setIssuer(mSslInfo.issuerOrganization.join(';'));
        const QString effectiveDate = Origin::Client::LocalizedDateFormatter().format(mSslInfo.effectiveDate.date(), Origin::Client::LocalizedDateFormatter::LongFormat);
        const QString expiryDate = Origin::Client::LocalizedDateFormatter().format(mSslInfo.expiryDate.date(), Origin::Client::LocalizedDateFormatter::LongFormat);
        igoWebSslInfo->setValidity(effectiveDate, expiryDate);
        QString encryptionStrength = QString::number(mSslInfo.encryptionBits);
        igoWebSslInfo->setEncryption(encryptionStrength);
        igoWebSslInfo->adjustSize();
        
        Origin::Engine::IIGOWindowManager::WindowProperties properties;
        properties.setWId(IGOUIFactory::ClientWID_SSLINFO);
        properties.setPosition(QPoint(x, y));
        
        Origin::Engine::IIGOWindowManager::WindowBehavior behavior;
        behavior.deleteOnLosingFocus = true;
        
        Origin::Engine::IGOController::instance()->igowm()->addPopupWindow(igoWebSslInfo, properties, behavior);
    }
}


IGOWebBrowser::~IGOWebBrowser()
{
    ui->urlText->removeEventFilter(this);
    ui->graphicsView->removeEventFilter(this);

    // Make sure to remove focus so that the content isn't accessed as "previous" proxy
    ui->urlText->clearFocus();
    if (mWebView)
        mWebView->clearFocus();
    
    clearFocus();
    
    if(mWebView)
    {
        mWebView->deleteLater();
        mWebView = NULL;
    }
    
    if(mWebPage)
    {
        mWebPage->deleteLater();
        mWebPage = NULL;
    }

    if( mContextMenu )
    {
        delete mContextMenu;
        mContextMenu = NULL;
    }

    delete ui;
    ui = NULL;
}

void IGOWebBrowser::setInitialURL(const QString &url)
{
    if (url == "")
    {
        QString defaultUrl = Origin::Services::readSetting(Origin::Services::SETTING_IGODefaultURL);
        mWebView->loadEntryUrl(defaultUrl);
        mDefaultURLonIGO = defaultUrl;
    }
    else
    {
        mDefaultURLonIGO = fixURL(url).toEncoded ();
        mWebView->loadEntryUrl(mDefaultURLonIGO );
    }
    
    ui->urlText->setText(mWebView->url().toEncoded());
}

void IGOWebBrowser::setBlankPage()
{
    resetSslInfo();
    ui->urlProgress->setValue(0);
    ui->urlText->setText(QString(""));
    ui->stopButton->hide();
    ui->reloadButton->hide();
}

void IGOWebBrowser::showNavigation(bool bShow)
{
    ui->navigationFrame->setVisible(bShow);
    mWebPage->setShowNavigation(bShow);
}

bool IGOWebBrowser::isNavigationVisible()
{
    return ui->navigationFrame->isVisible();
}

void IGOWebBrowser::setReducedMode(bool reduced)
{
    ui->HomeButton->setVisible(!reduced);
    ui->urlProgress->setVisible(!reduced);
    ui->reloadButton_reduced->setVisible(reduced);
    if( reduced )
    {
        ui->horizontalSpacer_4->changeSize(1,1, QSizePolicy::Expanding);
    }
    else
    {
        ui->horizontalSpacer_4->changeSize(1,1, QSizePolicy::Fixed);
    }

    mReducedMode = reduced;
}

void IGOWebBrowser::setupContextMenu(QWidget* window)
{
    mParentWindow = window;
    mContextMenu = new IGOWebBrowserContextMenu(window, this, ui->urlText);
    ui->urlText->installEventFilter(this);
}

void IGOWebBrowser::onInitializeFocus()
{
    // If the user had selected text in the URL bar, unselect it (just trying to match Safari's behavior)
    if (ui->urlText->hasSelectedText())
        ui->urlText->deselect();
    
    // When the user activates a tab, we need to decide what should get focus at first
    // -> if the URL bar is empty, give focus to the URL bar
    // -> if there is no content -> give focus to the URL bar
    // -> otherwise give focus to the webview
    if (ui->navigationFrame->isVisible()
        && (ui->urlText->text().isEmpty() || mWebView->url().isEmpty()))
        ui->urlText->setFocus();

    else
        mWebView->setFocus();
}

void IGOWebBrowser::paintEvent(QPaintEvent *)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void IGOWebBrowser::iconChanged()
{
    // todo
    QIcon icon = mWebView->icon();
    if (icon.isNull())
        ui->urlIcon->setIcon(DEFAULT_FAVICON);
    else
        ui->urlIcon->setIcon(icon);

    emit iconChanged(this, ui->urlIcon->icon());
}

void IGOWebBrowser::titleChanged(const QString &title)
{
    // We may be loading sub-frame data - currently the QGraphicsWebView always emits a title change event with an empty tile - then if the new content has a title
    // -> great! otherwise, we end up with no title at all. So we directly use the main frame title when we get the initial (and maybe only) empty titleChanged event
    if (title.isEmpty())
        emit titleChanged(this, page()->mainFrame()->title());
    else
        emit titleChanged(this, title);
}

void IGOWebBrowser::urlChanged(const QUrl &url)
{
    if(!ui->urlText->hasFocus())
        ui->urlText->setText(url.toEncoded());
    
    ui->browserStatus->setText(tr(LOADING));

    resetSslInfo();
}

void IGOWebBrowser::resetSslInfo()
{
    mSslInfo.secure = false;
    ui->sslIcon->setVisible(false);
}

void IGOWebBrowser::loadStarted()
{
    GetTelemetryInterface()->Metric_IGO_BROWSER_PAGELOAD();

    ui->stopButton->show();
    ui->reloadButton->hide();
}

void IGOWebBrowser::onLoadProgress(const int& progress)
{
    ui->urlProgress->setValue(progress);
    if(mWebView && mWebView->history())
        ui->backButton->setEnabled(mWebView->history()->canGoBack());
}

void IGOWebBrowser::loadFinished(bool ok)
{
    if (!ok && !mUrlSearch.isEmpty())
    {
        QString searchUrl = Origin::Services::readSetting(Origin::Services::SETTING_IGODefaultSearchURL);
        QUrl url = QUrl(searchUrl.arg(mUrlSearch));

        mWebView->loadEntryUrl(url, ui->urlText->hasFocus());

        if (ui->navigationFrame->isVisible())
        {
            // purpose of this code ? focus?
            ui->urlText->setEnabled(false);
            ui->urlText->setEnabled(true);
        }

        mUrlSearch.clear();
        return;
    }

    // set the ssl icon
    if( mSslInfo.secure )
    {
        ui->sslIcon->setVisible(true);
    }

    mUrlSearch.clear();

    ui->stopButton->hide();
    ui->reloadButton->show();
    ui->browserStatus->setText(tr(DONE));
    iconChanged();

    // reset the load status
    ui->urlProgress->setValue(0);

    ui->backButton->setEnabled(mWebView->history()->canGoBack());
    ui->forwardButton->setEnabled(mWebView->history()->canGoForward());
}

void IGOWebBrowser::statusBarMessage(const QString &msg)
{
    if (msg == "")
        return;
    else
        ui->browserStatus->setText(msg);
}

void IGOWebBrowser::linkHovered(const QString & link, const QString & title, const QString & textContent)
{
    if (link == "")
        ui->browserStatus->setText(tr(DONE));
    else
        ui->browserStatus->setText(link);

}

bool IGOWebBrowser::eventFilter(QObject *obj, QEvent *event)
{
#ifdef ORIGIN_MAC
    // Special handling of scrolling bars for Mac -> let the parent handle the event
    if (obj == ui->verticalScrollBar || obj == ui->horizontalScrollBar)
    {
        if (event->type() == QEvent::Wheel)
        {
            QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
            mWebView->updateScrollbars(wheelEvent->delta(), wheelEvent->orientation());
            
            return true;
        }
        
        else
            return QObject::eventFilter(obj, event);
    }    
#endif
    
    // handle content menu for URL text box
    if (obj == ui->urlText)
    {
        switch( event->type() )
        {
        case QEvent::MouseButtonPress:
            {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if( mouseEvent->button() == Qt::RightButton )
                {
                    mContextMenu->popup();
                    return true;
                }
            }
            break;

        case QEvent::KeyPress:
            {
                bool handled = false;
                QKeyEvent * keyEvent = dynamic_cast<QKeyEvent*>(event);
                if( keyEvent )
                    handled = handleUrlTextEvent(keyEvent);

                return handled;
            }
            break;

        case QEvent::HoverEnter:
            mWebView->setCursorShape(Qt::IBeamCursor);
            break;

        case QEvent::HoverLeave:
            mWebView->cursorLeave();
            break;

        default:
            break;
        };
        
        return QObject::eventFilter(obj, event);
    }

    // Everything below is for the GraphicsView
    switch (event->type())
    {
    case QEvent::KeyPress:
        if (handleKeyPress((QKeyEvent *)event))
            return true;
        break;

    case QEvent::Resize:
        {
            const QSize size = ((QResizeEvent*)event)->size();
            QPoint pos = ui->graphicsView->mapToGlobal(QPoint(0,0));
            if(topLevelWidget())
                pos = ui->graphicsView->mapTo(topLevelWidget(), QPoint(0,0));
            mWebView->setPos(pos);
            mWebView->resize(size);
            updateScrollbar();
        }
        break;

    case QEvent::Leave:
        {
            mWebView->cursorLeave();
        }
        break;

    case QEvent::Hide:
        {
            if (ui->navigationFrame->isVisible())
            {
                // Clear selection when selecting another tab
                ui->urlText->deselect();
            }
        }
        break;
            
    default:
        break;
    }
    
    return QObject::eventFilter(obj, event);
}

void IGOWebBrowser::contentsSizeChanged(const QSize &size)
{
    updateScrollbar();
}

void IGOWebBrowser::initialLayoutCompleted()
{
    updateScrollbar();
}

void IGOWebBrowser::updateScrollbar()
{
    QWebFrame *mainFrame = mWebView->page()->mainFrame();
    QPoint pt = mainFrame->scrollPosition();
    QSize contentSize = mainFrame->contentsSize();
    QSizeF displaySize = mWebView->size();

    int hrange = contentSize.width() - displaySize.width();
    int vrange = contentSize.height() - displaySize.height();

    int scrollWidth = ui->verticalScrollBar->width();

    if (hrange < scrollWidth)
        ui->horizontalScrollBar->hide();
    else
    {
        ui->horizontalScrollBar->show();
        ui->horizontalScrollBar->setRange(0, hrange);
        ui->horizontalScrollBar->setPageStep(displaySize.width());
        ui->horizontalScrollBar->setSliderPosition(pt.x());
    }

    if (vrange < scrollWidth)
        ui->verticalScrollBar->hide();
    else
    {
        ui->verticalScrollBar->show();
        ui->verticalScrollBar->setRange(0, vrange);
        ui->verticalScrollBar->setPageStep(displaySize.height());
        ui->verticalScrollBar->setSliderPosition(pt.y());
    }

    if (hrange >= scrollWidth && vrange >= scrollWidth)
        ui->scrollSpacer->show();
    else
        ui->scrollSpacer->hide();
}

void IGOWebBrowser::horzSliderMoved(int value)
{
    QWebFrame *mainFrame = mWebView->page()->mainFrame();
    QPoint pt = mainFrame->scrollPosition();
    pt.rx() = value;
    mainFrame->setScrollPosition(pt);
}

/*
    This slot is call when signal javaScriptWindowObjectCleared is executed.
*/
void IGOWebBrowser::onJavaScriptWindowObjectCleared()
{
    attachHelperOnWebPage();
}

void IGOWebBrowser::attachHelperOnWebPage()
{
    if (mWebView->isFrameTrusted())
    {
        mWebView->page()->mainFrame()->addToJavaScriptWindowObject("webManager",&mJSHelper);
        mWebView->page()->mainFrame()->addToJavaScriptWindowObject("storeHelper",&mJSHelper);
        mWebView->page()->mainFrame()->addToJavaScriptWindowObject("commonHelper", new CommonJsHelper(this));
    }
    else
    {
        mWebView->page()->mainFrame()->addToJavaScriptWindowObject("twitchHelper",&mJSHelper);
    }
}

void IGOWebBrowser::extractSslInformation( QNetworkReply * reply )
{
    // check for SSL configuration
    QSslConfiguration sslConfig = reply->sslConfiguration();
    if( !sslConfig.isNull() )
    {
        QSslCertificate peerCert = sslConfig.peerCertificate();
        
        // TODO: isValid() is not supported in Qt5 -> maybe we need more checks here!
        const QDateTime currentTime = QDateTime::currentDateTime();

        bool valid = !peerCert.isNull();
        valid &= !peerCert.isBlacklisted();
        valid &= currentTime >= peerCert.effectiveDate();
        valid &= currentTime < peerCert.expiryDate();
        
        if( valid )
        {
            QStringList subjectInfo_CommonName = peerCert.subjectInfo(QSslCertificate::CommonName);

            // Check that 'CommonName' string is same as the hostname
            QString hostname = mWebView->page()->mainFrame()->url().host();
            QSslCipher cipher = sslConfig.sessionCipher();
            
            bool hostNameMatch = false;
            for (int idx = 0; idx < subjectInfo_CommonName.size(); ++idx)
            {
                const QString& name = subjectInfo_CommonName.at(idx);
                hostNameMatch = compareHostName(hostname, name);
                if (hostNameMatch)
                    break;
            }
            
            if( !cipher.isNull() && hostNameMatch)
            {
                // fill in the info
                mSslInfo.secure = true;
                mSslInfo.effectiveDate = peerCert.effectiveDate();
                mSslInfo.expiryDate = peerCert.expiryDate();
                mSslInfo.hostname = hostname;
                mSslInfo.issuerOrganization = peerCert.issuerInfo(QSslCertificate::CommonName);
                if( mSslInfo.issuerOrganization.isEmpty() )
                    mSslInfo.issuerOrganization = peerCert.issuerInfo(QSslCertificate::Organization);
                mSslInfo.subjectOrganization = peerCert.subjectInfo(QSslCertificate::Organization);
                mSslInfo.encryptionBits = cipher.usedBits();
            }
        }
    }
}

bool IGOWebBrowser::compareHostName(QString hostname, QString subjectCommonName)
{
    bool match = false;
    if( subjectCommonName.contains("*") ) // has wildcard, e.g. "*.facebook.com"
    {
        QRegExp rx(subjectCommonName);
        rx.setPatternSyntax(QRegExp::Wildcard);
        match = rx.exactMatch(hostname);
    }
    else
    {
        match = (hostname == subjectCommonName);
    }

    return match;
}

void IGOWebBrowser::replyFinished(QNetworkReply* reply)
{
    // download support here is only meant for plugins and updates of plugins
    // so we restricted it to a hand full of content types and file extensions
    if (!reply)
        return;

    extractSslInformation(reply);

    QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);
    bool ok = false;
    int size = header.toInt(&ok);
    if (ok && size == 0)
        return;
    // check the content type
    QString contentType = reply->rawHeader("Content-Type");
    if(contentType == "application/x-msdos-program" || contentType == "application/octet-stream" || contentType == "application/zip" || contentType == "application/rar")
    {
         QString path = reply->url().path();
         QString basename = QFileInfo(path).fileName();

        if (basename.isEmpty())
            return;

        // check the file externsions
        if (basename.endsWith(".exe", Qt::CaseInsensitive) ||
            basename.endsWith(".msi", Qt::CaseInsensitive) ||
            basename.endsWith(".zip", Qt::CaseInsensitive) ||
            basename.endsWith(".rar", Qt::CaseInsensitive) ||
            basename.endsWith(".7z", Qt::CaseInsensitive))
        {
            using namespace Origin::UIToolkit;
            OriginWindow* messageBox = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
                NULL, OriginWindow::MsgBox, QDialogButtonBox::Yes | QDialogButtonBox::No, OriginWindow::ChromeStyle_Dialog, OriginWindow::OIG);
            messageBox->msgBox()->setup(OriginMsgBox::NoIcon, tr("ebisu_client_igo_external_browser_download_title"),
                tr("ebisu_client_igo_external_browser_download").arg(basename).arg(tr("application_name")));

            ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(rejected()), messageBox, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(messageBox, SIGNAL(finished(int)), messageBox, SLOT(close()));
            ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::Yes), SIGNAL(clicked()), messageBox, SLOT(accept()));
            ORIGIN_VERIFY_CONNECT(messageBox->button(QDialogButtonBox::No), SIGNAL(clicked()), messageBox, SLOT(reject()));

            Origin::Engine::IGOController::instance()->alert(messageBox);
            
            bool isConfirmed = messageBox->exec() == QDialog::Accepted;
            if (isConfirmed)
			{
				Origin::Engine::IGOController::instance()->putGameIntoBackground();
				static int counter = 0;
				while (Origin::Engine::IGOController::instance()->isGameInForeground() && counter<30) //max 3 seconds
				{
					Origin::Services::PlatformService::sleep(100); // give the game some time to perform the alt+tab switch
					counter++;
				}
                Origin::Services::PlatformService::asyncOpenUrlAndSwitchToApp(reply->url());
            }
        }
    }
}

void IGOWebBrowser::vertSliderMoved(int value)
{
    QWebFrame *mainFrame = mWebView->page()->mainFrame();
    QPoint pt = mainFrame->scrollPosition();
    pt.ry() = value;
    mainFrame->setScrollPosition(pt);
}

void IGOWebBrowser::backButtonClicked()
{
    ui->backButton->setEnabled(mWebView->history()->canGoBack());
    ui->forwardButton->setEnabled(mWebView->history()->canGoForward());

    mWebView->history()->back();
}

void IGOWebBrowser::forwardButtonClicked()
{
    ui->backButton->setEnabled(mWebView->history()->canGoBack());
    ui->forwardButton->setEnabled(mWebView->history()->canGoForward());

    mWebView->history()->forward();
}

void IGOWebBrowser::homeButtonClicked()
{
    mWebView->loadEntryUrl(fixURL(mDefaultURLonIGO));    
}

const QUrl IGOWebBrowser::fixURL(const QString &urlStr)
{
    QString urlString = urlStr.trimmed();

    QUrl url = QUrl(urlString);

    if (!urlString.startsWith("http://", Qt::CaseInsensitive) && !urlString.startsWith("https://", Qt::CaseInsensitive))
    {
        if (!urlString.contains(" ") && urlString.contains("."))
        {
            url = QUrl(QString("http://") + urlString);
        }
        else
        {
            QString searchUrl = Origin::Services::readSetting(Origin::Services::SETTING_IGODefaultSearchURL);
            url = QUrl(searchUrl.arg(urlString));
        }
    }

    return url;
}

void IGOWebBrowser::autoCompleteURLAndLoad(URL_SUFFIX suffix)
{
    const QString &urlStr = ui->urlText->text().trimmed();

    QUrl url = QUrl(urlStr);
    if (!urlStr.isEmpty()
        && !urlStr.startsWith("http://", Qt::CaseInsensitive)
        && !urlStr.startsWith("https://", Qt::CaseInsensitive))
    {
        const QChar lastChar = urlStr[urlStr.length() - 1];
        if (!urlStr.contains(" ")
            && lastChar.isLetterOrNumber()
            && !urlStr.endsWith(".com", Qt::CaseInsensitive)
            && !urlStr.endsWith(".net", Qt::CaseInsensitive)
            && !urlStr.endsWith(".org", Qt::CaseInsensitive))
        {
            QString suffixString;
            switch (suffix)
            {
                case DOT_NET:
                    if (!urlStr.endsWith(".net", Qt::CaseInsensitive))
                        suffixString = ".net";
                    break;
                case DOT_ORG:
                    if (!urlStr.endsWith(".org", Qt::CaseInsensitive))
                        suffixString = ".org";
                    break;
                case DOT_COM:
                default:
                    if (!urlStr.endsWith(".com", Qt::CaseInsensitive))
                        suffixString = ".com";
                    break;
            };
            
            if (!suffixString.isEmpty())
            {
                QString prefixString = urlStr.startsWith("www.", Qt::CaseInsensitive) ? "http://" : "http://www.";
                url = QUrl(prefixString + urlStr + suffixString);
                
                ui->urlText->setText(url.toString());
            
                mWebView->loadEntryUrl(url);
            }
            
            else
                newURLEntered();
        }
        else
        {
            newURLEntered();
        }
    }
    else
    {
        newURLEntered();
    }
}

void IGOWebBrowser::newURLEntered()
{
    mUrlSearch = ui->urlText->text();
    QUrl httpNoSpaceURL = fixURL(mUrlSearch);

    ui->urlText->setText(httpNoSpaceURL.toString());
    mWebView->loadEntryUrl(httpNoSpaceURL);
}

bool IGOWebBrowser::handleKeyPress(QKeyEvent *keyEvent)
{
    bool skip = false;
    switch (keyEvent->key())
    {
    // handle hot keys
    case Qt::Key_L:
        {
            if (ui->navigationFrame->isVisible())
            {
                if (keyEvent->modifiers() & Qt::ControlModifier)
                {
                    ui->urlText->selectAll();
                    ui->urlText->setFocus();
                }
            }
        }
        break;
        
    // create new window
    case Qt::Key_N:
        {
            if (ui->navigationFrame->isVisible())
            {
                if (keyEvent->modifiers() & Qt::ControlModifier)
                {
                    Origin::Engine::IGOController::instance()->createWebBrowser("");
                    skip = true;
                }
            }
        }
        break;
        
    case Qt::Key_F5:
        {
            // Right now, we allow reload() even if the navigation frame is hidden
            // if (ui->navigationFrame->isVisible())
                mWebView->reload();
        }
        break;
        
    default:
        break;
    }
    return skip;
}

bool IGOWebBrowser::handleUrlTextEvent(QKeyEvent* keyEvent)
{
    bool consume = false;
    if( keyEvent )
    {
        Qt::KeyboardModifiers mods = keyEvent->modifiers();
        int key = keyEvent->key();
        if (key == Qt::Key_Return || key == Qt::Key_Enter)
        {
            if (mods == Qt::NoModifier || mods == Qt::KeypadModifier)
            {
                GetTelemetryInterface()->Metric_IGO_BROWSER_URLSHORTCUT(QString("none").toUtf8().data());
                newURLEntered();
                consume = true;
            }
            else if (mods == Qt::ShiftModifier || mods == Qt::ShiftModifier + Qt::KeypadModifier)
            {
                GetTelemetryInterface()->Metric_IGO_BROWSER_URLSHORTCUT(QString(".net").toUtf8().data());
                autoCompleteURLAndLoad(DOT_NET);
                consume = true;
            }
            else if (mods == Qt::ControlModifier || mods == Qt::ControlModifier + Qt::KeypadModifier)
            {
                GetTelemetryInterface()->Metric_IGO_BROWSER_URLSHORTCUT(QString(".com").toUtf8().data());
                autoCompleteURLAndLoad(DOT_COM);
                consume = true;
            }
            else if (mods == Qt::ControlModifier + Qt::ShiftModifier || mods == Qt::ControlModifier + Qt::ShiftModifier + Qt::KeypadModifier)
            {
                GetTelemetryInterface()->Metric_IGO_BROWSER_URLSHORTCUT(QString(".org").toUtf8().data());
                autoCompleteURLAndLoad(DOT_ORG);
                consume = true;
            }
        }
    }

    return consume;
}

// Set/clear the focus on the URL bar of the browser
void IGOWebBrowser::onUrlBarFocus(bool giveFocus)
{
    if (ui->navigationFrame->isVisible())
    {
        if (giveFocus)
        {
            ui->urlText->selectAll();
            ui->urlText->setFocus();
        }

        else
        {
            ui->urlText->deselect();
            ui->urlText->clearFocus();
        }
    }
}

// Set focus on the webview control of the browser
void IGOWebBrowser::onWebviewFocus()
{
    mWebView->setFocus();
}

// Create a new browser
void IGOWebBrowser::onCreateWebBrowser()
{
    if (ui->navigationFrame->isVisible())
        Origin::Engine::IGOController::instance()->createWebBrowser("");
}

void IGOWebBrowser::onReloadContent()
{
    // Right now, we allow reload() even if the navigation frame is hidden
    //if (ui->navigationFrame->isVisible())
        mWebView->reload();
}

} // Client
} // Origin

