#include "origintitlebarosx.h"
#include "ui_origintitlebarosx.h"
#include "originwindow.h"


namespace Origin {
namespace UIToolkit {

OriginTitlebarOSX::OriginTitlebarOSX(QWidget* parent, OriginWindow* partner)
: OriginTitlebarBase(parent, partner)
, uiTitlebar(NULL)
{
    setOriginElementName("OriginTitlebarOSX");
    if(parent)
        parent->setObjectName("OriginTitlebarOSX");
    uiTitlebar = new Ui_OriginTitlebarOSX();
    uiTitlebar->setupUi(parent);
    uiTitlebar->buttonContainer->installEventFilter(this);
    uiTitlebar->lblDebugMode->setVisible(false);
    uiTitlebar->fullscreenContainer->setVisible(false);

    // Default to setting the taskbar title to Origin
    setTitleBarText(tr("application_name"));
    if(partner)
        partner->installEventFilter(this);
    
    connect(uiTitlebar->btnMinimize, SIGNAL(clicked()), this, SIGNAL(minimizeBtnClicked()));
    connect(uiTitlebar->btnZoom, SIGNAL(clicked()), this, SIGNAL(zoomBtnClicked()));
    connect(uiTitlebar->btnZoom, SIGNAL(clicked()), this, SLOT(removeZoomHover()));
    connect(uiTitlebar->btnClose, SIGNAL(clicked()), this, SIGNAL(closeBtnClicked()));
    connect(uiTitlebar->btnFullscreen, SIGNAL(clicked()), this, SIGNAL(fullScreenBtnClicked()));
}


OriginTitlebarOSX::~OriginTitlebarOSX()
{
    if(uiTitlebar)
    {
        delete uiTitlebar;
        uiTitlebar = NULL;
    }
}


void OriginTitlebarOSX::setEnvironmentLabel(const QString& lbl)
{
    uiTitlebar->lblDebugMode->setText(lbl);
    uiTitlebar->lblDebugMode->setVisible(!lbl.isEmpty());
}


void OriginTitlebarOSX::setTitleBarText(const QString& title)
{
    mTitle = title;
    // take the width of the dialog and then remove all
    // the items being shown in the title
    int spacerWidth = 0;

    if(!uiTitlebar->buttonContainer->isHidden())
        spacerWidth += uiTitlebar->buttonContainer->width();
    if (!uiTitlebar->lblDebugMode->isHidden()) 
        spacerWidth -= uiTitlebar->lblDebugMode->width();
    if (!uiTitlebar->fullscreenContainer->isHidden())
        spacerWidth -= uiTitlebar->fullscreenContainer->width();
    if (mCustomContent && !mCustomContent->isHidden())
    {
        // TODO: FIX
        //spacerWidth -= mCustomContent->width();
        spacerWidth -= 14;
    }
    if(spacerWidth < 0)
        spacerWidth = 0;
    uiTitlebar->horizontalSpacer->changeSize(spacerWidth, 1, QSizePolicy::Fixed);
    uiTitlebar->lblWindowTitle->setText(fontMetrics().elidedText(title, Qt::ElideRight, uiTitlebar->lblWindowTitle->width()));
    if(mWindowPartner)
        mWindowPartner->setWindowTitle(title);
}

void OriginTitlebarOSX::setTitleBarContent(QWidget* content)
{
    if(uiTitlebar)
    {
        if(!mCustomContent)
        {
            mCustomContent = content;
            QMargins m = uiTitlebar->titlebarLayout->contentsMargins();
            m.setRight(12);
            uiTitlebar->titlebarLayout->setContentsMargins(m);
            // custom content will be entirely handled externally!!!
            uiTitlebar->titlebarLayout->insertWidget(3/*after debug title*/, mCustomContent);
        }
        else
        {
            // remove the old content widget
            uiTitlebar->titlebarLayout->removeWidget(mCustomContent);
            mCustomContent = NULL;
            // set the new content widget
            setTitleBarContent(content);
        }
    }
}


bool OriginTitlebarOSX::showSystemMenu(bool /*show*/, int /*xPos*/, int /*yPos*/)
{
    // TODO: Does Mac have a system menu?
    return false;
}

void OriginTitlebarOSX::setupButtonFunctionality(const bool& /*hasIcon*/, const bool& hasMinimize, 
                                                 const bool& hasMaximize, const bool& hasClose,
                                                 const bool& hasFullScreen)
{
    uiTitlebar->btnMinimize->setEnabled(hasMinimize);
    uiTitlebar->btnZoom->setEnabled(hasMaximize);
    uiTitlebar->btnClose->setEnabled(hasClose);
    uiTitlebar->fullscreenContainer->setVisible(hasFullScreen);
    uiTitlebar->buttonContainer->adjustSize();
    uiTitlebar->buttonContainer->setFixedWidth(uiTitlebar->buttonContainer->width());
    if(!hasMinimize && !hasMaximize && !hasClose)
    {
        uiTitlebar->btnMinimize->setVisible(false);
        uiTitlebar->btnZoom->setVisible(false);
        uiTitlebar->btnClose->setVisible(false);
    }
    else
    {
        uiTitlebar->btnMinimize->setVisible(true);
        uiTitlebar->btnZoom->setVisible(true);
        uiTitlebar->btnClose->setVisible(true);
    }
    setTitleBarText(mTitle);
}


void OriginTitlebarOSX::setSystemMenuEnabled(const bool& enable)
{
    mSystemMenuEnabled = enable;
}


void OriginTitlebarOSX::setActiveWindowStyle(const bool& isActive)
{
    QVariant variant = isActive ? QVariant::Invalid : QVariant("inactive");
    uiTitlebar->btnClose->setProperty("style", variant);
    uiTitlebar->btnClose->setStyle(QApplication::style());
    uiTitlebar->btnMinimize->setProperty("style", variant);
    uiTitlebar->btnMinimize->setStyle(QApplication::style());
    uiTitlebar->btnZoom->setProperty("style", variant);
    uiTitlebar->btnZoom->setStyle(QApplication::style());
    uiTitlebar->btnFullscreen->setProperty("style", variant);
    uiTitlebar->btnFullscreen->setStyle(QApplication::style());
	uiTitlebar->lblWindowTitle->setProperty("style", variant);
    uiTitlebar->lblWindowTitle->setStyle(QApplication::style());
}
    

void OriginTitlebarOSX::removeZoomHover()
{
    // Why won't it just detect that the mouse isn't hovering?!
    // HACK: Force it to remove the hover on the button.
    QEvent event = QEvent(QEvent::Leave);
    QApplication::sendEvent(uiTitlebar->buttonContainer, &event);
    QApplication::sendEvent(uiTitlebar->btnZoom, &event);
}
    
    
bool OriginTitlebarOSX::isHoveringInButtonContainer(QWidget* w)
{
    return w->parentWidget() && w->parentWidget()->objectName() == "buttonContainer";
}


void OriginTitlebarOSX::setButtonContainerHoverStyle(const bool& isHovering)
{
    if(isHovering)
    {
        uiTitlebar->btnClose->setProperty("style", QVariant::Invalid);
        if(uiTitlebar->btnClose->isEnabled())
            uiTitlebar->btnClose->setProperty("buttonContainer", QVariant("hovering"));
        uiTitlebar->btnClose->setStyle(QApplication::style());
        
        uiTitlebar->btnMinimize->setProperty("style", QVariant::Invalid);
        if(uiTitlebar->btnMinimize->isEnabled())
            uiTitlebar->btnMinimize->setProperty("buttonContainer", QVariant("hovering"));
        uiTitlebar->btnMinimize->setStyle(QApplication::style());
        
        uiTitlebar->btnZoom->setProperty("style", QVariant::Invalid);
        if(uiTitlebar->btnZoom->isEnabled())
            uiTitlebar->btnZoom->setProperty("buttonContainer", QVariant("hovering"));
        uiTitlebar->btnZoom->setStyle(QApplication::style());
    }
    else
    {
        uiTitlebar->btnClose->setProperty("buttonContainer", QVariant::Invalid);
        uiTitlebar->btnClose->setStyle(QApplication::style());
        // If isn't not an active window and we removed the inactive style
        // - re-apply the inactive style
        if(mWindowPartner && mWindowPartner->isActiveWindow() == false)
        {
            uiTitlebar->btnClose->setProperty("style", QVariant("inactive"));
            uiTitlebar->btnClose->setStyle(QApplication::style());
        }
        
        uiTitlebar->btnMinimize->setProperty("buttonContainer", QVariant::Invalid);
        uiTitlebar->btnMinimize->setStyle(QApplication::style());
        // If isn't not an active window and we removed the inactive style
        // - re-apply the inactive style
        if(mWindowPartner && mWindowPartner->isActiveWindow() == false)
        {
            uiTitlebar->btnMinimize->setProperty("style", QVariant("inactive"));
            uiTitlebar->btnMinimize->setStyle(QApplication::style());
        }
        
        uiTitlebar->btnZoom->setProperty("buttonContainer", QVariant::Invalid);
        uiTitlebar->btnZoom->setStyle(QApplication::style());
        // If isn't not an active window and we removed the inactive style
        // - re-apply the inactive style
        if(mWindowPartner && mWindowPartner->isActiveWindow() == false)
        {
            uiTitlebar->btnZoom->setProperty("style", QVariant("inactive"));
            uiTitlebar->btnZoom->setStyle(QApplication::style());
        }
    }
}


bool OriginTitlebarOSX::eventFilter(QObject* obj, QEvent* event)
{
    switch(event->type())
    {
    case QEvent::Enter:
        if(uiTitlebar && obj == uiTitlebar->buttonContainer)
            setButtonContainerHoverStyle(true);
        break;

    case QEvent::Leave:
        if(uiTitlebar && obj == uiTitlebar->buttonContainer)
            setButtonContainerHoverStyle(false);
        break;

	case QEvent::Resize:
        if(mTitle.isEmpty() == false) setTitleBarText(mTitle);
        break;
    
    case QEvent::Show:
    case QEvent::Hide:
    case QEvent::WindowStateChange:
        if(mWindowPartner && obj == mWindowPartner)
        {
            // HACK: REMOVE THE HOVER FROM THE BUTTONS. GRRR
            QApplication::postEvent(uiTitlebar->btnZoom, new QEvent(QEvent::Leave), Qt::HighEventPriority);
            QApplication::postEvent(uiTitlebar->btnMinimize, new QEvent(QEvent::Leave), Qt::HighEventPriority);
            QApplication::postEvent(uiTitlebar->btnClose, new QEvent(QEvent::Leave), Qt::HighEventPriority);
            
            // When a window is minimized all the buttons show their hover state
            // unless they are disabled
            QVariant variant = QVariant::Invalid;
            if(mWindowPartner->windowState() & Qt::WindowMinimized)
            {
                variant = QVariant("hovering");
                uiTitlebar->btnZoom->setProperty("style", QVariant::Invalid);
                uiTitlebar->btnMinimize->setProperty("style", QVariant::Invalid);
                uiTitlebar->btnClose->setProperty("style", QVariant::Invalid);
            }
            if(uiTitlebar->btnMinimize->isEnabled())
                uiTitlebar->btnMinimize->setProperty("buttonContainer", variant);
            if(uiTitlebar->btnClose->isEnabled())
                uiTitlebar->btnClose->setProperty("buttonContainer", variant);
            if(uiTitlebar->btnZoom->isEnabled())
                uiTitlebar->btnZoom->setProperty("buttonContainer", variant);
            uiTitlebar->btnZoom->setStyle(QApplication::style());
            uiTitlebar->btnMinimize->setStyle(QApplication::style());
            uiTitlebar->btnClose->setStyle(QApplication::style());
        }
        break;
		
    default:
        break;
    }

    return OriginTitlebarBase::eventFilter(obj, event);
}

QVector<QObject*> OriginTitlebarOSX::buttons() const
{
    QVector<QObject*> actionButtons;

    actionButtons.append(uiTitlebar->btnMinimize);
    actionButtons.append(uiTitlebar->btnZoom);
    actionButtons.append(uiTitlebar->btnClose);
    actionButtons.append(uiTitlebar->btnFullscreen);

    return actionButtons;
}
    
}
}
