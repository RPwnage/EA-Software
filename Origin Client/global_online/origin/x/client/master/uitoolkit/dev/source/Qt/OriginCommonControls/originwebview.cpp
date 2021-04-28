#include "originwebview.h"
#include "originspinner.h"

#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
// Overriding the cursor
#include <QApplication>
#include <QUrl>
#include <QStringList>
#include <QDesktopWidget>
#include "OriginCommonUIUtils.h"

static const QSize WINDOWMARGIN = QSize(16,45);

namespace Origin {
    namespace UIToolkit {
OriginWebView::OriginWebView(QWidget* parent)
: QStackedWidget(parent)
, mWebView(NULL)
, mGraphicsView(NULL)
, mGraphicsWebpage(NULL)
, mSpinner(NULL)
, mWindowLoadingSize(QSize(450, 201))
, mIsSelectable(true)
, mUsesOriginScrollbars(true)
, mIsContentSize(true)
, mChangeLoadSizeAfterInitLoad(false)
, mInitalLoadComplete(false)
, mShowSpinnerAfterInitLoad(true)
, mScaleSizeOnContentsSizeChanged(false)
, mShowAfterInitialLayout(false)
, mOverridingCursor(false)
{
    setOriginElementName("OriginWebView");
    m_children.append(this);

    setWebview(new QWebView());
    mWebView->settings()->setAttribute(QWebSettings::PluginsEnabled, true);
    addWidget(mWebView);

#ifdef Q_OS_MAC
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
#endif
    
    if(topLevelWidget())
        topLevelWidget()->move((QApplication::desktop()->availableGeometry(topLevelWidget()).center() - topLevelWidget()->rect().center()) / 2);
}


OriginWebView::~OriginWebView()
{
    if(mSpinner)
    {
        removeWidget(mSpinner);
        delete mSpinner;
        mSpinner = NULL;
    }

    removeWebview();

    removeElementFromChildList(this);
}


void OriginWebView::setHasSpinner(const bool& spinner)
{
    if(spinner)
    {
        mSpinner = new OriginSpinner();
        addWidget(mSpinner);
    }
    else
    {
        if(mWebView)
            setCurrentWidget(mWebView);
        removeWidget(mSpinner);
        delete mSpinner;
        mSpinner = NULL;
    }
}


void OriginWebView::showLoading()
{
    if(mIsContentSize && mWebView)
    {
        mWebView->page()->setPreferredContentsSize(mPreferredContentsSize.isEmpty() ? QSize(100,100) : mPreferredContentsSize);
        if(mInitalLoadComplete == false 
            || (mInitalLoadComplete && mChangeLoadSizeAfterInitLoad == false))
        {
            setMaximumSize(mWindowLoadingSize);
            resize(mWindowLoadingSize);
            if(topLevelWidget())
            {
                topLevelWidget()->resize(size()+WINDOWMARGIN);
            }
        }
    }
    
#if !defined(Q_OS_MAC)
	if (!mOverridingCursor)
    {
		//QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
		mOverridingCursor = true;
    }
#endif

    // Do we have a spinner to show?
    if(mSpinner && 
        // Is it set to not show the spinner ever again after the initial load?
        (mShowSpinnerAfterInitLoad || (mShowSpinnerAfterInitLoad == false && mInitalLoadComplete == false)))
    {
        mSpinner->startSpinner();
        setCurrentWidget(mSpinner);
    }
    if(mWebView && mIsContentSize)
        connect(mWebView->page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)), this, SLOT(updateSize()), Qt::QueuedConnection);
}


