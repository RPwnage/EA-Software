#include "ClientSettingsProxy.h"
#include "OriginApplication.h"
#include "services/log/LogService.h"
#include "services/settings/InGameHotKey.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/TrustedClock.h"
#include "services/debug/DebugService.h"
#include "services/voice/VoiceService.h"
#include "LocalHost/LocalHost.h"
#include "LocalHost/LocalHostOriginConfig.h"
#include "ContentFlowController.h"
#include "ClientFlow.h"
#include "ClientViewController.h"
#include "LocalContentViewController.h"
#include "InstallerView.h"
#include "DialogController.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

ClientSettingsProxy::ClientSettingsProxy(QObject *parent) :
    QObject(parent)
{
    ORIGIN_VERIFY_CONNECT(
        Origin::Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
        this, SLOT(onUpdateSettings(const QString&, const Origin::Services::Variant&)));

    if (parent)
    {
        ORIGIN_VERIFY_CONNECT(parent, SIGNAL(hotkeySettingError(const QString&, const QString&)),
            this, SLOT(onSettingsError(const QString&, const QString&)));
    }

    // grab the initial value of LocalHostEnabled in order to hide the "Origin Helper" UI per OFM-11618
    // but don't hide when enabled until after user has logged out and logged back in.
    QVariant var = Services::readSetting(QString("LocalHostEnabled"), Services::Session::SessionService::currentSession()).toQVariant();
    mLocalHostInitialValue = var.toBool();

    // In case we ever want to hide this one also
    var = Services::readSetting(QString("LocalHostResponderEnabled"), Services::Session::SessionService::currentSession()).toQVariant();
    mLocalHostResponderInitialValue = var.toBool();
}

QStringList ClientSettingsProxy::supportedlanguages()
{
    return OriginApplication::instance().LanguageList();
}

QVariantMap ClientSettingsProxy::supportedLanguagesData()
{
    Origin::Downloader::InstallLanguageRequest installLangReq;
    QStringList lang = OriginApplication::instance().LanguageList();

    installLangReq.additionalDownloadRequiredLanguages = QStringList();
    installLangReq.availableLanguages = lang;
    QJsonObject jobj = InstallerView::formatLanguageSelection(installLangReq);
    return jobj.toVariantMap();
}

bool ClientSettingsProxy::areServerUserSettingsLoaded()
{
    return Services::SettingsManager::areServerUserSettingsLoaded();
}

QVariant ClientSettingsProxy::getTimestamp()
{
    qulonglong timestamp = Origin::Services::TrustedClock::instance()->nowUTC().toMSecsSinceEpoch() / 1000;
    return timestamp;
}

void ClientSettingsProxy::writeSetting(const QString& settingName, const QVariant& value)
{
    using namespace Services;
    const Setting* setting = NULL;
    SettingsManager::instance()->getSettingByName(settingName, &setting);
    if (setting)
    {
        if(setting->scope() == Setting::GlobalPerUser && SettingsManager::areServerUserSettingsLoaded() == false)
        {
            ORIGIN_LOG_ACTION << "Cannot write global per user - server setting: " << settingName;
            Client::MainFlow::instance()->contentFlowController()->localContentViewController()->showServerSettingsUnsavable();
        }
        else
        {
            Services::writeSetting(*setting, Variant(value), Session::SessionService::currentSession());
        }
    }
    else
    {
        ORIGIN_LOG_ACTION << "Cannot write global per user - server setting: " << settingName;
    }
}

void ClientSettingsProxy::appendStringSetting(const QString& settingName, const QVariant& value)
{
    const QString valueStr = value.toString();
    if(valueStr.count() && settingName.count())
    {
        const QString seperator = "#//";
        // Get the list of our preferred titles.
        QString settingStringList = Services::readSetting(settingName, Services::Session::SessionService::currentSession()).toString();
        if(settingStringList.count())
        {
            if(settingStringList.contains(valueStr) == false)
            {
                // The server has a max amount it will accept. Therefore, we are inserting at the beginning of the list.
                settingStringList = (valueStr + seperator + settingStringList);
            }
            else
            {
                ORIGIN_LOG_ACTION << valueStr << " already in setting " << settingName;
                return;
            }
        }
        else
        {
            settingStringList = valueStr;
        }
        writeSetting(settingName, settingStringList);
    }
}

QVariant ClientSettingsProxy::readSetting(const QString& settingName) const
{
    return Services::readSetting(settingName, Services::Session::SessionService::currentSession()).toQVariant();
}

bool ClientSettingsProxy::readBoolSetting(const QString& settingName) const
{
    QVariant var = Services::readSetting(settingName, Services::Session::SessionService::currentSession()).toQVariant();
    if (var.type() == QVariant::Bool)
    {
        return var.toBool();
    }
    return false;
}

QVariant ClientSettingsProxy::settingIsDefault(const QString& settingName) const
{
    const Services::Setting* setting = NULL;
    Services::SettingsManager::instance()->getSettingByName(settingName, &setting);
    return Services::isDefault(*setting, Services::Session::SessionService::currentSession());
}

