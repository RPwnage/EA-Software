#include "originwindow.h"
#include <QDesktopWidget>
#include <QSizeGrip>
#include <QKeyEvent>
#include "ui_originchromebase.h"
#include "originwindowmanager.h"
#include "originbanner.h"
#include "originwindowtemplatebase.h"
#include "originwindowtemplatescrollable.h"
#include "originwindowtemplatemsgbox.h"
#include "originbuttonbox.h"
#include "originwebview.h"
#include "originmsgbox.h"
#include "originscrollablemsgbox.h"

#include "origintitlebarbase.h"
#if defined(Q_OS_WIN)
#include <windows.h>
#include "origintitlebarwin.h"
#elif defined(Q_OS_MAC)
#include "origintitlebarosx.h"
#endif

#include "platform.h"

//#define ENABLE_FULLSCREEN


namespace Origin {
namespace UIToolkit {
QString OriginWindow::mPrivateStyleSheet = QString("");

#ifdef Q_OS_WIN
static const int CORNER_RADIUS = 4;
#endif

OriginWindow::OriginWindow(const TitlebarItems& iconFlags,
                         QWidget* content,
                         const ContentTemplate& contentTemplate,
                         QDialogButtonBox::StandardButtons buttons,
                         ChromeStyle chromeStyle, const WindowContext& context)
: OriginChromeBase(NULL)
, mTemplate(NULL)
, mTitlebar(NULL)
, mManager(NULL)
, mIgnoreEscPress(false)
, mContentTemplate(contentTemplate)
, mTitlebarItems(iconFlags)
, mWindowContext(context)
, mCreationTime(QTime::currentTime())
{
    uiBase->setupUi(this);

    pluginCustomUi();
    setObjectName("OriginWindow");
    setOriginElementName("OriginWindow");
    m_children.append(this);

#ifdef Q_OS_MAC
    if (chromeStyle == OriginWindow::ChromeStyle_Window)
    {
        // For Mac, we want a little margin (only for regular windows as the dialogs have a shadow box we can use for the same purpose) to
        // help with resizing cursors
        setContentsMargins(3, 3, 3, 3);
    }
    
    else
    {
        // There is currently a Qt5 rendering bug associated to translucent window - a margins of 0, 0, 0, 0 would cause the background to not be cleanup properly.
        // But with a little magic minimum margin, the rendering occurs properly (the problem was very visible on the Client side Chat window)
        setContentsMargins(1, 1, 1, 1);
    }
#endif
    
    switch(mWindowContext)
    {
    case Normal:
        if(mManager == NULL)
            mManager = new OriginWindowManager(this, OriginWindowManager::Movable, borderWidths(), uiBase->titlebar);
        break;
    case OIG:
        configForOIG();
        break;
    default:
        break;
    }

    setChromeStyle(chromeStyle);
    setupUiFunctionality(iconFlags);

    switch(mContentTemplate)
    {
    case WebContent:
        {
            setObjectName("OriginWindow_WebView");
            if(!content)
            {
                content = new OriginWebView();
            }
        }
        break;

    case Scrollable:
        {
            setObjectName("OriginWindow_Scrollable");
            mTemplate = new OriginWindowTemplateScrollable(this);
            dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->addButtons(buttons);
            OriginChromeBase::setContent(mTemplate);
        }
        break;

    case MsgBox:
        {
            setObjectName("OriginWindow_MsgBox");
            mTemplate = new OriginWindowTemplateMsgBox(this);
            dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->addButtons(buttons);
            OriginChromeBase::setContent(mTemplate);
        }
        break;
		
	case ScrollableMsgBox:
        {
            setObjectName("OriginWindow_ScrollableMsgBox");
            OriginScrollableMsgBox* scrollableMsgBox = new OriginScrollableMsgBox();
            scrollableMsgBox->addButtons(buttons);
            OriginChromeBase::setContent(scrollableMsgBox);
        }
        break;
        
    case Custom:
    default:
        break;
    }

    setContent(content);
    
    setMouseTracking(true);
    
    adjustSize();
    
#ifdef Q_OS_MAC
    // NOTE: LEAVE THIS AT THE END OF THE METHOD
    QMargins margins = contentsMargins();
    if (margins.top() || margins.left() || margins.bottom() || margins.right())
    ToolkitPlatform::MakeWidgetTransparencyClickable(winId());
#endif

    // EBIBUGS-27349: It's important that this is a Qt::QueuedConnection for the right click message system menu.
    // The right click menu has it's only funky event loop. Takes over the window event without a point of return it seems.
    connect(this, SIGNAL(closeWindow()), this, SIGNAL(rejected()), Qt::QueuedConnection);
}


OriginWindow::~OriginWindow() 
{
    if(mManager)
    {
        delete mManager;
        mManager = NULL;
    }

    removeElementFromChildList(this);
}


void OriginWindow::pluginCustomUi()
{
#if defined(Q_OS_WIN)
    mTitlebar = new OriginTitlebarWin(uiBase->titlebar, this);
    setMenuBarVisible(false);
#elif defined(Q_OS_MAC)
    mTitlebar = new OriginTitlebarOSX(uiBase->titlebar, this);
#endif
    connect(mTitlebar, SIGNAL(rejectWindow()), this, SIGNAL(closeWindow()));

    uiBase->outerFrame->setProperty("style","norm");
    uiBase->outerFrame->setStyle(QApplication::style());
}


void OriginWindow::setupUiFunctionality(const TitlebarItems& flags)
{
	setTitlebarItems(flags);

    if(mTitlebarItems & Minimize)
    {
        setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);
        if(mTitlebar)
            connect(mTitlebar, SIGNAL(minimizeBtnClicked()), this, SLOT(onMinimizeClicked()));
    }
    else
    {
        setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);
    }