void OriginWebView::showWebpage(bool ok)
{
    if(mWebView && mWebView->page()->mainFrame()->url().toString() != "")
        mInitalLoadComplete = true;

	if (mOverridingCursor)
    {
		QApplication::restoreOverrideCursor();
		mOverridingCursor = false;
    }

    setCurrentWidget(mWebView);
    if(mSpinner)
    {
        mSpinner->stopSpinner();
    }
    if(ok)
        updateSize();
    else
    {
        // if the page loading failed and it's content size is smaller than the spinner, set the page to the spinner size
        // BUT only do it when the window is setup to resize to show the entire page - otherwise just leave it alone
        // (since even the updateSize() is skipped in the normal/non-failing case / we want to keep the origin window size)
		if (mIsContentSize && mSpinner)
		{
			const QSize contentSize = mWebView->page()->mainFrame()->contentsSize();
			if (contentSize.width() < mWindowLoadingSize.width() && contentSize.height() < mWindowLoadingSize.height())
			{
                mWebView->page()->setPreferredContentsSize(mPreferredContentsSize.isEmpty() ? mWindowLoadingSize : mPreferredContentsSize);
				setMaximumSize(mWindowLoadingSize);
				resize(mWindowLoadingSize);
				if(topLevelWidget())
				{
					topLevelWidget()->resize(size()+WINDOWMARGIN);
				}
			}
			else // or just use the actual current content size - otherwise we end up with a small white square(webview)
			{
				updateSize();
			}
		}
    }
}
        
        
void OriginWebView::showGraphicsWebpage(bool ok)
{
    if(mGraphicsWebpage && mGraphicsWebpage->mainFrame()->url().toString() != "")
        mInitalLoadComplete = true;
    
    QApplication::restoreOverrideCursor();
    setCurrentWidget(mGraphicsView);
    if(mSpinner)
    {
        mSpinner->stopSpinner();
    }
}


void OriginWebView::updateSize()
{
    // Only resize it to the parent if the scroll bars are off.
    // Only resize if the page is page is loaded.
    // EBIBUGS-27286: Don't resize if we are minimized.
    QWidget* topWidget = topLevelWidget();
    bool isMinimized = false;
    if(topWidget && topWidget->isMinimized())
    {
        isMinimized = true;
        // Mac is dumb and will tell us we are minimized while we are restoring.
        if(topWidget->isActiveWindow())
        {
            isMinimized = false;
        }
    }
    if(mIsContentSize && currentWidget() == mWebView && isMinimized == false)
    {
        // Set the preferred size so that qt will ignore it and finally give us the 
        // actual size of the content
        const QSize contentSize = mWebView->page()->mainFrame()->contentsSize();
        if(contentSize == QSize(0,0))
            return;

        if(mScaleSizeOnContentsSizeChanged)
        {
            // This is needed for the promo browser
            int height = mPreferredContentsSize.height() == -1 ? 100 : mPreferredContentsSize.height();
            mWebView->page()->setPreferredContentsSize(QSize(contentSize.width(), height));
        }
        
#if defined(Q_OS_MAC)
        else
        {
            // Try to limit some of the recurring resizing processing!
            const QSize currentSize = size();
            if (currentSize == contentSize)
                return;
        }
#endif
        
        mWebView->resize(contentSize);
        setMaximumSize(contentSize);
        resize(contentSize);

        if(topWidget)
        {
            topWidget->resize(contentSize+WINDOWMARGIN);
            
#if defined(Q_OS_MAC)
            // Note for Mac:
            // The mask we currently apply for Mac is really useless, EXCEPT that it forces a reset of the Cocoa pre-computed
            // window shadow, which is a good thing with all the crazy resizing going on
            
            // Currently in Qt5 the masks only applied to top level widgets (ie windows), so I could also move this call from here and into
            // originwindow.cpp!
            OriginCommonUIUtils::DrawRoundCorner(topWidget, 6, true);
#endif
        }
        
#if !defined(Q_OS_MAC)
        OriginCommonUIUtils::DrawRoundCorner(this, 6, true);
#endif

        emit sizeUpdated();
    }
}


void OriginWebView::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
    if (mWebView)
        mWebView->settings()->setUserStyleSheetUrl(QUrl(""));
}


