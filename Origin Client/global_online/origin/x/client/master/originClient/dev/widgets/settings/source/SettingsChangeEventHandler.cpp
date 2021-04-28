//////////////////////////////////////////////////////////////////////////////
// SettingsChangeEventHandler.cpp
//
// Copyright (c) 2011 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#include "SettingsChangeEventHandler.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "OriginApplication.h"
#include "originwindow.h"
#include "originmsgbox.h"
#include "TelemetryAPIDLL.h"

namespace Origin
{
namespace Client
{

SettingsChangeEventHandler::SettingsChangeEventHandler(QObject* parent, const int& context)
: QObject(parent)
, mContext(context)
{
    ORIGIN_VERIFY_CONNECT(
        Services::SettingsManager::instance(), SIGNAL(settingChanged(QString const&, Origin::Services::Variant const&)),
        this, SLOT(onSettingChanged(QString const&, Origin::Services::Variant const&)));
}


SettingsChangeEventHandler::~SettingsChangeEventHandler()
{
    UIToolkit::OriginWindow* dialog = NULL;
    while (!mWindowMap.empty())
    {
        QMap<QString, Origin::UIToolkit::OriginWindow*>::iterator i = mWindowMap.begin();
        dialog = (*i);
        // Remove it from the map
        ORIGIN_VERIFY_DISCONNECT(dialog, NULL, this, NULL);
        mWindowMap.erase(i);
        if (dialog)
            dialog->deleteLater();
    }
}


void SettingsChangeEventHandler::onSettingChanged(QString const& settingName, Origin::Services::Variant const& val)
{
    if(settingName == Services::SETTING_LOCALE.name())
    {
        const QString oldLang = OriginApplication::instance().locale();
        const QString newLang = Services::readSetting(Services::SETTING_LOCALE);
        // stops the notification if we reset and also if they somehow manage to choose it back.
        if (oldLang != newLang)
        {
            if(LogoutExitFlow::canLogout())
            {
                ORIGIN_LOG_ACTION << "User selected language change = " << newLang;
                showLocaleResetWindow();
            }
            else
            {
                // Fail safe: Show a message that will alert the user that they can't change their
                // language right now because Origin can't exit
                UIToolkit::OriginWindow::alert(UIToolkit::OriginMsgBox::NoIcon, tr("ebisu_client_settings_uppercase"), 
                                    tr("ebisu_client_locale_error_text"), tr("ebisu_client_ok_uppercase"));
            }
        }
    }
}


void SettingsChangeEventHandler::showLocaleResetWindow()
{
    using namespace UIToolkit;
    const bool changeLang = OriginWindow::prompt(OriginMsgBox::NoIcon, tr("ebisu_client_restart_change_language_title"), 
        tr("ebisu_client_restart_change_language_text").arg(tr("application_name")),
        tr("ebisu_client_restart_now"),
        tr("ebisu_client_cancel"));

    if(changeLang)
    {
        const QString newLang = Services::readSetting(Services::SETTING_LOCALE);
        //  TELEMETRY:  Need to send out settings event on language change
        GetTelemetryInterface()->SetLocale(newLang.toUtf8().data());
        MainFlow::instance()->restart();
    }
    else
    {
        Services::writeSetting(Services::SETTING_LOCALE, OriginApplication::instance().locale());
    }
}


void SettingsChangeEventHandler::pushAndShowWindow(UIToolkit::OriginWindow* window, const QString& key)
{
    if(mWindowMap.contains(key))
    {
        // If it's a duplicate, delete the one that was passed in.
        window->disconnect();
        window->deleteLater();
        window = mWindowMap.value(key);
    }
    else
    {
        ORIGIN_VERIFY_CONNECT(window, SIGNAL(finished(int)), this, SLOT(onRemoveWindow()));
        mWindowMap.insert(key, window);
    }
    window->showWindow();
}


void SettingsChangeEventHandler::onRemoveWindow()
{
    UIToolkit::OriginWindow* window = dynamic_cast<UIToolkit::OriginWindow*>(this->sender());
    if(window)
    {
        const QString key = mWindowMap.key(window);
        mWindowMap.remove(key);
        ORIGIN_VERIFY_DISCONNECT(window, NULL, this, NULL);
        window->close();
    }
}

}
}