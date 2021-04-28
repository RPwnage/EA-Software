#ifndef _CLIENTSETTINGSPROXY_H
#define _CLIENTSETTINGSPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QStringList>
#include <QVariant>

#include "services/plugin/PluginAPI.h"
#include "MainFlow.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API ClientSettingsProxy : public QObject
{
    Q_OBJECT
public:
    ClientSettingsProxy(QObject *parent = NULL);

    Q_PROPERTY(QStringList supportedlanguages READ supportedlanguages);
    /// \brief List of locale codes that the Origin Client currently supports
    QStringList supportedlanguages();

    Q_PROPERTY(QVariantMap supportedLanguagesData READ supportedLanguagesData);
    QVariantMap supportedLanguagesData();

    /// \brief Returns true if the user server-side settings have finished loading.
    Q_PROPERTY(bool areServerUserSettingsLoaded READ areServerUserSettingsLoaded)
    Q_INVOKABLE bool areServerUserSettingsLoaded();

    /// \brief Returns time stamp
    Q_INVOKABLE QVariant getTimestamp();

    /// \brief Restores the default Origin In-Game hotkey. Resets the Hotkey value and string.
    Q_INVOKABLE void restoreDefaultHotkey(const QString& hotkeySettingName, const QString& hotkeyStrSettingName);

    /// \brief Overwrites a setting contained in the SettingsManager.
    Q_INVOKABLE void writeSetting(const QString& settingName, const QVariant& value);

    /// \brief Appends a settings contained in the SettingsManager.
    Q_INVOKABLE void appendStringSetting(const QString& settingName, const QVariant& value);

    /// \brief Returns current value of named setting.
    Q_INVOKABLE QVariant readSetting(const QString& settingName) const;

    /// \brief Returns true/false value of named boolean setting.
    Q_INVOKABLE bool readBoolSetting(const QString& settingName) const;

    /// \brief Returns if the setting value is the default.
    Q_INVOKABLE QVariant settingIsDefault(const QString& settingName) const;

    /// \brief stops the local host.
    Q_INVOKABLE void stopLocalHost();

    /// \brief starts the local host.
    Q_INVOKABLE void startLocalHost();

    /// \brief return initial setting for local host.
    Q_INVOKABLE bool localHostInitialSetting();

    /// \brief stops the local host.
    Q_INVOKABLE void stopLocalHostResponder();

    /// \brief starts the local host.
    Q_INVOKABLE void startLocalHostResponder();

    /// \brief starts flow to ask user who previously opted-out of localhost to start localhost service
    Q_INVOKABLE void startLocalHostResponderFromOptOut();

    /// \brief return initial setting for local host.
    Q_INVOKABLE bool localHostResponderInitialSetting();

    /// \brief Sets whether we should listen for the hotkey input. Called from the 
    /// Javascript when the hotkey line edit focuses-in and out.
    Q_INVOKABLE void hotkeyInputHasFocus(const bool& hasFocus);
    Q_INVOKABLE void pinHotkeyInputHasFocus(const bool& hasFocus);
    Q_INVOKABLE void pushToTalkHotKeyInputHasFocus(const bool& hasFocus);

    /// \brief Returns map of swapped values
    Q_PROPERTY(QVariantMap hotkeyConflictSwap READ hotkeyConflictSwap);
    QVariantMap hotkeyConflictSwap();

signals:
    void updateSettings(const QString&, const QString&);
    void returnFromSettingsDialog();
    void settingsError(const QString&, const QString&);

    private slots:
        void onUpdateSettings(const QString&, const Origin::Services::Variant&);
    void onSettingsError(const QString&, const QString&);

    /// \brief Handle non-modal response to the dialog prompt for opting-in for localhost service when previous opted-out of localhost
    void onLocalHostResponderAsk(QJsonObject);

private: 
    /// \brief Initial value at login for LocalHostEnabled
    bool mLocalHostInitialValue;
    bool mLocalHostResponderInitialValue;

    QString mHotkeySwapName;
};

}
}
}

#endif