QString& OriginWebView::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = 
            getStyleSheet(true, mIsSelectable == false, mUsesOriginScrollbars);
    }

    if (mWebView)
        mWebView->settings()->setUserStyleSheetUrl(QUrl("data:text/css;charset=utf-8;base64," +
                                                    mPrivateStyleSheet.toUtf8().toBase64()));

    return mPrivateStyleSheet;
}


QString OriginWebView::getErrorStyleSheet(const bool& alignCenter)
{
    // Load the error css
    const QString alignment = alignCenter ? "center" : "left";
    const QString styleSheet = loadPrivateStyleSheet(QString(":/OriginCommonControls/OriginWebView/stylePageError_%1.css").arg(alignment), "OriginWebView")
                        .arg(alignment)
                        .arg(QSStoCSS("OriginWebView", "OriginLabel", "DialogTitle", "styleDialogTitle.qss"))
                        .arg(QSStoCSS("OriginWebView", "OriginLabel", "DialogDescription", "styleDialogDescription.qss"))
                        .arg(QSStoCSS("OriginWebView", "OriginPushButton"));
    return styleSheet;
}


QString OriginWebView::getStyleSheet(const bool& hasFocusOutline, const bool& isSelectable, const bool& useOriginScrollbars)
{
    QString styleSheet = "";

    if(useOriginScrollbars)
    {
        styleSheet += loadPrivateStyleSheet(":/OriginCommonControls/OriginWebView/styleScrollbar.css",
            "OriginWebView");
    }

    if(hasFocusOutline)
    {
        styleSheet += loadPrivateStyleSheet(":/OriginCommonControls/OriginWebView/styleFocusOutline.css",
            "OriginWebView");
    }

    if(isSelectable == false)
    {
        styleSheet += loadPrivateStyleSheet(":/OriginCommonControls/OriginWebView/styleSelectable.css",
            "OriginWebView");
    }

    return styleSheet;
}

const QString OriginWebView::QSStoCSS(const QString& thisElementName, const QString& elementName, 
                                     const QString& elementSubName, const QString& qssfile)
{
    QString qssFile = loadPrivateStyleSheet(QString(":/OriginCommonControls/%1/%2").arg(elementName).arg(qssfile),
                                            elementName,
                                            elementSubName);

    const QString qualifiedElementName("Origin--UIToolkit--" + elementName);

    // Things that need to be switched for it to read as a CSS file
    qssFile.replace(qualifiedElementName, QString(".%1%2").arg(elementName).arg(elementSubName));
    qssFile.replace(QString("/.%1%2").arg(qualifiedElementName).arg(elementSubName), "/"+elementName);
    qssFile.replace("border-image", "-webkit-border-image");
    qssFile.replace("url(:/", "url('qrc:");
    qssFile.replace("png)", "png')");
    qssFile.replace("pressed", "active");
    qssFile.replace("\"", "");

    return qssFile;
}


bool OriginWebView::event(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::ParentChange:
        if(topLevelWidget())
        {
            // We need to resize after we come back from minimize. We have to keep track of the top level
            // so we can catch the WindowActivate event
            topLevelWidget()->installEventFilter(this);
        }
        break;
    default:
        break;
    }
    OriginUIBase::event(event);
    return QStackedWidget::event(event);
}


bool OriginWebView::eventFilter(QObject* obj, QEvent* event)
{
    switch(event->type())
    {
        // IGO cursor events
    case QEvent::CursorChange:
        if(obj == mWebView)
            emit cursorChanged(mWebView->cursor().shape());
        break;
    case QEvent::Leave:
        if(obj == mWebView)
            emit cursorChanged(Qt::ArrowCursor);
        break;
    case QEvent::WindowActivate:
        if(topLevelWidget() && obj == topLevelWidget())
        {
            updateSize();
        }
        break;
    default:
        break;
    }
    return QStackedWidget::eventFilter(obj, event);
}


void OriginWebView::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
    QStackedWidget::paintEvent(event);
}


