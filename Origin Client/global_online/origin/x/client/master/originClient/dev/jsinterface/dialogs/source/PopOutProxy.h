/////////////////////////////////////////////////////////////////////////////
// PopOutProxy.h
//
// Copyright (c) 2015, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef _PopOutProxy_H
#define _PopOutProxy_H

#include <QObject>
#include <QVector>
#include <QVariant>
#include <QJsonObject>
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

    class ORIGIN_PLUGIN_API PopOutProxy : public QObject
{
    Q_OBJECT

public:
    PopOutProxy(QObject* parent = 0);

signals:
    void popOutClosed(QString);
    void popInWindow(QString);

};

}
}
}

#endif