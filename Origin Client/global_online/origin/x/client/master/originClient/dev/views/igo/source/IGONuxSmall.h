///////////////////////////////////////////////////////////////////////////////
// IGONuxSmall.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef IGONUXSMALL_H
#define IGONUXSMALL_H

#include <QWidget>

#include "services/plugin/PluginAPI.h"

namespace Ui 
{
    class IGONuxSmall;
}

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API IGONuxSmall : public QWidget
{
Q_OBJECT

public:
    IGONuxSmall(QWidget *parent = 0);
    virtual ~IGONuxSmall();

    virtual void paintEvent(QPaintEvent *);
    void setHotKey(const QString &hotkey);

signals:
    void igoSetOpacity(qreal);

private:
    Ui::IGONuxSmall* ui;

    QTimer *mTimer;
    int mCount;

private slots:
    void updateShow();
    void updateHide();
    void startFadeOut();
    void closeNux(bool active);
};

} // Client
} // Origin

#endif
