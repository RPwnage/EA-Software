// *********************************************************************
//  IGOBroadcastWidget.cpp
//  Copyright (c) 2012 Electronic Arts Inc. All rights reserved.
// **********************************************************************

#include "IGOBroadcastingWidget.h"
#include "ui_IGOBroadcastingWidget.h"
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "OriginCommonUIUtils.h"
#include <QEvent>
#include <QPainter>

namespace Origin
{
namespace Client
{

IGOBroadcastingWidget::IGOBroadcastingWidget(QWidget* parent)
: Engine::IGOQWidget(parent)
, ui(new Ui::IGOBroadcastingWidget)
{
    ui->setupUi(this);
    onBroadcastDurationChanged(0);
    onBroadcastStreamInfoChanged(0);
#ifdef ORIGIN_PC
    Engine::IGOTwitch* igoTwitch = Engine::IGOController::instance()->twitch();
    if(igoTwitch)
    {
        ORIGIN_VERIFY_CONNECT(igoTwitch, SIGNAL(broadcastStreamInfoChanged(int)), this, SLOT(onBroadcastStreamInfoChanged(const int&)));
        ORIGIN_VERIFY_CONNECT(igoTwitch, SIGNAL(broadcastDurationChanged(int)), this, SLOT(onBroadcastDurationChanged(const int&)));
        ORIGIN_VERIFY_CONNECT(igoTwitch, SIGNAL(broadcastIndicatorChanged(bool)), this, SLOT(onBroadcastIndicatorChanged(const bool&)));
    }
#endif
}


IGOBroadcastingWidget::~IGOBroadcastingWidget()
{
    Engine::IGOController::instance()->igowm()->removeWindow(ivcView(), true);
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


bool IGOBroadcastingWidget::event(QEvent* event)
{
    switch(event->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;

    default:
        break;
    }

    return IGOQWidget::event(event);
}


void IGOBroadcastingWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


void IGOBroadcastingWidget::resizeEvent(QResizeEvent* e)
{
#ifdef ORIGIN_PC
    if(Engine::IGOController::instance()->twitch())
    {
        ui->lblNumViewers->setText(QString::number(Engine::IGOController::instance()->twitch()->broadcastViewers()));
    }
#endif
    OriginCommonUIUtils::DrawRoundCorner(this, 6);
    QWidget::resizeEvent(e);
}


void IGOBroadcastingWidget::onBroadcastStreamInfoChanged(const int& num)
{
    ui->lblNumViewers->setText(QString::number(num));
    adjustSize();
}


void IGOBroadcastingWidget::onBroadcastDurationChanged(const int& duration)
{
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    if(duration > 0)
    {
        hours = duration / 3600;
        const int hoursInSeconds = hours * 3600;
        minutes = (duration - hoursInSeconds) / 60;
        const int minutesInSeconds = minutes * 60;
        seconds = duration - hoursInSeconds - minutesInSeconds;
    }
    const QString timeString = QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0'));
    ui->lblDuration->setText(timeString);
    adjustSize();
}


void IGOBroadcastingWidget::onBroadcastIndicatorChanged(const bool& visible)
{
    if(visible)
    {
        Engine::IIGOWindowManager::WindowBehavior behavior;
        behavior.cacheMode = QGraphicsItem::ItemCoordinateCache;
        behavior.persistent = true;
        behavior.dontStealFocus = true;
        behavior.alwaysOnTop = true;
        behavior.alwaysVisible = true;
        behavior.disableDragging = true;
        behavior.disableResizing = true;
        behavior.singleton = true;
        Engine::IIGOWindowManager::WindowProperties settings;
        settings.setPosition(QPoint(0, 25), Engine::IIGOWindowManager::WindowDockingOrigin_X_CENTER);

        if(!Engine::IGOController::instance()->igowm()->showWindow(ivcView(), behavior.alwaysVisible))
        {
            Engine::IGOController::instance()->igowm()->addWindow(ivcView(), settings, behavior);
        }

        // force re-layouting
        Engine::IGOController::instance()->igowm()->showWindow(ivcView(), behavior.alwaysVisible);
    }
    else
    {
        Engine::IGOController::instance()->igowm()->hideWindow(ivcView(), true);
    }
}

} // Client
} // Origin