void ClientSettingsProxy::restoreDefaultHotkey(const QString& hotkeySettingName, const QString& hotkeyStrSettingName)
{
    const Services::Setting* setting = NULL;
    Services::SettingsManager::instance()->getSettingByName(hotkeySettingName, &setting);
    if (Services::isDefault(*setting, Services::Session::SessionService::currentSession()) == false)
    {
        Services::reset(*setting, Services::Session::SessionService::currentSession());
        Services::InGameHotKey settingHotKeys = Services::InGameHotKey::fromULongLong(Services::readSetting(*setting, Services::Session::SessionService::currentSession()));
        Services::SettingsManager::instance()->getSettingByName(hotkeyStrSettingName, &setting);
        Services::writeSetting(*setting, settingHotKeys.asNativeString(), Services::Session::SessionService::currentSession());
    }
}

void ClientSettingsProxy::stopLocalHost()
{
    ORIGIN_LOG_EVENT << "ClientSettingsProxy::stopLocalHost";
    Origin::Sdk::LocalHost::stop();
}

void ClientSettingsProxy::startLocalHost()
{
    ORIGIN_LOG_EVENT << "ClientSettingsProxy::startLocalHost";

    Origin::Sdk::LocalHost::start(new Origin::Sdk::LocalHost::LocalHostOriginConfig());
}

bool ClientSettingsProxy::localHostInitialSetting()
{
    return mLocalHostInitialValue;
}

void ClientSettingsProxy::stopLocalHostResponder()
{
    ORIGIN_LOG_EVENT << "ClientSettingsProxy::stopLocalHostResponder";
    // Flush settings to disk if we're being enabled via the UI, otherwise the service won't see it, since it is a separate process
    Services::SettingsManager::instance()->sync();
    Origin::Sdk::LocalHost::stopResponderService();
}

void ClientSettingsProxy::startLocalHostResponder()
{
    ORIGIN_LOG_EVENT << "ClientSettingsProxy::startLocalHostResponder";
    // Flush settings to disk if we're being enabled via the UI, otherwise the service won't see it, since it is a separate process
    Services::SettingsManager::instance()->sync();
    Origin::Sdk::LocalHost::startResponderService();
}

void ClientSettingsProxy::startLocalHostResponderFromOptOut()
{
    ORIGIN_LOG_EVENT << "ClientSettingsProxy::startLocalHostResponderFromOptOut";

    DialogController::instance()->showMessageDialog(DialogController::SPA, DialogController::MessageTemplate("", tr("ebisu_client_localhost_service_dialog_caps"), tr("ebisu_client_localhost_service_dialog_text"), tr("ebisu_client_allow"), tr("ebisu_client_cancel"), "yes", QJsonObject(), this, "onLocalHostResponderAsk"));
}

void ClientSettingsProxy::onLocalHostResponderAsk(QJsonObject obj)
{
    ORIGIN_LOG_EVENT << "ClientSettingsProxy::onLocalHostResponderAsk " << obj["accepted"].toBool();

    if (obj["accepted"].toBool())   // true means the user Allowed
    {   // so turn on both localhost and localhost service
        Services::writeSetting(Services::SETTING_LOCALHOSTENABLED, true);
        Services::writeSetting(Services::SETTING_LOCALHOSTRESPONDERENABLED, true);

        startLocalHost();
        startLocalHostResponder();
    }
    else
    {   // if not allowed, make sure localhost service setting is off
        Services::writeSetting(Services::SETTING_LOCALHOSTRESPONDERENABLED, false);
    }

    emit returnFromSettingsDialog();
}


bool ClientSettingsProxy::localHostResponderInitialSetting()
{
    return mLocalHostResponderInitialValue;
}

void ClientSettingsProxy::onUpdateSettings(const QString& name, const Origin::Services::Variant& var)
{
    emit updateSettings(name, var.toString());
}

void ClientSettingsProxy::hotkeyInputHasFocus(const bool& hasFocus)
{
    ClientViewController* clientViewController = ClientFlow::instance()->view();
    if (clientViewController)
    {
        clientViewController->hotkeyInputHasFocus(hasFocus);
    }
}

void ClientSettingsProxy::pinHotkeyInputHasFocus(const bool& hasFocus)
{
    ClientViewController* clientViewController = ClientFlow::instance()->view();
    if (clientViewController)
    {
        clientViewController->pinHotkeyInputHasFocus(hasFocus);
    }
}
void ClientSettingsProxy::pushToTalkHotKeyInputHasFocus(const bool& hasFocus)
{
    ClientViewController* clientViewController = ClientFlow::instance()->view();
    if (clientViewController)
    {
        clientViewController->pushToTalkHotKeyInputHasFocus(hasFocus);
    }
}

void ClientSettingsProxy::onSettingsError(const QString& code, const QString& info)
{
    ORIGIN_LOG_EVENT << "onSettingsError: " << code << " " << info;
    emit settingsError(code, info);

    if (code == "hotkey_taken")
        mHotkeySwapName = info; // set name of conflicting setting so we can retrieve the value
}

QVariantMap ClientSettingsProxy::hotkeyConflictSwap()
{
    QJsonObject jobj;

    const Services::Setting* setting = NULL;
    Services::SettingsManager::instance()->getSettingByName(mHotkeySwapName, &setting);

    jobj["setting"] = mHotkeySwapName;
    if (setting)
        jobj["keystr"] = Services::readSetting(*setting, Services::Session::SessionService::currentSession()).toString();
    else
        jobj["keystr"] = "";

    return jobj.toVariantMap();
}

}
}
}