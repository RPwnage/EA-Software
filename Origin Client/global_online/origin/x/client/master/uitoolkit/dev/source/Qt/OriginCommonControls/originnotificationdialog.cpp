#include "originnotificationdialog.h"
#include "ui_originchromebase.h"
#include <QTimer>
#include <QMouseEvent>
#include <QLabel>
#include <QResizeEvent>
#include <QHideEvent>
#include <QMargins>

namespace Origin
{
namespace UIToolkit
{

QString OriginNotificationDialog::mPrivateStyleSheet = QString("");

namespace
{
    int TIMER_FADE_TICK = 0;
    const qreal POPUP_FADEIN_INCREMENT = 0.25f;
    const qreal POPUP_FADEOUT_INCREMENT = 0.05f;
    const int POPUP_OPACITY_TRANSPARENT = 0;
    const int POPUP_OPACITY_SOLID = 1;
    const int POPUP_CLOSE_TIMER_HOVER = 1500;
}


OriginNotificationDialog::OriginNotificationDialog(QWidget* content, QWidget* parent) 
: OriginChromeBase(parent)
, mOpacity(POPUP_OPACITY_TRANSPARENT)
, mMaxOpacity(POPUP_OPACITY_SOLID)
, mTimer(new QTimer(this))
, mBubbleLabel(NULL)
, mIGOMode(false)
, mClosing(false)
, mCloseTime(3000) // default to 3 seconds
{
    uiBase->setupUi(this);

    setOriginElementName("OriginNotificationDialog");
    m_children.append(this);
    setContent(content);

    mBubbleLabel = new QLabel(this);
    mBubbleLabel->setObjectName("originBubble");
    mBubbleLabel->setPixmap(QPixmap(":/OriginCommonControls/OriginNotificationDialog/notificationBubble.png"));
    mBubbleLabel->setFixedSize(QSize(18, 23));

    QMargins gMargings = uiBase->gridLayout->contentsMargins();
    gMargings.setLeft(15);
    uiBase->gridLayout->setContentsMargins(gMargings);
    mLifeTimer = new QTime();
    mShouldAcceptClick = true;
    setOpacity(mOpacity);
}


OriginNotificationDialog::~OriginNotificationDialog() 
{
    removeElementFromChildList(this);

    // Stop the timer and disconnect all the timer slots
    mTimer->stop();
    disconnect(mTimer, SIGNAL(timeout()), this, NULL);
    delete mLifeTimer;
}



QString& OriginNotificationDialog::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginNotificationDialog::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

void OriginNotificationDialog::setFadeInTime(const int& fadeInTime)
{
    mTimer->stop();
    qreal fade_in_steps = 1/POPUP_FADEIN_INCREMENT;
    qreal fade_out_steps = 1/POPUP_FADEOUT_INCREMENT;
    int total_steps = fade_in_steps + fade_out_steps;
    TIMER_FADE_TICK = fadeInTime/total_steps;
}


void OriginNotificationDialog::beginFadeOut()
{
    mTimer->stop();
    disconnect(mTimer, SIGNAL(timeout()), this, SLOT(beginFadeOut()));
    setOpacity(mMaxOpacity);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(fadeOutTick()));
    mTimer->start(TIMER_FADE_TICK);
}


void OriginNotificationDialog::fadeInTick()
{
    setOpacity(mOpacity + POPUP_FADEIN_INCREMENT);

    if(mOpacity >= mMaxOpacity) 
    {
        mTimer->stop();
        setOpacity(mMaxOpacity);
        disconnect(mTimer, SIGNAL(timeout()), this, SLOT(fadeInTick()));

        // Set a 3 second timer to keep the notification on-screen
        // ***  this gets reset if another notification comes in or if
        //      the user hovers over the notification window
        // Check to ensure we have a valid close time.
        // For example Incoming voice call notifications will have a -1
        if (mCloseTime > 0)
        {
            connect(mTimer, SIGNAL(timeout()), this, SLOT(beginFadeOut()));
            mTimer->start(mCloseTime);
        }
    }
}


void OriginNotificationDialog::fadeOutTick()
{
    setOpacity(mOpacity - POPUP_FADEOUT_INCREMENT);
    if(mOpacity <= POPUP_OPACITY_TRANSPARENT) 
    {
        mTimer->stop();
        disconnect(mTimer, SIGNAL(timeout()), this, NULL);

        // Use a call to accept() or reject() here instead of close() to make sure the finished() signal is triggered (close() will only
        // do that if your widget is visible, which may not be the case if, for example, the widget was embedded in IGO and we detach it
        // -> we reset the parent to NULL -> Qt will automatically hide the widget
        reject();
    }
}


void OriginNotificationDialog::enterEvent(QEvent*) 
{
    mTimer->stop();
    disconnect(mTimer, SIGNAL(timeout()), this, NULL);
    setOpacity(mMaxOpacity);
}


void OriginNotificationDialog::leaveEvent(QEvent*) 
{
    // We get this event after closing the notifiction,
    // but we dont' want to re-set this timer.
    if(!mClosing)
    {
        // Restart the timer to dismiss the notification after a few seconds
        disconnect(mTimer, SIGNAL(timeout()), this, NULL);
        connect(mTimer, SIGNAL(timeout()), this, SLOT(beginFadeOut()));
        mTimer->start(POPUP_CLOSE_TIMER_HOVER);
    }
}


void OriginNotificationDialog::setOpacity(qreal opacity) 
{
    mOpacity = opacity;
    if (mIGOMode)
    {
        emit opacityChanged(mOpacity);
    }
    else
    {
        setWindowOpacity(mOpacity);
    }
}

void OriginNotificationDialog::setInitalOpacity(qreal opacity)
{
    mOpacity = opacity;
    setOpacity(mOpacity);
}

/* This function needs to be overwritten in order to prevent
   the QWidget's function from supressing the QMouseEvent from
   propagating to mouseReleaseEvent function
   Specifically the QGraphicsScene (used in OIG) will clear the 
   list of mouse grabbers if this event is not accepted.
*/
void OriginNotificationDialog::mousePressEvent( QMouseEvent * mouseEvent)
{
    QWidget::mousePressEvent(mouseEvent);
    mouseEvent->accept();
    return;
}


void OriginNotificationDialog::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if(mouseEvent && mouseEvent->button() == Qt::LeftButton)
    {
        windowClicked();
    }
}