    if(mTitlebarItems & Maximize)
    {
        if(mTitlebar)
        {
#if defined(Q_OS_WIN)
            connect(mTitlebar, SIGNAL(maximizeBtnClicked()), this, SLOT(onMaximizeClicked()));
            connect(mTitlebar, SIGNAL(restoreBtnClicked()), this, SLOT(onRestoreClicked()));
#elif defined(Q_OS_MAC)
            connect(mTitlebar, SIGNAL(zoomBtnClicked()), this, SLOT(showZoomed()));
            connect(mTitlebar, SIGNAL(fullScreenBtnClicked()), this, SLOT(onFullScreenClicked()));
#endif
        }
        if(mManager)
        {
            mManager->setManagedFunctionalities((OriginWindowManager::Functionalities)
                (OriginWindowManager::Movable|OriginWindowManager::Resizable));
            uiBase->innerFrame->installEventFilter(mManager);
#if defined(Q_OS_MAC)
			// We might get any or all of these items during a resize event so add the to a list to check against later
            mManager->addResizableItem(uiBase->innerFrame);
            mManager->addResizableItem(uiBase->outerFrame);
#endif
        }
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
        uiBase->outerFrame->setMouseTracking(true);
        uiBase->innerFrame->setMouseTracking(true);
    }
    else
    {
        setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
    }

    if(mTitlebarItems & Close)
    {
        setWindowFlags(windowFlags() | Qt::WindowCloseButtonHint);
        if(mTitlebar)
            connect(mTitlebar, SIGNAL(closeBtnClicked()), this, SIGNAL(closeWindow()));
    }
    else
    {
        setWindowFlags(windowFlags() & ~Qt::WindowCloseButtonHint);
    }
}

#if defined (Q_OS_MAC)
void OriginWindow::setHideTitlebar(bool aHide)
{
    if (aHide)
    {
        mTitlebar->setVisible(false);
        uiBase->innerVLayout->removeWidget(uiBase->titlebar);
    }
    else
    {
        uiBase->innerVLayout->insertWidget(0, uiBase->titlebar);
    }
}

bool OriginWindow::isFullscreen()
{
    return ToolkitPlatform::isFullscreen(this);
}
#endif
    
#ifdef Q_OS_WIN
bool OriginWindow::nativeEvent(const QByteArray & eventType, void* _message, long* result)
{
    MSG* message = static_cast<MSG*>(_message);
    if(message->message == WM_MENUCOMMAND)
    {
        if(message->wParam == 4) // SC_MAXIMIZE
        {
            onMaximizeClicked();
            if(result)
                *result = TRUE;
            return true;
        }
        
        else if(message->wParam == 3) // SC_MINIMIZE
        {
            onMinimizeClicked();
            if(result)
                *result = TRUE;
            return true;
        }
        
        else if(message->wParam == 0) // SC_RESTORE
        {
            // If we were maximized prior to minimizing, maximize again.
            if(isMinimized() && mManager && mManager->isManagedMaximized())
                onMaximizeClicked();
            else
            {
                // If it has the resizable functionality, show the button
                if(mTitlebar && (dynamic_cast<OriginTitlebarWin*>(mTitlebar))->isRestoreButtonVisible())
                    onRestoreClicked();
                else // Just call the functionality to restore the dialog
                {
                    if(mManager)
                        mManager->onManagedRestored();
                }
            }
            if(result)
                *result = TRUE;
            return true;
        }
        
        else if(message->wParam == 6) // SC_CLOSE
        {
            emit closeWindow();
            if(result)
                *result = TRUE;
            return true;
        }
    }
    else if(message->message == WM_SYSKEYDOWN)
    {
        // Alt+Space handler, since Qt's handler positions the menu incorrectly because of our app shadow.
        if(message->wParam == VK_SPACE)
        {
            if(mTitlebar)
            {
                return (dynamic_cast<OriginTitlebarWin*>(mTitlebar))->showWindowsSystemMenu(result);
            }
            return false;
        }
    }
    else if(message->message == WM_MENUCHAR)
    {
        // This prevents the Windows "error beep" on showing the Alt+Space menu and highlights the first menu option.
        if(result)
            *result = MNC_SELECT << 16;
        return true;
    }
    else if (message->message == WM_PALETTECHANGED)
    {
        // When Alt-Tabbing out of fullscreen DirectX apps or switching from an RDP session our cached DWM bitmap's alpha channel can become corrupted.
        // I'm not sure if this is a Windows, Qt or Origin bug but this seems to fix it
        update();
    }
    
    return QDialog::nativeEvent(eventType, message, result);
}
#endif


