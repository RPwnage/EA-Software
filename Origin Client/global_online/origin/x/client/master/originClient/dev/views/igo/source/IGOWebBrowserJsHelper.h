///////////////////////////////////////////////////////////////////////////////
// IGOWEBBROWSERJSHELPER.h
// 
// Created by Thomas Bruckschlegel
// Copyright (c) 2013 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOWEBBROWSERJSHELPER_H
#define IGOWEBBROWSERJSHELPER_H

#include <QObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

class ORIGIN_PLUGIN_API IGOWebBrowserJsHelper :
    public QObject
{
    Q_OBJECT

public:
    IGOWebBrowserJsHelper( QObject *parent = 0 );
    ~IGOWebBrowserJsHelper(void);
public slots: 
    void closeIGO();
    void close();
    void close(QString token);
    void openLinkInNewOIGBrowser(QString url);
signals:
    void closeIGOBrowserTab();
    void closeIGOBrowserByTwitch(QString token);
};

} // Client
} // Origin

#endif
