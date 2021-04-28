///////////////////////////////////////////////////////////////////////////////
// SettingsChangeEventHandler.h
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef SETTINGSCHANGEEVENTHANDLER_H
#define SETTINGSCHANGEEVENTHANDLER_H

#include <QObject>
#include "services/settings/SettingsManager.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace UIToolkit
{
    class OriginWindow;
}
namespace Client
{

class ORIGIN_PLUGIN_API SettingsChangeEventHandler : public QObject
{
    Q_OBJECT

public:
	// Give it a parent - something to die with.
    SettingsChangeEventHandler(QObject* parent, const int& context = 0);
    ~SettingsChangeEventHandler();
	

private slots:
    void onSettingChanged(QString const&, Origin::Services::Variant const&);
    void onRemoveWindow();

private:
    void showLocaleResetWindow();
    void pushAndShowWindow(UIToolkit::OriginWindow* window, const QString& key);

    const int mContext;
    QMap<QString, UIToolkit::OriginWindow*> mWindowMap;
};

} // Client
} // Origin
#endif