void OriginWindow::onMaximizeClicked()
{
    showMaximized();
    emit(maximizeClicked());
}


void OriginWindow::onMinimizeClicked()
{
    showMinimized();
    emit(minimizeClicked());
}


void OriginWindow::onRestoreClicked()
{
    showNormal();
    emit(restoreClicked());
}

#if defined (Q_OS_MAC)

void OriginWindow::toggleFullScreen()
{
#ifdef ENABLE_FULLSCREEN
    ToolkitPlatform::toggleFullscreen(this);
    emit(fullScreenChanged(ToolkitPlatform::isFullscreen(this)));
#endif
}

void OriginWindow::onFullScreenClicked()
{
    toggleFullScreen();
}
    
#endif

void OriginWindow::setTitleBarText(const QString& title)
{
    // TODO: Remove this function?
    if(mTitlebar)
        mTitlebar->setTitleBarText(title);
    setWindowIconText(title);
}


void OriginWindow::setTitleBarContent(QWidget* content)
{
    if(mTitlebar)
        mTitlebar->setTitleBarContent(content);
}


void OriginWindow::setEnvironmentLabel(const QString& lbl)
{
    // TODO: Remove this function?
    if(mTitlebar)
        mTitlebar->setEnvironmentLabel(lbl);
}


OriginPushButton* OriginWindow::getClickedButton()
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->getClickedButton();
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->getClickedButton();
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->getClickedButton();
    default:
        break;
    }
    return NULL;
}


void OriginWindow::clearClickedButton()
{
    switch(mContentTemplate)
    {
    case MsgBox:
        dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->clearButtonBoxClicked();
        break;
    case Scrollable:
        dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->clearButtonBoxClicked();
        break;
	case ScrollableMsgBox:
        dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->clearButtonBoxClicked();
		break;
    default:
        break;
    }
}


bool OriginWindow::event(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::Resize:
    case QEvent::WindowStateChange:
            
        updateWindowRegion();
            
#if defined (Q_OS_MAC)
        if (titlebarItems() & FullScreen)
        {
            bool isFullscreen = ToolkitPlatform::isFullscreen(this);
            if (isFullscreen)
            {
                setHideTitlebar(true);
                setEnableSizeGrip(false);
                
                uiBase->outerFrame->setProperty("style","max");
                uiBase->outerFrame->setStyle(QApplication::style());
            }
            else
            {
                setHideTitlebar(false);
                setEnableSizeGrip(false);
                
                uiBase->outerFrame->setProperty("style","norm");
                uiBase->outerFrame->setStyle(QApplication::style());
            }
            
            emit(fullScreenChanged(ToolkitPlatform::isFullscreen(this)));
        }
#endif

        break;

    case QEvent::WindowActivate:
        if(mTitlebar)
            mTitlebar->setActiveWindowStyle(true);            
        break;
    case QEvent::WindowDeactivate:
        if(mTitlebar)
            mTitlebar->setActiveWindowStyle(false);
        break;

    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
        {
            //pressing ESC while in one of the EADP edit boxes does not seem to trigger keyPressEvent
            //but we are able to detect as an ShortcutOverride event so handle it here
            //but since ESC pressed while NOT on the edit box ALSO comes thru here, change mIgnoreEscPress
            //so that when keyPressEvent gets called for that case, it won't emit rejected() again
            QKeyEvent* ke = dynamic_cast<QKeyEvent*>(event);
            if (ke)
            {
                switch (ke->key())
                {
                case Qt::Key_Escape:
                    if (!mIgnoreEscPress)
                    {
                        mIgnoreEscPress = true;
                        emit closeWindow();
                    }
                    break;

                    // ORPLAT-2818: OSX has a ghost shortcut key Cmd+Peroid. We need to ignore it.
                case Qt::Key_Period:
                    if (ke->modifiers() & Qt::ControlModifier)
                    {
                        event->ignore();
                        return true;
                    }
                    break;

                default:
                    break;
                }
            }
        }

        
    default:
        break;
    }

    OriginUIBase::event(event);
    return QDialog::event(event);
}


void OriginWindow::keyPressEvent(QKeyEvent* event)
{
    if(event->key() == Qt::Key_Escape)
    {
#if defined (Q_OS_MAC)
        if(supportsFullscreen() && isFullscreen())
            toggleFullScreen();
#endif
        if(!mIgnoreEscPress)
        {
            //pressing ESC while in one of the EADP edit boxes does not seem to trigger keyPressEvent
            //but we are able to detect as an ShortcutOverride event so it gets handled there
            //but since ESC pressed while NOT on the edit box ALSO comes goes to ShortCutOverride, change mIgnoreEscPress
            //so that in case this event is triggered first, ShortCutOverride wont emit rejected() again
            mIgnoreEscPress = true;
            emit closeWindow();
        }
        return;
    }

    QDialog::keyPressEvent(event);
}


