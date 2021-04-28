///////////////////////////////////////////////////////////////////////////////
// IGONuxSmall.cpp
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#include "IGONuxSmall.h"

#include <QTimer>
#include <QPainter>
#include "ui_IGONuxSmall.h"

// 10 seconds for nux timeout
#define NUX_TIMEOUT 5000
#define NUX_FADEOUT_TIME_INTERVAL 50

#if !defined(ORIGIN_MAC)

#define NUX_FADEOUT_TIME 500
#define NUX_FADEIN_TIME 500

#else

#define NUX_FADEOUT_TIME 2000
#define NUX_FADEIN_TIME 2000

#endif

namespace Origin
{
namespace Client
{

IGONuxSmall::IGONuxSmall(QWidget *parent) : 
    QWidget(parent),
    ui(new Ui::IGONuxSmall()),
    mTimer(new QTimer()),
    mCount(0)
{
    ui->setupUi(this);

    // start the countdown timer
    mTimer->start(NUX_FADEOUT_TIME_INTERVAL);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(updateShow()));
}


IGONuxSmall::~IGONuxSmall()
{
    delete mTimer;
    delete ui;
    ui = NULL;
}

void IGONuxSmall::paintEvent(QPaintEvent *)
{
    // to allow QWidget to draw background css
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


void IGONuxSmall::setHotKey(const QString &hotkey)
{
    if (ui && ui->hotkey)
        ui->hotkey->setText(tr("ebisu_client_igo_nux_notification").arg(hotkey));
}


void IGONuxSmall::updateShow()
{
    mCount += NUX_FADEOUT_TIME_INTERVAL;

    qreal alpha = (qreal)mCount/(qreal)NUX_FADEOUT_TIME;
    emit igoSetOpacity(alpha);

    if (mCount >= NUX_FADEIN_TIME)
    {
        disconnect(mTimer, SIGNAL(timeout()), this, SLOT(updateShow()));
        connect(mTimer, SIGNAL(timeout()), this, SLOT(startFadeOut()));
        mTimer->start(NUX_TIMEOUT);
    }
}

void IGONuxSmall::updateHide()
{
    if (mCount <= 0)
    {
        mCount = 0;
        mTimer->stop();
        return;
    }
    mCount -= NUX_FADEOUT_TIME_INTERVAL;

    qreal alpha = (qreal)mCount/(qreal)NUX_FADEOUT_TIME;
    emit igoSetOpacity(alpha);

    if (mCount <= 0)
    {
        mTimer->stop();
        close();
    }
}

void IGONuxSmall::startFadeOut()
{
    disconnect(mTimer, SIGNAL(timeout()), this, SLOT(startFadeOut()));
    connect(mTimer, SIGNAL(timeout()), this, SLOT(updateHide()));
    mTimer->start(NUX_FADEOUT_TIME_INTERVAL);
}

void IGONuxSmall::closeNux(bool active)
{
    if (active)
        close();
}
    
} // Client
} // Origin

