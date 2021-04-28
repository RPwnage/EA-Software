/////////////////////////////////////////////////////////////////////////////
// PlayView.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef PLAYVIEW_H
#define PLAYVIEW_H

#include <QWidget>
#include <QJsonObject>

#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace UIToolkit
{
    class OriginTransferBar;
}

namespace Client
{

class ORIGIN_PLUGIN_API PlayView : public QObject
{
    Q_OBJECT
    
public:
    static QJsonObject commandLinkObj(const QString& option, const QString& icon, const QString& title, const QString& text, const QString& bottomText = "");
};

class ORIGIN_PLUGIN_API OIGSettingsView : public QWidget
{
    Q_OBJECT
    
public:
    OIGSettingsView(QWidget* parent = 0);
    bool isChecked() const;
};

}
}

#endif