void OriginWindow::closeEvent(QCloseEvent* event)
{
    OriginChromeBase::closeEvent(event);
    emit closeEventTriggered();
}


void OriginWindow::showEvent(QShowEvent* event)
{
    //// HACK -
    //// For inexplicable reasons, when showMinimized() happens the button remains
    //// with it's hover image. Perhaps we are not calling showMinimized() correctly..?
    //// the minimize animation does not occur. Dialog acts like hide() is being called.
    //QApplication::postEvent(this, new QEvent(QEvent::UpdateRequest), Qt::LowEventPriority);
    QDialog::showEvent(event);
    emit(aboutToShow());
}


QString& OriginWindow::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginWindow::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}


void OriginWindow::setChromeStyle(ChromeStyle chromeStyle)
{
    mChromeStyle = chromeStyle;
    switch(mChromeStyle)
    {
        default:
        case ChromeStyle_Dialog:
            {
                uiBase->outerFrame->setProperty("chromeStyle","dialog");

                // Set translucent background style
                setAttribute(Qt::WA_TranslucentBackground, true);
            }
            break;
        case ChromeStyle_Window:
            {
                uiBase->outerFrame->setProperty("chromeStyle","window");

#if defined(Q_OS_MAC)
                // We need the window to have a translucent background, at least for the content margin we use to extend
                // the grabbing area - furthermore, although small, we can see white areas around the corners of the window
                QMargins margins = contentsMargins();
                if (margins.top() || margins.left() || margins.bottom() || margins.right())
                    setAttribute(Qt::WA_TranslucentBackground, true);
                else
                    setAttribute(Qt::WA_TranslucentBackground, false);
#else
                // Remove translucent background style
                setAttribute(Qt::WA_TranslucentBackground, false);
#endif
            }
            break;
    }
    this->setStyleSheet(getElementStyleSheet());
}


const QMargins OriginWindow::borderWidths() const
{
    // TODO! - pull these from the QSS somehow!
    switch(mChromeStyle)
    {
        default:
        case ChromeStyle_Dialog:
            return QMargins(8,7,10,9); // THIS IS WRONG - FIX LATER
        case ChromeStyle_Window:
            return QMargins(7,7,9,9); // THIS IS WRONG - FIX LATER
    }
}


void OriginWindow::setManagerEnabled(const bool& enable)
{
    if(enable)
    {
        if(mManager == NULL)
        {
            OriginWindowManager::Functionalities functionality = OriginWindowManager::Movable;
            if(mTitlebarItems & Maximize)
                functionality |= OriginWindowManager::Resizable;
            mManager = new OriginWindowManager(this, functionality, borderWidths(), uiBase->titlebar);
        }
    }
    else
    {
        if(mManager)
        {
            delete mManager;
            mManager = NULL;
        }
    }
}


bool OriginWindow::isManagerEnabled() const
{
    return mManager != NULL;
}


void OriginWindow::setContent(QWidget* content)
{
    // This is kind of a bad thing to do, but kind of the easiest place to do it
    OriginWebView* webcontainer = dynamic_cast<OriginWebView*>(content);
    if(webcontainer && mManager)
        connect(webcontainer, SIGNAL(sizeUpdated()), mManager, SLOT(ensureOnScreen()));

    if(content)
    {
        switch(mContentTemplate)
        {
        case Scrollable:
        case MsgBox:
            mTemplate->setContent(content);
            break;
			
		case ScrollableMsgBox:
			dynamic_cast<OriginScrollableMsgBox*>(mContent)->setContent(content);
			break;

        case Custom:
        default:
            OriginChromeBase::setContent(content);
            break;
        }
    }
}


QWidget* const OriginWindow::content() const
{
    switch(mContentTemplate)
    {
    case MsgBox:
    case Scrollable:
        return mTemplate->getContent();
		
	case ScrollableMsgBox:
		return dynamic_cast<OriginScrollableMsgBox*>(mContent)->content();

    case Custom:
    default:
        return OriginChromeBase::content();
    }
}


OriginPushButton* OriginWindow::addButton(QDialogButtonBox::StandardButton which)
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->addButton(which);
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->addButton(which);
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->addButton(which);
    default:
        break;
    }
    return NULL;
}


OriginPushButton* OriginWindow::button(QDialogButtonBox::StandardButton which) const
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->button(which);
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->button(which);
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->button(which);
    default:
        break;
    }
    return NULL;
}


void OriginWindow::setButtonText(QDialogButtonBox::StandardButton which, const QString &text) 
{
    switch(mContentTemplate)
    {
    case MsgBox:
        dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->setButtonText(which, text);
        break;

    case Scrollable:
        dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->setButtonText(which, text);
        break;
		
	case ScrollableMsgBox:
        dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->setButtonText(which, text);
		break;

    default:
        break;
    }
}


OriginPushButton* OriginWindow::defaultButton() const
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->defaultButton();
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->defaultButton();
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->defaultButton();
    default:
        break;
    }
    return NULL;
}


