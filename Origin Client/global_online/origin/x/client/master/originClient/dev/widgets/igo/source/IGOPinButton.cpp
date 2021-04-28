#include "IGOPinButton.h"

#include "engine/igo/IGOController.h"

#include "services/debug/DebugService.h"
#include "IGOTooltipSliderWidget.h"
#include <QApplication>

namespace Origin
{
namespace Client
{

IGOPinButton::IGOPinButton(const Origin::Client::UIScope& context, QWidget* parent)
: QPushButton(parent)
, mIGOTooltipSlider(NULL)
, mContext(context)
, mTimerID(-1)
{
    setObjectName("IGOPinButton");
    setFixedSize(QSize(26, 21));
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    setChecked(false);

    mIGOTooltipSlider = new IGOTooltipSliderWidget();
    ORIGIN_VERIFY_CONNECT(mIGOTooltipSlider, SIGNAL(requestVisibilityChange(const bool&)), this, SLOT(onSliderRequestVisibilityChange(const bool&)));
    ORIGIN_VERIFY_CONNECT(mIGOTooltipSlider, SIGNAL(active()), this, SLOT(onSliderActive()));
    ORIGIN_VERIFY_CONNECT(mIGOTooltipSlider, SIGNAL(mousePressed()), this, SIGNAL(pinningStarted()));
    ORIGIN_VERIFY_CONNECT(mIGOTooltipSlider, SIGNAL(mouseReleased()), this, SIGNAL(pinningFinished()));
    ORIGIN_VERIFY_CONNECT(mIGOTooltipSlider, SIGNAL(sliderChanged(const int&)), this, SIGNAL(sliderChanged(const int&)));
    mIGOTooltipSlider->installEventFilter(this);
    mIGOTooltipSlider->hide();

    ORIGIN_VERIFY_CONNECT(this, SIGNAL(toggled(bool)), this, SLOT(onToggle(const bool&)));
}


IGOPinButton::~IGOPinButton()
{
    disconnect();
    if (mIGOTooltipSlider)
    {
        // TODO: revisit this
        // I don't know why we are doing a disconnect here (ie not allowing the OIG widget to get destroyed!
        // -> remove from OIG if necessary
        Origin::Engine::IGOController::instance()->igowm()->removeWindow(mIGOTooltipSlider, false);
        
        mIGOTooltipSlider->disconnect();
        mIGOTooltipSlider->deleteLater();
    }
    mIGOTooltipSlider = NULL;
}


void IGOPinButton::setPinned(bool pinned)
{
    ORIGIN_VERIFY_DISCONNECT(this, SIGNAL(toggled(bool)), this, SLOT(onToggle(const bool&)));
    setChecked(pinned);
    ORIGIN_VERIFY_CONNECT(this, SIGNAL(toggled(bool)), this, SLOT(onToggle(const bool&)));
}

bool IGOPinButton::pinned() const
{
    return isChecked();
}

void IGOPinButton::setVisibility(bool visible)
{
    hideSlider();
    setVisible(visible);
    
    UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(parent());
    if (window)
        window->setTitlebarItems(visible ? (UIToolkit::OriginWindow::Icon | UIToolkit::OriginWindow::Close) : UIToolkit::OriginWindow::Icon);
}

void IGOPinButton::setSliderValue(int percent)
{
    mIGOTooltipSlider->setSliderValue(percent);
}

int IGOPinButton::sliderValue() const
{
    return mIGOTooltipSlider->sliderValue();
}

void IGOPinButton::onToggle(const bool& checked)
{
    if (checked)
    {
        emit pinningStarted();
        showSlider();
    }  
    else
    {
        hideSlider();
    }
    emit pinningToggled();
}


void IGOPinButton::onSliderRequestVisibilityChange(const bool& visible)
{
    if(visible)
    {
        showSlider();
    }
    else
    {
        hideSlider();
    }
}


void IGOPinButton::onSliderActive()
{
    setProperty("style", QVariant("sliderActive"));
    setStyle(QApplication::style());
}


bool IGOPinButton::event(QEvent* e)
{
    switch(e->type())
    {
    case QEvent::Enter:
        {
            mTimerID = startTimer(350);
        }
        break;
    case QEvent::MouseButtonPress:
        {
            killTimer(mTimerID);
            mTimerID = -1;
        }
        break;
    case QEvent::Timer:
        if(mTimerID != -1)
        {
            killTimer(mTimerID);
            mTimerID = -1;
            if(this->underMouse() || mIGOTooltipSlider->underMouse())
            {
                showSlider();
            }
            else
            {
                hideSlider();
            }
        }
        break;
    default:
        break;
    }
    return QPushButton::event(e);
}


void IGOPinButton::showSlider()
{
    if(isChecked() && mIGOTooltipSlider)
    {
        if (mContext == Origin::Client::IGOScope)
        {
            Origin::Engine::IIGOWindowManager* igowm = Origin::Engine::IGOController::instance()->igowm();
            
            Origin::Engine::IIGOWindowManager::WindowProperties btnProperties;
            if (igowm->windowProperties(this, &btnProperties))
            {
                // Do we need to create this guy first?
                Origin::Engine::IIGOWindowManager::WindowProperties sliderProperties;
                bool validSlider = igowm->windowProperties(mIGOTooltipSlider, &sliderProperties);
                if (!validSlider)
                {
                    Origin::Engine::IIGOWindowManager::WindowBehavior sliderBehavior;
                    sliderBehavior.dontStealFocus = true;
                    sliderBehavior.dontblockRenderingWhilePinningInProgress = true;

                    if (igowm->addPopupWindow(mIGOTooltipSlider, sliderProperties, sliderBehavior))
                    {
                        igowm->windowProperties(mIGOTooltipSlider, &sliderProperties);
                        validSlider = true;
                    }
                }
                
                if (validSlider)
                {
                    QSize sliderSize = sliderProperties.size();
                    
                    const int BUTTON_Y_BORDER = 4;
                    const QPoint offset = this->mapTo(this->topLevelWidget(), QPoint(0, 0));
                    int x = btnProperties.position().x () + offset.x() + (width()/2) - (sliderSize.width()/2);
                    int y = btnProperties.position().y() + offset.y() + height() - BUTTON_Y_BORDER;
                    
                    igowm->setWindowPosition(mIGOTooltipSlider, QPoint(x, y));
                    igowm->setWindowFocusWidget(mIGOTooltipSlider);
                    
                    // Set a timer to check if we are still hovered over in a second or so
                    mTimerID = startTimer(500);
                }
            }
        }
        else
        {
            const QPoint offset = this->mapToGlobal(geometry().bottomLeft());
            const int BUTTON_Y_BORDER = 7;
            const QPoint containerPos(offset.x() + (width()/2) - (mIGOTooltipSlider->width()/2), offset.y() - BUTTON_Y_BORDER);
            mIGOTooltipSlider->move(containerPos);
            mIGOTooltipSlider->show();
        }
    }
}


void IGOPinButton::hideSlider()
{
    if(mIGOTooltipSlider && mIGOTooltipSlider->isVisible())
    {
        if (mContext == Origin::Client::IGOScope)
            Origin::Engine::IGOController::instance()->igowm()->removeWindow(mIGOTooltipSlider, false);
        
        if (mTimerID != -1)
        {
           killTimer(mTimerID);
           mTimerID = -1;
        }

        mIGOTooltipSlider->hide();
        setProperty("style", QVariant::Invalid);
        setStyle(QApplication::style());
        emit pinningFinished();
    }
}


void IGOPinButton::raiseSlider()
{
    Origin::Engine::IGOController::instance()->igowm()->moveWindowToFront(mIGOTooltipSlider);
}
    
} // Client
} // Origin