const Qt::ScrollBarPolicy OriginWebView::hScrollBarPolicy(void) const
{
    if(mWebView && mWebView->page() && mWebView->page()->mainFrame())
        return mWebView->page()->mainFrame()->scrollBarPolicy(Qt::Horizontal);
    return Qt::ScrollBarAsNeeded;
}


void OriginWebView::setHScrollPolicy(const Qt::ScrollBarPolicy& policy)
{
    if(mWebView && mWebView->page() && mWebView->page()->mainFrame())
        mWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Horizontal, policy);
    setStyleSheetDirty();
}


const Qt::ScrollBarPolicy OriginWebView::vScrollBarPolicy(void) const
{
    if(mWebView && mWebView->page() && mWebView->page()->mainFrame())
        return mWebView->page()->mainFrame()->scrollBarPolicy(Qt::Vertical);
    return Qt::ScrollBarAsNeeded;
}


void OriginWebView::setVScrollPolicy(const Qt::ScrollBarPolicy& policy)
{
    if(mWebView && mWebView->page() && mWebView->page()->mainFrame())
        mWebView->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, policy);
    setStyleSheetDirty();
}


void OriginWebView::setIsSelectable(const bool& selectable)
{
    mIsSelectable = selectable;
    setStyleSheetDirty();
}


void OriginWebView::setUsesOriginScrollbars(const bool& usesOriginScrollbars)
{
    mUsesOriginScrollbars = usesOriginScrollbars;
    setStyleSheetDirty();
}


void OriginWebView::setStyleSheetDirty()
{
    prepForStyleReload();
    setStyleSheet(getElementStyleSheet());
}


void OriginWebView::setWebview(QWebView* webview)
{
    if(webview)
    {
        removeWebview();

        mWebView = webview;
        addWidget(mWebView);
        mWebView->installEventFilter(this);

        connect(mWebView, SIGNAL(loadStarted()), this, SLOT(showLoading()));

        if (mShowAfterInitialLayout)
        {
            connect(mWebView->page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(showWebpage()));
        }

        // EBIBUGS-27124: Always connect loadFinished, initialLayoutCompleted may never
        // be emitted for some error responses
        connect(mWebView, SIGNAL(loadFinished(bool)), this, SLOT(showWebpage(bool)));
    }
}

        
void OriginWebView::removeWebview()
{
    if(mWebView)
    {
        // The mouse cursor will appear in a waiting state if a transaction
        // is still in progress during deletion (such as a page reloading
        // while logging out on a slow connection).
        mWebView->stop();
        
        removeWidget(mWebView);
        disconnect(mWebView->page()->mainFrame(), SIGNAL(contentsSizeChanged(const QSize&)), this, SLOT(updateSize()));
        disconnect(mWebView, SIGNAL(loadStarted()), this, SLOT(showLoading()));
        
        if (mShowAfterInitialLayout)
        {
            disconnect(mWebView->page()->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(showWebpage()));
        }

        // EBIBUGS-27124: Always connect loadFinished, initialLayoutCompleted may never
        // be emitted for some error responses
        disconnect(mWebView, SIGNAL(loadFinished(bool)), this, SLOT(showWebpage(bool)));
        
        mWebView->deleteLater();
        mWebView = NULL;
    }
}
        
        
void OriginWebView::setGraphicsWebview(QWidget* view, QWebPage* page)
{
    if(view)
    {
        removeWebview();
        mGraphicsView = view;
        addWidget(mGraphicsView);
        mGraphicsWebpage = page;
        
        connect(mGraphicsWebpage, SIGNAL(loadStarted()), this, SLOT(showLoading()));

        if (mShowAfterInitialLayout)
        {
            connect(mGraphicsWebpage->mainFrame(), SIGNAL(initialLayoutCompleted()), this, SLOT(showGraphicsWebpage()));
        }
        else
        {
            connect(mGraphicsWebpage, SIGNAL(loadFinished(bool)), this, SLOT(showGraphicsWebpage(bool)));
        }
    }
}
}
}