void OriginWindow::setDefaultButton(QDialogButtonBox::StandardButton which)
{
    switch(mContentTemplate)
    {
    case MsgBox:
        dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->setDefaultButton(which);
        break;

    case Scrollable:
        dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->setDefaultButton(which);
        break;
		
	case ScrollableMsgBox:
        dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->setDefaultButton(which);
        break;

    default:
        break;
    }
}


void OriginWindow::setDefaultButton(OriginPushButton* button/*=NULL*/)
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->setDefaultButton(button);
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->setDefaultButton(button);
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->setDefaultButton(button);
    default:
        break;
    }
}


QDialogButtonBox::StandardButton OriginWindow::standardButton(OriginPushButton *button) const
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->standardButton(button);
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->standardButton(button);
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->standardButton(button);
    default:
        break;
    }
    return QDialogButtonBox::NoButton;
}


QDialogButtonBox::ButtonRole OriginWindow::buttonRole(OriginPushButton* button) const
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->buttonRole(button);
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->buttonRole(button);
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->buttonRole(button);
    default:
        break;
    }
    return QDialogButtonBox::NoRole;
}


void OriginWindow::setCornerContent(QWidget* content)
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->setCornerContent(content);
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->setCornerContent(content);
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->setCornerContent(content);
    default:
        break;
    }
}


QWidget* OriginWindow::cornerContent() const
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getCornerContent();
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getCornerContent();
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getCornerContent();
    default:
        break;
    }
    return NULL;
}


void OriginWindow::setEnableAllButtons(bool enable)
{
    switch(mContentTemplate)
    {
    case MsgBox:
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getButtonBox()->setEnableAllButtons(enable);
    case Scrollable:
        return dynamic_cast<OriginWindowTemplateScrollable*>(mTemplate)->getButtonBox()->setEnableAllButtons(enable);
	case ScrollableMsgBox:
        return dynamic_cast<OriginScrollableMsgBox*>(mContent)->getButtonBox()->setEnableAllButtons(enable);
    default:
        break;
    }
}


OriginMsgBox* const OriginWindow::msgBox() const
{
    if(mContentTemplate == MsgBox)
    {
        return dynamic_cast<OriginWindowTemplateMsgBox*>(mTemplate)->getMsgBox();
    }
    return NULL;
}


OriginScrollableMsgBox* const OriginWindow::scrollableMsgBox() const
{
    return dynamic_cast<OriginScrollableMsgBox*>(mContent);
}


OriginWebView* const OriginWindow::webview() const
{
    if(mContentTemplate == WebContent)
        return (OriginWebView*)mContent;
    return NULL;
}


void OriginWindow::ensureCreatedMsgBanner()
{
    if(mMsgBanner == NULL)
    {
        mMsgBanner = new OriginBanner();
        mMsgBanner->setBannerType(OriginBanner::Message);
        mMsgBanner->setMinimumHeight(200);
        mMsgBanner->setMaximumHeight(200);
        // We have to use it here because the banner relize on it's parent's
        // size if it's a message banner
        uiBase->innerVLayout->insertWidget(0, mMsgBanner);
        mMsgBanner->setVisible(false);
    }
}


void OriginWindow::setBannerVisible(const bool& visible)
{
    ensureCreatedBanner();
    mBanner->setVisible(visible);
    // If false should we remove it from uiBase->innerVLayout and delete it?
    if(visible == false)
    {
        if(mTemplate)
        {
            mTemplate->resize(mTemplate->width(), mTemplate->height() - mBanner->height());
            QMetaObject::invokeMethod (this, "adjustWidgetSize", Qt::QueuedConnection, Q_ARG (QWidget*, mTemplate));
        }

        resize(width(), height() - mBanner->height());
        disconnect(mBanner, SIGNAL(linkActivated(const QString&)), this, SIGNAL(bannerLinkActivated(const QString&)));
    }
    else 
    {
        // disconnect before we connect, otherwise we may connect multiple times and therefore fire the signal multiple times
        disconnect(mBanner, SIGNAL(linkActivated(const QString&)), this, SIGNAL(bannerLinkActivated(const QString&)));
        connect(mBanner, SIGNAL(linkActivated(const QString&)), this, SIGNAL(bannerLinkActivated(const QString&)));
    }

    QMetaObject::invokeMethod (this, "adjustWidgetSize", Qt::QueuedConnection, Q_ARG (QWidget*, this));
}


void OriginWindow::setMsgBannerVisible(const bool& visible)
{
    ensureCreatedMsgBanner();
    if(mMsgBanner)
    {
        // If false should we remove it from uiBase->innerVLayout and delete it?
        if(mTemplate)
            mTemplate->setVisible(!visible);
        mMsgBanner->setVisible(visible);
        QMetaObject::invokeMethod (this, "adjustWidgetSize", Qt::QueuedConnection, Q_ARG (QWidget*, this));
        if (visible)
        {
            // disconnect before we connect, otherwise we may connect multiple times and therefore fire the signal multiple times
            disconnect(mMsgBanner, SIGNAL(linkActivated(const QString&)), this, SIGNAL(bannerLinkActivated(const QString&)));
            connect(mMsgBanner, SIGNAL(linkActivated(const QString&)), this, SIGNAL(bannerLinkActivated(const QString&)));
        }
        else 
        {
            disconnect(mMsgBanner, SIGNAL(linkActivated(const QString&)), this, SIGNAL(bannerLinkActivated(const QString&)));
        }
    }
}


