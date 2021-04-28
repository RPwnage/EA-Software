#include "NativeBehaviorManager.h"

#include <QPoint>
#include <QtWebKitWidgets/QWebPage>
#include <QWidget>
#include <QtWebKitWidgets/QWebView>
#include <QtWebKitWidgets/QWebFrame>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QEvent>
#include <QMenu>
#include <QtWebKit/QWebElement>
#include <QApplication>
#include <QtWebKit/QWebSettings>
#include <QClipboard>

#include "services/debug/DebugService.h"
#include "services/qt/QtUtil.h"
#include "services/settings/SettingsManager.h"
#include "originwebview.h"

namespace
{
    using namespace Origin;

    const QString OverridenCursorAttribute("origin-cursor-override");

    bool webDebugEnabled()
    {
        return Services::readSetting(Services::SETTING_WebDebugEnabled, Services::Session::SessionRef());
    }

#if !defined(ORIGIN_MAC)
    QWebElement bodyElement(QWebPage *page)
    {
        QWebElement docElement = page->mainFrame()->documentElement();
        return docElement.findFirst("body");
    }
#endif
}

namespace Origin
{
namespace Client
{

QList<QPointer<NativeBehaviorManager> > NativeBehaviorManager::sInstances;
    
void NativeBehaviorManager::applyNativeBehavior(QWidget *viewWidget, QWebPage *page)
{
    sInstances.append(new NativeBehaviorManager(viewWidget, page));
}

void NativeBehaviorManager::applyNativeBehavior(QWebView *webView)
{
    sInstances.append(new NativeBehaviorManager(webView, webView->page()));
}

void NativeBehaviorManager::shutdown()
{
    foreach(QPointer<NativeBehaviorManager> nbm, sInstances)
    {
        if(!nbm.isNull())
        {
            // Don't delete the NativeBehaviorManager instances here.  The web views will clean them up.
            nbm->removeNativeBehavior();
        }
    }
    sInstances.clear();
}
    
NativeBehaviorManager::NativeBehaviorManager(QWidget *viewWidget, QWebPage *page) :
    QObject(page),
    mViewWidget(viewWidget),
    mPage(page)
{
    // Install an event filter on the view widget
    viewWidget->installEventFilter(this);
            
    // Inject scrollbar CSS
    mPage->settings()->setUserStyleSheetUrl(QUrl("data:text/css;charset=utf-8;base64," +
                UIToolkit::OriginWebView::getStyleSheet(true, true).toUtf8().toBase64()));

// Busy cursors are rarely used on Mac
#ifndef ORIGIN_MAC
    // Show a busy cursor during load
    ORIGIN_VERIFY_CONNECT(mPage, SIGNAL(loadStarted()), this, SLOT(showBusyCursor()));
    ORIGIN_VERIFY_CONNECT(mPage, SIGNAL(loadFinished(bool)), this, SLOT(restoreCursor(bool)));
#endif
    
    // Enable web debug if it's configured to be on
    if (webDebugEnabled())
    {
        mPage->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    }

    // Update our menu actions
    updateMenuActions();
}
    
void NativeBehaviorManager::removeNativeBehavior()
{
#ifndef ORIGIN_MAC
    ORIGIN_VERIFY_DISCONNECT(mPage, SIGNAL(loadStarted()), this, SLOT(showBusyCursor()));
    ORIGIN_VERIFY_DISCONNECT(mPage, SIGNAL(loadFinished(bool)), this, SLOT(restoreCursor(bool)));
#endif
}

void NativeBehaviorManager::updateMenuActions()
{
    // Iterate over all the possibel actions
    for(int idx = 0; idx < QWebPage::WebActionCount; idx++)
    {
        QWebPage::WebAction webAction = static_cast<QWebPage::WebAction>(idx);

        QAction *action = mPage->action(webAction);
        
        if (!action)
        {
            continue;
        }

        // Update the text for actions we know, disable the ones we don't
        switch(webAction)
        {
        case QWebPage::Undo:
            action->setText(tr("ebisu_client_menu_undo"));
            break;
        case QWebPage::Cut:
            action->setText(tr("ebisu_client_menu_cut"));
            break;
        case QWebPage::Copy:
            action->setText(tr("ebisu_client_menu_copy"));
            break;
        case QWebPage::Paste:
            action->setText(tr("ebisu_client_menu_paste"));
            break;
        case QWebPage::InspectElement:
            action->setVisible(webDebugEnabled());
            break;
        case QWebPage::CopyLinkToClipboard:
            action->setText(tr("ebisu_client_copy_link_adderss"));
            // This is conditionally enabled depending if we're over an http:// or https:// link
            action->setVisible(false);
            break;
        default:
            // I don't know you and I don't like you
            action->setVisible(false);
        }
    }

}
    
bool NativeBehaviorManager::eventFilter(QObject *, QEvent *event)
{
    // Filter out backspace as that makes us navigate back
	if (event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        return filterKeyPressEvent(keyEvent);
	}
    else if (event->type() == QEvent::ContextMenu)
    {
		QContextMenuEvent *menuEvent = static_cast<QContextMenuEvent *>(event);
        handleContextMenuEvent(menuEvent);
        return true;
    }

    return false;
}
    
bool NativeBehaviorManager::filterKeyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Backspace)
    {
        // If an input element has focus allow the event
        const QString whitelistSelector("input:focus,textarea:focus,*[contenteditable]:focus");

        QWebElement document = mPage->currentFrame()->documentElement();
        QWebElementCollection elements = document.findAll(whitelistSelector);

        if (elements.count() > 0)
        {
            return false;
        }

        // Event, you shall not pass!
        return true;
    }
    else if (webDebugEnabled() &&
            event->key() == Qt::Key_U &&
            (0 != (event->modifiers() & Qt::ControlModifier)))
    {
        // Trick to copy the URL to the clipboard
        // Not really native behavior but neat trick for QA
        QApplication::clipboard()->setText(mPage->mainFrame()->url().toString());
    }

