#include "origintitlebarwin.h"
#include "ui_origintitlebarwin.h"
#include "originwindow.h"

#include <QMouseEvent>
#include <QDesktopWidget>
#include <QDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#include <winuser.h>
const static unsigned int enabled = (MF_BYCOMMAND | MF_ENABLED);
const static unsigned int disabled = (MF_BYCOMMAND | MF_GRAYED);
#endif

namespace Origin {
namespace UIToolkit {

OriginTitlebarWin::OriginTitlebarWin(QWidget* parent, OriginWindow* partner)
: OriginTitlebarBase(parent, partner)
, uiTitlebar(NULL)
{
    setOriginElementName("OriginTitlebarWin");
    parent->setObjectName("OriginTitlebarWin");
    uiTitlebar = new Ui::OriginTitlebarWin();
    uiTitlebar->setupUi(parent);
    parent->installEventFilter(this);
    mTitle = uiTitlebar->lblWindowTitle->text();
    setEnvironmentLabel("");
    uiTitlebar->windowIcon->installEventFilter(this);
    if(mSystemMenuEnabled)
        connect(uiTitlebar->windowIcon, SIGNAL(toggled(bool)), this, SLOT(showSystemMenu(bool)));
    connect(uiTitlebar->btnMinimize, SIGNAL(clicked()), this, SIGNAL(minimizeBtnClicked()));
    connect(uiTitlebar->btnMaximize, SIGNAL(clicked()), this, SIGNAL(maximizeBtnClicked()));
    connect(uiTitlebar->btnRestore, SIGNAL(clicked()), this, SIGNAL(restoreBtnClicked()));
    connect(uiTitlebar->btnClose, SIGNAL(clicked()), this, SIGNAL(closeBtnClicked()));
}


OriginTitlebarWin::~OriginTitlebarWin()
{
    if(uiTitlebar)
    {
        delete uiTitlebar;
        uiTitlebar = NULL;
    }
}


void OriginTitlebarWin::showMaximized()
{
    if((uiTitlebar && uiTitlebar->btnRestore && uiTitlebar->btnMaximize)
        // Only show these buttons if restore/maximized is enabled
        && (uiTitlebar->btnRestore->isVisible() || uiTitlebar->btnMaximize->isVisible()))
    {
        uiTitlebar->btnMaximize->hide();
        uiTitlebar->btnRestore->show();
    }

    if(parentWidget())
    {
        parentWidget()->setProperty("style", "max");
        parentWidget()->setStyle(QApplication::style());
    }
}


void OriginTitlebarWin::showNormal()
{
    if((uiTitlebar && uiTitlebar->btnRestore && uiTitlebar->btnMaximize)
        // Only show these buttons if restore/maximized is enabled
        && (uiTitlebar->btnRestore->isVisible() || uiTitlebar->btnMaximize->isVisible()))
    {
        uiTitlebar->btnRestore->hide();
        uiTitlebar->btnMaximize->show();
    }

    if(parentWidget())
    {
        parentWidget()->setProperty("style", QVariant::Invalid);
        parentWidget()->setStyle(QApplication::style());
    }
}


void OriginTitlebarWin::setMenuBarVisible(const bool& visible)
{
    if(uiTitlebar)
        uiTitlebar->menuContainer->setVisible(visible);
}


QLayout* OriginTitlebarWin::menuBarLayout()
{
    return uiTitlebar ? (uiTitlebar->menuContainerLayout) : NULL;
}


void OriginTitlebarWin::setSystemMenuEnabled(const bool& enable)
{
    mSystemMenuEnabled = enable;
    if(uiTitlebar && uiTitlebar->windowIcon)
    {
        if(enable) connect(uiTitlebar->windowIcon, SIGNAL(toggled(bool)), this, SLOT(showSystemMenu(bool)));
        else disconnect(uiTitlebar->windowIcon, SIGNAL(toggled(bool)), this, SLOT(showSystemMenu(bool)));
    }
}


void OriginTitlebarWin::setEnvironmentLabel(const QString& lbl)
{
    if(uiTitlebar)
    {
        uiTitlebar->lblDebugMode->setText(lbl);
        uiTitlebar->lblDebugMode->setMaximumWidth(lbl.size() ? 0xffffff : 0);
        uiTitlebar->lblDebugMode->setVisible(lbl.count());
    }
}


void OriginTitlebarWin::setTitleBarText(const QString& title)
{
    mTitle = title;
    
    if(uiTitlebar)
    {
        // Is-hidden is not the same as is-visible == false.
        // Is-hidden means that you - the engineer told the object - don't show yourself
        // Is-visible == false means that the object isn't visible to the desktop
        // If we are in the situation where you are asking an object .... hey are you visible, but the parent isn't visible - it will return false even though it will be visible when the parent is
        int spacerWidth = uiTitlebar->buttonContainer->width();
        if (uiTitlebar->windowIcon->isHidden() == false)
            spacerWidth -= uiTitlebar->windowIcon->width();
        if(uiTitlebar->menuContainer->isHidden() == false)
            spacerWidth -= uiTitlebar->menuContainer->contentsMargins().left() + uiTitlebar->menuContainer->contentsMargins().right();

        // Account for the titlebar's layoutMargins
        spacerWidth -= uiTitlebar->titlebarLayout->contentsMargins().left();

        if(spacerWidth < 0)
            spacerWidth = 0;
        uiTitlebar->horizontalSpacer->changeSize(spacerWidth, 1, QSizePolicy::Fixed);
        uiTitlebar->lblWindowTitle->setText(uiTitlebar->lblWindowTitle->fontMetrics().elidedText(title, Qt::ElideRight, uiTitlebar->lblWindowTitle->width()));
    }

    if(mWindowPartner)
        mWindowPartner->setWindowTitle(title);
}

void OriginTitlebarWin::setTitleBarContent(QWidget* content)
{
    if(uiTitlebar)
    {
        if(mCustomContent == NULL)
        {
            if(content)
            {
                mCustomContent = content;
                // custom content will be entirely handled externally!!!
                uiTitlebar->horizontalLayout->insertWidget(0/*after lblWindowTitle*/, mCustomContent);
            }
        }
        else
        {
            // remove the old content widget
            uiTitlebar->horizontalLayout->removeWidget(mCustomContent);
            mCustomContent = NULL;
            // set the new content widget
            setTitleBarContent(content);
        }
    }
}


bool OriginTitlebarWin::isRestoreButtonVisible()
{
    // TODO: Remove this function and do something smarter
    return (uiTitlebar && uiTitlebar->btnRestore) ? uiTitlebar->btnRestore->isVisible() : false;
}


void OriginTitlebarWin::setupButtonFunctionality(const bool& hasIcon, const bool& hasMinimize, 
                                                     const bool& hasMaximize, const bool& hasClose)
{
    if(uiTitlebar && parentWidget())
    {
        parentWidget()->setUpdatesEnabled(false);
        if(!hasMinimize && !hasMaximize && !hasClose)
            uiTitlebar->buttonContainer->setFixedWidth(uiTitlebar->buttonContainer->width());
        else
        {
            uiTitlebar->buttonContainer->setMinimumSize(0,0);
            uiTitlebar->buttonContainer->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        }
        uiTitlebar->windowIcon->setVisible(hasIcon);
        uiTitlebar->btnMinimize->setVisible(hasMinimize);
        uiTitlebar->btnMaximize->setVisible(hasMaximize);
        uiTitlebar->btnRestore->hide();
        uiTitlebar->btnClose->setVisible(hasClose);
        setTitleBarText(mTitle);
        parentWidget()->setUpdatesEnabled(true);
    }
}


bool OriginTitlebarWin::showSystemMenu(bool show /*= true*/, int xPos /*= -1*/, int yPos /*= -1*/)
{
    bool ret = false;
#if defined(Q_OS_WIN)
    if(show)
    {
        ret = showWindowsSystemMenu(NULL, xPos, yPos);
    }
#endif
    return ret;
}


#if defined(Q_OS_WIN)
bool OriginTitlebarWin::showWindowsSystemMenu(long *result, int xPos /*= -1*/, int yPos /*= -1*/)
{
    HWND id = (HWND)(mWindowPartner->window()->winId());
    HMENU menu = ::GetSystemMenu(id, FALSE);
    int menuX = 0, menuY = 0;

    if(uiTitlebar)
    {
        MENUINFO lpmi = {0};
        lpmi.cbSize = sizeof( MENUINFO ); // required !
        ::GetMenuInfo(menu, &lpmi);
        lpmi.fMask = MIM_STYLE;
        lpmi.dwStyle = lpmi.dwStyle | MNS_MODELESS | MNS_NOTIFYBYPOS;
        ::SetMenuInfo(menu, &lpmi);

        // Enable/disable menu options based on our state.
        // Size is always disabled because we have custom resizing.
        // TODO: Figure out a way to tie in our custom sizing to Windows sizing?
        const bool maximized = uiTitlebar->btnRestore ? uiTitlebar->btnRestore->isVisible() : false;
        const bool minimized = mWindowPartner ? mWindowPartner->isMinimized() : false;
        const bool isMaximizable = uiTitlebar->btnRestore->isVisible() || uiTitlebar->btnMaximize->isVisible();
        const bool isMinimizable = uiTitlebar->btnMinimize ? uiTitlebar->btnMinimize->isVisible() : false;
        const bool isClosable = uiTitlebar->btnClose ? uiTitlebar->btnClose->isVisible() : false;
        ::EnableMenuItem(menu, SC_MINIMIZE, (!minimized && isMinimizable) ? enabled : disabled);
        ::EnableMenuItem(menu, SC_MAXIMIZE, (!maximized && isMaximizable) ? enabled : disabled);
        ::EnableMenuItem(menu, SC_RESTORE, (minimized || maximized) ? enabled : disabled);
        ::EnableMenuItem(menu, SC_SIZE, disabled);
        ::EnableMenuItem(menu, SC_MOVE, disabled);
        ::EnableMenuItem(menu, SC_CLOSE, isClosable ? enabled : disabled);

        // Bold the "Close" menu item.
        MENUITEMINFO closeItem;
        closeItem.cbSize = sizeof(MENUITEMINFO);
        closeItem.fMask = MIIM_STATE;
        closeItem.fState = MFS_DEFAULT;
        ::SetMenuItemInfo(menu, SC_CLOSE, FALSE, &closeItem);


        QPoint global;
        if(parentWidget())
            global = parentWidget()->mapToGlobal(QPoint(0, 0)) + parentWidget()->geometry().bottomLeft();

        menuX = xPos >= 0 ? xPos : global.x();
        menuY = yPos >= 0 ? yPos : global.y();
    }

   return ::TrackPopupMenu(menu, TPM_TOPALIGN | TPM_LEFTALIGN, menuX, menuY, 0, id, 0);
}
#endif


void OriginTitlebarWin::setActiveWindowStyle(const bool& isActive)
{
    QVariant variant = isActive ? QVariant::Invalid : QVariant("inactive");
    if(uiTitlebar && uiTitlebar->lblWindowTitle)
    {
        uiTitlebar->lblWindowTitle->setProperty("style", variant);
        uiTitlebar->lblWindowTitle->setStyle(QApplication::style());
    }
}


void OriginTitlebarWin::setToolTipVisible(const bool& isVisible)
{
    if(uiTitlebar)
    {
        if(isVisible)
        {
            if(uiTitlebar->btnMinimize)
                uiTitlebar->btnMinimize->setToolTip(tr("ebisu_client_minimize"));
            if(uiTitlebar->btnMaximize)
                uiTitlebar->btnMaximize->setToolTip(tr("ebisu_client_maximize"));
            if(uiTitlebar->btnRestore)
                uiTitlebar->btnRestore->setToolTip(tr("ebisu_client_restore_down"));
            if(uiTitlebar->btnClose)
                uiTitlebar->btnClose->setToolTip(tr("ebisu_client_close"));
        }
        else
        {
            if(uiTitlebar->btnMinimize)
                uiTitlebar->btnMinimize->setToolTip("");
            if(uiTitlebar->btnMaximize)
                uiTitlebar->btnMaximize->setToolTip("");
            if(uiTitlebar->btnRestore)
                uiTitlebar->btnRestore->setToolTip("");
            if(uiTitlebar->btnClose)
                uiTitlebar->btnClose->setToolTip("");
        }
    }
}


bool OriginTitlebarWin::eventFilter(QObject* obj, QEvent* event)
{
    switch(event->type())
    {
    case QEvent::MouseButtonDblClick:
        {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if(mouseEvent && mouseEvent->button() == Qt::LeftButton && uiTitlebar)
            {
                if(uiTitlebar->windowIcon && obj == uiTitlebar->windowIcon)
                {
                    emit rejectWindow();
                    return true;
                }
                else if((parentWidget() && obj == parentWidget())
                    && uiTitlebar->btnMaximize || uiTitlebar->btnRestore)
                {
                    uiTitlebar->btnMaximize->isVisible() ? emit maximizeBtnClicked() : emit restoreBtnClicked();
                }
            }
        }
        break;

    case QEvent::MouseButtonRelease:
        {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if(parentWidget() && obj == parentWidget())
            {
                if(mouseEvent->button() == Qt::RightButton
                    && mSystemMenuEnabled)
                {
                    showSystemMenu(true, mouseEvent->globalX(), mouseEvent->globalY());
                }
            }
        }
        break;

    case QEvent::Resize:
        if(parentWidget() && obj == parentWidget())
        {
            if(mTitle.isEmpty() == false) setTitleBarText(mTitle);
        }
        break;

    default:
        break;
    }

    return QWidget::eventFilter(obj, event);
}

QVector<QObject*> OriginTitlebarWin::buttons() const
{
    QVector<QObject*> actionButtons;

    actionButtons.append(uiTitlebar->btnMinimize);
    actionButtons.append(uiTitlebar->btnMaximize);
    actionButtons.append(uiTitlebar->btnRestore);
    actionButtons.append(uiTitlebar->btnClose);

    return actionButtons;
}

}
}