void OriginWindow::setSystemMenuEnabled(const bool& enable)
{
    // TODO: Remove this function?
    mTitlebar->setSystemMenuEnabled(enable);
}


void OriginWindow::setTitlebarItems(const TitlebarItems& titlebarItems)
{
    mTitlebarItems = titlebarItems;
#ifdef ENABLE_FULLSCREEN
    // If fullscreen is not supported disable the flag
    if ((mTitlebarItems & FullScreen) && !supportsFullscreen())
    {
        mTitlebarItems &= ~FullScreen;
    }
#else
    mTitlebarItems &= ~FullScreen;
#endif

#if defined(Q_OS_MAC)
    if(mTitlebar)
        mTitlebar->setupButtonFunctionality(mTitlebarItems & Icon, 
        mTitlebarItems & Minimize, 
        mTitlebarItems & Maximize, 
        mTitlebarItems & Close,
        mTitlebarItems & FullScreen);
#else
    if(mTitlebar)
        mTitlebar->setupButtonFunctionality(mTitlebarItems & Icon,
        mTitlebarItems & Minimize,
        mTitlebarItems & Maximize,
        mTitlebarItems & Close);
#endif
}

QVector<QObject*> OriginWindow::titlebarButtons() const
{
    if (mTitlebar)
        return mTitlebar->buttons();
    
    return QVector<QObject*>();
}

const QString OriginWindow::decoratedWindowTitle()
{
    // First see if the window has an accessible name and use that if available
    QString title = accessibleName();

    // Next look to see if this is a message box template that we can grab the title text from
    if (title.isEmpty())
    {
        if (msgBox())
        {
            // Yes it's a message box template. See if it has title text
            title = msgBox()->getTitle();
        }
		else if(scrollableMsgBox())
		{
			title = scrollableMsgBox()->getTitle();
		}
    }

    // Lastly just grab the titlebar text
    if (title.isEmpty())
    {
        title = windowTitle();
        // We may want to filter out any titles that just say "Origin"
    }

    return title;
}


// If-def'ed because the mac titlebar doesn't include these functions.
// If it should be taken out: Put function in OriginTitlebarBase as pure virtual
#ifdef Q_OS_WIN
QLayout* OriginWindow::menuBarLayout()
{
    // TODO: Remove this function?
    return mTitlebar ? dynamic_cast<OriginTitlebarWin*>(mTitlebar)->menuBarLayout() : NULL;
}


void OriginWindow::setMenuBarVisible(const bool& visible)
{
    // TODO: Remove this function?
    if(mTitlebar)
        dynamic_cast<OriginTitlebarWin*>(mTitlebar)->setMenuBarVisible(visible);
}

bool OriginWindow::msWinEvent(MSG * message, long *result)
{
	// TODO: Qt5 - FIX ME
    return nativeEvent("windows_generic_MSG", message, result);
}
#endif


void OriginWindow::raise()
{
    if (isMinimized())
    {
        onRestoreClicked();
    }

    OriginChromeBase::raise();
}

QRegion OriginWindow::roundedRectRegion(const QRect& rect, int r)
{
    QRegion region;

    // middle and borders
    region += rect.adjusted(r, 0, -r, 0);
    region += rect.adjusted(0, r, 0, -r);

    // top left
    QRect corner(rect.topLeft(), QSize(r*2, r*2));
    region += QRegion(corner, QRegion::Ellipse);

    // top right
    corner.moveTopRight(rect.topRight());
    region += QRegion(corner, QRegion::Ellipse);

    // bottom left
    corner.moveBottomLeft(rect.bottomLeft());
    region += QRegion(corner, QRegion::Ellipse);

    // bottom right
    corner.moveBottomRight(rect.bottomRight());
    region += QRegion(corner, QRegion::Ellipse);

    return region;
}

void OriginWindow::updateWindowRegion()
{
#ifdef Q_OS_WIN
    if (mChromeStyle == ChromeStyle_Window)
    {
        if (mManager && mManager->isManagedMaximized())
        {
            this->setMask(roundedRectRegion(this->rect(), 0));
        }
        else
        {
            this->setMask(roundedRectRegion(this->rect(), CORNER_RADIUS));
        }
    }
#endif
}

void OriginWindow::adjustWidgetSize (QWidget* widget)
{
    widget->adjustSize();
}

int OriginWindow::execWindow(const QPoint& pos)
{
    setModal(true);
    showWindow(pos);
    return this->exec();
}


