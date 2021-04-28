/////////////////////////////////////////////////////////////////////////////
// ToastView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ToastView.h"
#include "ui_ToastView.h"
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/Setting.h"
#include "HotkeyToastView.h"
#include "ActionableToastView.h"
#include "originlabel.h"
#include <QNetworkReply>
// For event handling
#include <QMouseEvent>
#include "originnotificationdialog.h"

namespace Origin
{
namespace Client
{

ToastView::ToastView(const UIScope& context)
: QWidget(NULL)
, ui(new Ui::ToastView)
, mContext(context)
{
	ui->setupUi(this);
    bool igoActive = Engine::IGOController::instance()->isActive();
    if(mContext == IGOScope)
    {
        ui->closeButtonContainer->setVisible(igoActive);
        // in game and IGO notifications have different styling
        if (!igoActive)
        {
            ui->lblTitle->setSpecialColorStyle(UIToolkit::OriginLabel::Color_White);
            ui->lblText->setSpecialColorStyle(UIToolkit::OriginLabel::Color_LightGrey);
            ui->gridLayout->setContentsMargins(0,0,12,9);
        }
    }
#ifdef ORIGIN_MAC
    ui->closeBtnLayout->setContentsMargins(11, 8, 0, 0);
#endif


    ORIGIN_VERIFY_CONNECT(ui->closeButton, SIGNAL(clicked()), this, SIGNAL(closeClicked()));
}


ToastView::~ToastView()
{
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


void ToastView::setupCustomWidgets(QVector<QWidget*> widgets, UIToolkit::OriginNotificationDialog* dialog)
{
    if(!widgets.isEmpty())
    {
        int index = 0;
        foreach(QWidget* widget, widgets)
        {
            ui->customLayout->insertWidget(index, widget);
            // We need to hook up the actionable buttons signals to the toplevel dialog here
            // Since this might be an OIG notification using topLevelWidget() will not work
            ActionableToastView* atv = dynamic_cast<ActionableToastView*>(widget);
            if (atv)
            {
                if (dialog)
                {
                    ORIGIN_VERIFY_CONNECT(atv, SIGNAL(acceptClicked()), dialog, SLOT(onActionAccepted()));
                    ORIGIN_VERIFY_CONNECT(atv, SIGNAL(declineClicked()), dialog, SLOT(onActionRejected()));
                }
            }
        }
    }
}


void ToastView::setIcon(const QPixmap& pixmap)
{
    QPixmap scaledPixmap = pixmap.scaled(28, 28, Qt::KeepAspectRatio);
    ui->icon->setPixmap(scaledPixmap);
}


void ToastView::setIcon(QNetworkReply* reply)
{
    QPixmap p;
    if(reply->error())
    {
        p = QPixmap(":/notifications/defaultorigin.png");
    }
    else 
    {
        QByteArray bArray = reply->readAll();
        p.loadFromData(bArray, "PNG");
    }

    if(p.width() != ui->icon->width())
    {
        p = p.scaled(QSize(28, 28), Qt::KeepAspectRatio);
    }

    emit showParent(topLevelWidget());
}


void ToastView::setText(const QString& title, const QString& message)
{
    setTitle(title);
    setMessage(message);
}


void ToastView::setMessage(const QString& message)
{
    ui->lblText->setVisible(message.count());
    ui->lblText->setText(message);

    if(message.isEmpty())
    {
        // If we have no message we need to space out title properly
        ui->lblTitle->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
        ui->gridLayout->setRowStretch(1,1);
        ui->gridLayout->setRowStretch(2,0);
    }
}


void ToastView::setTitle(const QString& title)
{
    ui->lblTitle->setVisible(title.count());
    ui->lblTitle->setText(title);
}


void ToastView::setName(const QString& name)
{
    mName = name;
    // For the super special case. Only in this case do we always want to show our
    // normal text even if we are in OIG.
    if(mName.compare("POPUP_OIG_HOTKEY") == 0)
    {
        ui->closeButtonContainer->setVisible(Engine::IGOController::instance()->isActive());
        ui->lblTitle->setSpecialColorStyle(UIToolkit::OriginLabel::Color_Normal);
        ui->lblText->setSpecialColorStyle(UIToolkit::OriginLabel::Color_Normal);
    }
}


void ToastView::handleHotkeyLables()
{
    // Layouts do not have children directly so look though the
    // layout's parent widget's children
    QObjectList children = this->children();

    foreach (QObject* child, children)
    {
        HotkeyToastView* hotkeys = dynamic_cast<HotkeyToastView*>(child);
        if(hotkeys)
        {
            hotkeys->handleHotkeyLables();
            hotkeys->setOIGHotkeyStyle(mName.compare("POPUP_OIG_HOTKEY") == 0);
        }
    }
}


int ToastView::removeCustomWidget(QWidget* widget)
{
    int wh = widget->height(); 
    int vh = this->height();
    int newHeight = vh - wh;
    ui->customLayout->removeWidget(widget);
    widget->deleteLater();
    setFixedHeight(newHeight);
    return (newHeight);
}

/* This function needs to be overwritten in order to prevent
   the QWidget's function from supressing the QMouseEvent from
   propagating to mouseReleaseEvent function
   Specifically the QGraphicsScene (used in IGO) will clear the 
   list of mouse grabbers if this event is not accepted.
*/
void ToastView::mousePressEvent( QMouseEvent * mouseEvent)
{
    QWidget::mousePressEvent(mouseEvent);
    mouseEvent->accept();
    return;
}


void ToastView::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    emit clicked();
}

}
}