    return false;
}
    
void NativeBehaviorManager::handleContextMenuEvent(QContextMenuEvent *event)
{
    // See if the page wants the event first
    if (mPage->swallowContextMenuEvent(event))
    {
        return;
    }
    
    // Find the element under the cursor
    QWebHitTestResult hitResult = mPage->currentFrame()->hitTestContent(event->pos());

    // Determine if this is a link with origin-allow-copy-link
    const QUrl linkUrl = hitResult.linkUrl();
    const bool allowLinkCopy = linkUrl.isValid() && hitResult.linkElement().hasClass("origin-allow-copy-link");

    // Make the Copy Link Address action visible based on the above
    mPage->action(QWebPage::CopyLinkToClipboard)->setVisible(allowLinkCopy);

    // See what WebKit wants to show
    mPage->updatePositionDependentActions(event->pos());
    // EBIBUGS-27776: updatePositionDependentActions reverts all the strings to the description of the action.
    updateMenuActions();
    QMenu *standardMenu = mPage->createStandardContextMenu();

    // See if there's any useful actions
    QList<QAction *> actions = standardMenu->actions();

    for(QList<QAction *>::ConstIterator it = actions.constBegin();
        it != actions.constEnd();
        it++)
    {
        QAction *action(*it);

        if (action->isVisible() && action->isEnabled() && !action->isSeparator())
        {
            // Found something! Show the menu

            // Delete the menu once it closes
            ORIGIN_VERIFY_CONNECT(standardMenu, SIGNAL(aboutToHide()),
                standardMenu, SLOT(deleteLater()));

            // Draw our rounded corners
            Services::DrawRoundCornerForMenu(standardMenu, 6);

            // Show for real
            standardMenu->popup(event->globalPos());

            return;
        }
    }

    // Didn't find anything; don't show an empty menu
    delete standardMenu;
    return;
}

void NativeBehaviorManager::showBusyCursor()
{
#if !defined(ORIGIN_MAC)
    QWebElement body = bodyElement(mPage);

    if (!body.isNull())
    {
        // Set the cursor so when Qt recalculates the cursor based on which element
        // the cursor is over it doesn't blow away our busy cursor
        body.setStyleProperty("cursor", "wait !important");
        // Mark the fact we overrode it programatically
        body.setAttribute(OverridenCursorAttribute, "true");
    }

    if (mViewWidget)
    {
        mViewWidget->setCursor(Qt::WaitCursor);
    }
#endif
}

void NativeBehaviorManager::restoreCursor(bool ok)
{
#if !defined(ORIGIN_MAC)
    QWebElement body = bodyElement(mPage);

    // In the event of a successful load the DOM will have changed under us
    // so make sure our attribute is there before we go mucking with <body>s
    // style
    if (!body.isNull() && body.hasAttribute(OverridenCursorAttribute))
    {
        body.setStyleProperty("cursor", "default !important");
        body.removeAttribute(OverridenCursorAttribute);
    }
    
    if (mViewWidget)
    {
        mViewWidget->setCursor(Qt::ArrowCursor);
    }
#endif
}

}
}