void OriginWindow::showWindow(const QPoint& pos)
{
    if (!pos.isNull())
    {
        this->move(pos);
    }
    this->show();
    
    // this is an ugly hack but ...
    // when we create a hidden Qt window, the OS window creation is deferred until we show it
    // unfortunately, this means that the title may not have been set properly so we manually
    // hammer it in here
//    Q_D(QDialog);
//    d->setWindowTitle_helper(windowTitle());
//    d->setWindowIconText_helper(windowIconText());
   
    if(QApplication::focusWidget() && QApplication::activeModalWidget() == NULL)
    {
        this->raise();
        this->activateWindow();
    }
}
    
bool OriginWindow::supportsFullscreen()
{
#ifdef Q_OS_MAC
    return ToolkitPlatform::supportsFullscreen(this);
#else
    return false;
#endif
}


void OriginWindow::configForOIG(const bool& keepWindowManagerEnabled)
{
    // Because OIG handles a top level Esc key reaction - closes itself. We don't want to close
    // the window. The user might have meant to close the OIG and not the window.
    mIgnoreEscPress = true;
    mTitlebar->setToolTipVisible(false);
    setSystemMenuEnabled(false);
    setManagerEnabled(keepWindowManagerEnabled);

    // In the tabbed IGO Web Browser, we need to explicitly clip the tabs of the tabBar
    // since the tabBar is contained within a QGraphicsScene. By default, QGraphicsItems
    // (i.e, the tabBar and its tabs) don't get clipped. However, when clipping is set
    // on the tabBar via QGraphicsProxyWidget::createProxyForChildWidget(), the backgrounds
    // of the 'outerFrame' and 'innerFrame' of OriginWindow gets set to be opaque... unless
    // it is set beforehand. Thus, we set the backgrounds to be transparent here... 
    uiBase->outerFrame->setPalette(QPalette(QColor(0,0,0,0)));
    uiBase->innerFrame->setPalette(QPalette(QColor(0,0,0,0)));
}


void OriginWindow::setEnableSizeGrip(bool aEnable)
{
    QSizeGrip* grip = this->findChild<QSizeGrip*>();
    if (grip)
    {
        if (aEnable)
            grip->setVisible(true);
        else
            grip->setVisible(false);
    }
}

void OriginWindow::showMaximized()
{
    uiBase->outerFrame->setProperty("style","max");
    uiBase->outerFrame->setStyle(QApplication::style());

#if defined(Q_OS_WIN)
    if(mTitlebar)
        dynamic_cast<OriginTitlebarWin*>(mTitlebar)->showMaximized();
#endif
    
    setEnableSizeGrip(false);

    if(mManager)
        mManager->onManagedMaximized();
}

void OriginWindow::showMinimized()
{
    // There seems to be a QT5 bug that makes qt windows freak out if you try and minimize them before they are shown
    // See the following Jira tickets
    // EBIBUGS-25664
    // EBIBUGS-25665
    show();

    if(mManager)
        mManager->onManagedMinimized();
}

void OriginWindow::showNormal()
{
    if(mManager)
        mManager->onManagedRestored();

    uiBase->outerFrame->setProperty("style","norm");
    uiBase->outerFrame->setStyle(QApplication::style());

#if defined(Q_OS_WIN)
    if(mTitlebar)
        dynamic_cast<OriginTitlebarWin*>(mTitlebar)->showNormal();
    
    setEnableSizeGrip(true);
#endif
#ifdef Q_OS_MAC
    // We don't want to show the gripper icon on the Mac platform
    setEnableSizeGrip(false);
#endif
}


void OriginWindow::showZoomed()
{
    if(mManager)
        mManager->onManagedZoomed();
}


void OriginWindow::alert(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText)
{
    OriginWindow::alert(icon, title, description, buttonText, QString(""), QPoint());
}


void OriginWindow::alert(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText,
                         const QString& windowTitle,const QPoint& newPosition)
{
    OriginWindow *dialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
                                NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);

    dialog->msgBox()->setup(icon, title, description);

    dialog->setButtonText(QDialogButtonBox::Ok, buttonText);

    dialog->setDefaultButton(QDialogButtonBox::Ok);
    dialog->manager()->setupButtonFocus();

    connect(dialog, SIGNAL(finished(int)), dialog, SLOT(close()));
    connect(dialog, SIGNAL(closeWindow()), dialog, SLOT(reject()));
    connect(dialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(reject()));

    if (!windowTitle.isEmpty())
        dialog->setTitleBarText(windowTitle);
    if (!newPosition.isNull())
       dialog->execWindow(newPosition);
    else
        dialog->execWindow();
}


OriginWindow* OriginWindow::alertNonModal(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText)
{
    OriginWindow* dialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok);
    dialog->msgBox()->setup(icon, title, description);
    dialog->setButtonText(QDialogButtonBox::Ok, buttonText);
    dialog->manager()->setupButtonFocus();
    disconnect(dialog, SIGNAL(closeWindow()), dialog, SIGNAL(rejected()));
    connect(dialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(reject()));
    connect(dialog, SIGNAL(closeWindow()), dialog, SLOT(reject()));
    return dialog;
}