void OriginNotificationDialog::showEvent(QShowEvent* event)
{
    mLifeTimer->start();
    mClosing = false;
    // Start the fade in effect if we are suppose to have it.
    if(TIMER_FADE_TICK > 0)
    {
        disconnect(mTimer, SIGNAL(timeout()), this, NULL);
        connect(mTimer, SIGNAL(timeout()), this, SLOT(fadeInTick()));
        mTimer->start(TIMER_FADE_TICK);
    }
    QDialog::showEvent(event);
    mBubbleLabel->raise();
    uiBase->outerFrame->stackUnder(mBubbleLabel);
    uiBase->innerFrame->stackUnder(mBubbleLabel);
}


void OriginNotificationDialog::resizeEvent(QResizeEvent* event)
{
    if(mBubbleLabel)
    {
        QSize newSize = event->size();
        // move the bubble to be halfway down the page
        int newY = newSize.height()/2 - 10;
        mBubbleLabel->move(9, newY);
    }
}


void OriginNotificationDialog::windowClicked()
{
    if(mShouldAcceptClick)
    {
        emit clicked();
        mOpacity = 0;
        // Disconnect and stop out timer since we are closing
        disconnect(mTimer, SIGNAL(timeout()), 0, 0);
        mTimer->stop();
        mClosing = true;
        // Use a call to accept() or reject() here instead of close() to make sure the finished() signal is triggered (close() will only
        // do that if your widget is visible, which may not be the case if, for example, the widget was embedded in IGO and we detach it
        // -> we reset the parent to NULL -> Qt will automatically hide the widget
        accept();
    }
}

void OriginNotificationDialog::windowClosed()
{
        mOpacity = 0;
        // Disconnect and stop out timer since we are closing
        disconnect(mTimer, SIGNAL(timeout()), 0, 0);
        mTimer->stop();
        mClosing = true;
        // Use a call to accept() or reject() here instead of close() to make sure the finished() signal is triggered (close() will only
        // do that if your widget is visible, which may not be the case if, for example, the widget was embedded in IGO and we detach it
        // -> we reset the parent to NULL -> Qt will automatically hide the widget
        reject();
}

void OriginNotificationDialog::showBubble(bool doShow)
{
    mBubbleLabel->setVisible(doShow);
}


// This is called when IGO is activated/deactivated
void OriginNotificationDialog::setIGOMode(bool igoMode)
{
    mIGOMode = igoMode;
    if(mIGOMode)// If we are opening OIG we need to let everyone else know
        emit igoActivated();
    else // If we are closing OIG we need to remove all notifications
    {
        // Use a call to accept() or reject() here instead of close() to make sure the finished() signal is triggered (close() will only
        // do that if your widget is visible, which may not be the case if, for example, the widget was embedded in IGO and we detach it
        // -> we reset the parent to NULL -> Qt will automatically hide the widget
        reject();
    }
}

void OriginNotificationDialog::setInitalIGOMode(bool igoMode)
{
        mIGOMode = igoMode;
}

// This is called when the notificatoin is created and tells us a game is focused
void OriginNotificationDialog::setIGOFlags(bool isIGO)
{
    QString value = (isIGO) ? "true" :"false";
    uiBase->outerFrame->setProperty("isIGO", value);
    uiBase->innerFrame->setProperty("isIGO", value);

    uiBase->outerFrame->setStyle(QApplication::style());
    uiBase->innerFrame->setStyle(QApplication::style());
}


void OriginNotificationDialog::activated()
{
    windowClicked();
}

void OriginNotificationDialog::setCloseTimer(int time)
{
    mCloseTime = time;
}

void OriginNotificationDialog::onActionAccepted()
{
    emit actionAccepted();
    mOpacity = 0;
    // Disconnect and stop out timer since we are closing
    disconnect(mTimer, SIGNAL(timeout()), 0, 0);
    mTimer->stop();
    mClosing = true;
    // Use a call to accept() or reject() here instead of close() to make sure the finished() signal is triggered (close() will only
    // do that if your widget is visible, which may not be the case if, for example, the widget was embedded in IGO and we detach it
    // -> we reset the parent to NULL -> Qt will automatically hide the widget
    accept();
}

void OriginNotificationDialog::onActionRejected()
{
    emit actionRejected();
    windowClosed();
}

}
}
