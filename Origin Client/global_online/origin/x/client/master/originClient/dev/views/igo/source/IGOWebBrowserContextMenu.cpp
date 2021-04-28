#include <QCursor>
#include <QApplication>
#include <QClipboard>
#include <QGraphicsProxyWidget>
#include <QLineEdit>

#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"

#include "IGOWebBrowserContextMenu.h"
#include "CustomQMenu.h"
#include "IGOWebBrowser.h"
#include "OriginTabWidget.h"

namespace Origin
{
namespace Client
{

IGOWebBrowserContextMenu::IGOWebBrowserContextMenu(QWidget* window, IGOWebBrowser *browser, QLineEdit* urlText /*= NULL*/)
    : mNativeMenu(NULL)
    , mWindow(window)
    , mBrowser(browser)
    , mActionLinkTab(NULL)
    , mActionLinkWindow(NULL)
    , mActionCopyLinkAddress(NULL)
    , mActionCopy(NULL)
    , mActionCopyUrlText(NULL)
    , mActionPaste(NULL)
    , mActionPasteUrlText(NULL)
    , mUrlText(urlText)
{
    bool inIGO = Origin::Engine::IGOController::instance()->isActive();
    mNativeMenu = new CustomQMenu(inIGO ? IGOScope : ClientScope);
    mActionLinkWindow = new QAction(tr("ebisu_client_igo_browser_link_in_new_window"), this);
    mActionLinkTab = new QAction(tr("ebisu_client_igo_browser_link_in_new_tab"), this);
    mActionCopyLinkAddress = new QAction(tr("ebisu_client_igo_browser_copy_link_address"), this);

    if (inIGO)
    {
        if (Origin::Engine::IGOController::instance()->igowm()->createContextMenu(window, mNativeMenu, this, true))
        {
            initMenuActions();

            ORIGIN_VERIFY_CONNECT(mNativeMenu, SIGNAL(aboutToHide()), this, SLOT(hiding()));
            ORIGIN_VERIFY_CONNECT(browser->browser()->page(), SIGNAL(linkHovered(QString,QString,QString)), this, SLOT(linkHovered(QString,QString,QString)));
        }
    }

}

void IGOWebBrowserContextMenu::openLinkInNewWindow()
{
    if( !mCurrentLinkHovered.isEmpty() )
    {
        // save link so it won't get cleared
        mLinkHovered = mCurrentLinkHovered;
        
        emit Engine::IGOController::instance()->showBrowser(mLinkHovered, false);
    }
}

void IGOWebBrowserContextMenu::initMenuActions()
{
    ORIGIN_VERIFY_CONNECT(mActionLinkTab, SIGNAL(triggered()), this, SLOT( openLinkInNewTab()));

    ORIGIN_VERIFY_CONNECT(mActionLinkWindow, SIGNAL(triggered()), this, SLOT( openLinkInNewWindow()));

    ORIGIN_VERIFY_CONNECT(mActionCopyLinkAddress, SIGNAL(triggered()), this, SLOT( copyLinkAddress()));

    mActionCopy = mBrowser->browser()->pageAction(QWebPage::Copy);

    mActionPaste = mBrowser->browser()->pageAction(QWebPage::Paste);

    if( mUrlText )
    {
        mActionCopyUrlText = new QAction(NULL);
        ORIGIN_VERIFY_CONNECT(mActionCopyUrlText, SIGNAL(triggered()), this, SLOT(onCopyUrlText()));

        mActionPasteUrlText = new QAction(NULL);
        ORIGIN_VERIFY_CONNECT(mActionPasteUrlText, SIGNAL(triggered()), this, SLOT(onPasteUrlText()));
    }
}

void IGOWebBrowserContextMenu::onCopyUrlText()
{
    QApplication::clipboard()->setText(mUrlText->selectedText());
}

void IGOWebBrowserContextMenu::onPasteUrlText()
{
    if(!QApplication::clipboard()->text().isEmpty())
    {
        mUrlText->insert(QApplication::clipboard()->text());
    }
}

void IGOWebBrowserContextMenu::openLinkInNewTab()
{
    if(!mCurrentLinkHovered.isEmpty())
    {
        // save link so it won't get cleared
        mLinkHovered = mCurrentLinkHovered;
        
        emit Origin::Engine::IGOController::instance()->showBrowser(mLinkHovered, true);
    }
}

void IGOWebBrowserContextMenu::copyLinkAddress()
{
    if(!mCurrentLinkHovered.isEmpty())
    {
        QApplication::clipboard()->setText(mCurrentLinkHovered);
    }
}

void IGOWebBrowserContextMenu::linkHovered(const QString & link, const QString & title, const QString & textContent)
{
    if(mNativeMenu && !mNativeMenu->isVisible())
        mCurrentLinkHovered = link;
}

IGOWebBrowserContextMenu::~IGOWebBrowserContextMenu()
{
    delete mActionCopyUrlText;
    delete mActionPasteUrlText;

    delete mNativeMenu;
}

void IGOWebBrowserContextMenu::popup()
{
    if (mNativeMenu && Origin::Engine::IGOController::instance()->isActive())
    {
        bool actionsExist = false;
        mNativeMenu->clear();
        
        if(!mCurrentLinkHovered.isEmpty())
        {
            mNativeMenu->addAction(mActionLinkWindow);
            mNativeMenu->addAction(mActionLinkTab);
            bool canOpenInNewTab = mBrowser->tabWidget()->allowAddTab() && !mBrowser->reducedMode();
            mActionLinkTab->setEnabled(canOpenInNewTab);
            mNativeMenu->addAction(mActionCopyLinkAddress);
            actionsExist = true;
        }

        if (mUrlText && mUrlText->hasFocus())
        {
            if (mUrlText->hasSelectedText())
            {
                mActionCopyUrlText->setText(tr("ebisu_client_menu_copy"));
                mNativeMenu->addAction(mActionCopyUrlText);
                actionsExist = true;
            }
        }
        
        else
        if(!mBrowser->browser()->page()->selectedText().isEmpty())
        {
            mActionCopy->setText(tr("ebisu_client_menu_copy"));
            mNativeMenu->addAction(mActionCopy);
            actionsExist = true;
        }
        
        if(!QApplication::clipboard()->text().isEmpty())
        {
            if( mUrlText )
            {
                mActionPasteUrlText->setText(tr("ebisu_client_menu_paste"));
                mNativeMenu->addAction(mActionPasteUrlText);
            }
            else
            {
                mActionPaste->setText(tr("ebisu_client_menu_paste"));
                mNativeMenu->addAction(mActionPaste);
            }

            actionsExist = true;
        }
        
        if(actionsExist)
        {
            Origin::Engine::IIGOWindowManager::WindowProperties cmProperties;
            if (Origin::Engine::IGOController::instance()->igowm()->showContextMenu(mWindow, mNativeMenu, &cmProperties))
                mNativeMenu->popup(cmProperties.position());
        }
    }
}

void IGOWebBrowserContextMenu::onFocusChanged(QWidget* window, bool hasFocus)
{
    Q_UNUSED(window);
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        if (mNativeMenu)
            mNativeMenu->hide();

        mCurrentLinkHovered.clear();
    }
}

void IGOWebBrowserContextMenu::onDeactivate()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        if (mNativeMenu)
            mNativeMenu->hide();

        mCurrentLinkHovered.clear();
    }
}

void IGOWebBrowserContextMenu::hiding()
{
    if (Origin::Engine::IGOController::instance()->isActive())
    {
        Origin::Engine::IGOController::instance()->igowm()->hideContextMenu(mWindow);
    }
    
    // Give back focus to our url text if we did show the popup on top of it
    if (mUrlText)
        mUrlText->setFocus();
}
    
} // Client
} // Origin