OriginWindow* OriginWindow::alertNonModalScrollable(OriginScrollableMsgBox::IconType icon, const QString& title, const QString& description, const QString& buttonText)
{
    OriginWindow *dialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Ok);
    dialog->scrollableMsgBox()->setup(icon, title, description);
    dialog->setButtonText(QDialogButtonBox::Ok, buttonText);
    dialog->manager()->setupButtonFocus();
    disconnect(dialog, SIGNAL(closeWindow()), dialog, SIGNAL(rejected()));
    connect(dialog->button(QDialogButtonBox::Ok), SIGNAL(clicked()), dialog, SLOT(reject()));
    connect(dialog, SIGNAL(closeWindow()), dialog, SLOT(reject()));
    return dialog;
}


int OriginWindow::prompt(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& yesText, const QString& noText, QDialogButtonBox::StandardButton defaultButton/*=QDialogButtonBox::NoButton*/)
{
    return OriginWindow::prompt(icon, "", title, description, yesText, noText, defaultButton);
}


int OriginWindow::prompt(OriginMsgBox::IconType icon, const QString& titleBar, const QString& title, const QString& description, const QString& yesText, const QString& noText, QDialogButtonBox::StandardButton defaultButton/*=QDialogButtonBox::NoButton*/)
{
    OriginWindow *dialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Close,
                                NULL, OriginWindow::MsgBox, QDialogButtonBox::Yes|QDialogButtonBox::No);

    if (!titleBar.isEmpty())
        dialog->setTitleBarText(titleBar);

    dialog->msgBox()->setup(icon, title, description);

    dialog->setButtonText(QDialogButtonBox::Yes, yesText);
    dialog->setButtonText(QDialogButtonBox::No, noText);
    
    if (defaultButton != QDialogButtonBox::NoButton)
    {
        dialog->setDefaultButton(defaultButton);
    }
    else
    {
        dialog->setDefaultButton(QDialogButtonBox::No);
    }
    dialog->manager()->setupButtonFocus();

    connect(dialog, SIGNAL(finished(int)), dialog, SLOT(close()));
    connect(dialog, SIGNAL(rejected()), dialog, SLOT(close()));
    connect(dialog->button(QDialogButtonBox::Yes), SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(dialog->button(QDialogButtonBox::No), SIGNAL(clicked()), dialog, SLOT(reject()));

    return dialog->execWindow();
}


OriginWindow* OriginWindow::promptNonModal(OriginMsgBox::IconType icon, const QString& title, const QString& description, const QString& yesText, const QString& noText, QDialogButtonBox::StandardButton defaultButton/*=QDialogButtonBox::NoButton*/)
{
    OriginWindow *dialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        NULL, OriginWindow::MsgBox, QDialogButtonBox::Yes|QDialogButtonBox::No);
    dialog->msgBox()->setup(icon, title, description);
    dialog->setButtonText(QDialogButtonBox::Yes, yesText);
    dialog->setButtonText(QDialogButtonBox::No, noText);
    dialog->setDefaultButton(defaultButton != QDialogButtonBox::NoButton ? defaultButton : QDialogButtonBox::No);
    dialog->manager()->setupButtonFocus();
    disconnect(dialog, SIGNAL(closeWindow()), dialog, SIGNAL(rejected()));
    connect(dialog->button(QDialogButtonBox::Yes), SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(dialog->button(QDialogButtonBox::No), SIGNAL(clicked()), dialog, SLOT(reject()));
    connect(dialog, SIGNAL(closeWindow()), dialog, SLOT(reject()));
    return dialog;
}


OriginWindow* OriginWindow::promptNonModalScrollable(OriginScrollableMsgBox::IconType icon, const QString& title, const QString& description, const QString& yesText, const QString& noText, QDialogButtonBox::StandardButton defaultButton/*=QDialogButtonBox::NoButton*/)
{
    OriginWindow *dialog = new OriginWindow(OriginWindow::Icon | OriginWindow::Minimize | OriginWindow::Close,
        NULL, OriginWindow::ScrollableMsgBox, QDialogButtonBox::Yes|QDialogButtonBox::No);
    dialog->scrollableMsgBox()->setup(icon, title, description);
    dialog->setButtonText(QDialogButtonBox::Yes, yesText);
    dialog->setButtonText(QDialogButtonBox::No, noText);
    dialog->setDefaultButton(defaultButton != QDialogButtonBox::NoButton ? defaultButton : QDialogButtonBox::No);
    dialog->manager()->setupButtonFocus();
    disconnect(dialog, SIGNAL(closeWindow()), dialog, SIGNAL(rejected()));
    connect(dialog->button(QDialogButtonBox::Yes), SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(dialog->button(QDialogButtonBox::No), SIGNAL(clicked()), dialog, SLOT(reject()));
    connect(dialog, SIGNAL(closeWindow()), dialog, SLOT(reject()));
    return dialog;
}


void OriginWindow::setCreationTime()
{
    mCreationTime = QTime::currentTime();
}
    
}
}
