/////////////////////////////////////////////////////////////////////////////
// ApplicationSettingsViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef APPLICATION_SETTINGS_VIEW_CONTROLLER_H
#define APPLICATION_SETTINGS_VIEW_CONTROLLER_H

#include <QObject>
#include <QUrl>
#include "services/settings/Setting.h"
#include "services/plugin/PluginAPI.h"

class QKeyEvent;
class QMouseEvent;

namespace WebWidget
{
    class WidgetView;
}

namespace Origin
{
namespace UIToolkit
{
class OriginWebView;
}
namespace Client
{
class SettingsChangeEventHandler;

class ORIGIN_PLUGIN_API ApplicationSettingsViewController : public QObject
{
	Q_OBJECT

public:

    enum SettingsTab
    {
        //default tab must always be first at the 0 position
        DefaultTab,
        GeneralTab,
        NotificationsTab,
        OIGTab,
        AdvancedTab,
        VoiceTab
    };

    /// \brief Constructor
    /// \param parent The parent of the ApplicationSettingsViewController
    /// \param context TBD.
    ApplicationSettingsViewController(QWidget* parent = 0, const int& context = 0);

    /// \brief Destructor
	~ApplicationSettingsViewController();

    /// \brief Returns the widget that should be displayed.
    QWidget* view() const;

    /// \brief Load web page if it hasn't already loaded
    void initialLoad(SettingsTab tab = DefaultTab);

    /// \brief loads the deep link to the general tab 
    void showGeneralView();

    /// \brief loads the deep link to the notifications tab 
    void showNotificationsView();

    /// \brief loads the deep link to the ingame tab 
    void showInGameView();

    /// \brief loads the deep link to the advanced tab 
    void showAdvancedView();

    /// \brief loads the deep link to the voice tab
    void showVoiceView();

    /// \brief Sets whether we should listen for the hotkey input. Called from the 
    /// Javascript when the hotkey line edit focuses-in and out.
    Q_INVOKABLE void hotkeyInputHasFocus(const bool& hasFocus) {mHotkeyInputHasFocus = hasFocus;}
    Q_INVOKABLE void pinHotkeyInputHasFocus(const bool& hasFocus) {mPinHotkeyInputHasFocus = hasFocus;}
    Q_INVOKABLE void pushToTalkHotKeyInputHasFocus(const bool& hasFocus) {mPushToTalkHotKeyHasFocus = hasFocus;}

    /// \brief Called from the Javascript to know what our context is.
    /// If we are in OIG some settings well be hidden or disabled.
    Q_INVOKABLE int context() const {return mContext;}


protected:
    bool eventFilter(QObject* obj, QEvent* event);

protected slots:
    /// \brief Records the navigation when the url changes.
    void onUrlChanged(const QUrl& url);

    /// \brief Let's the JavaScript access this class. Used for telling the the 
    /// web page what context we are in and to start listening for OIG hotkey input.
    void populatePageJavaScriptWindowObject();


private:
    /// \brief Handles the OIG hotkey input.
    void handleHotkeyInput(const Services::Setting& hotkey, const Services::Setting& hotkeyStr, QKeyEvent* event);

    /// \brief Handles the VOIP Push to Talk hotkey event
    void handlePushToTalkHotkeyInput(QKeyEvent* event);

    /// \brief Handles the VOIP Push to Talk mouse event
    void handlePushToTalkMouseInput(QMouseEvent* mouseEvent);

    const int mContext;
    UIToolkit::OriginWebView* mWebViewContainer;
    WebWidget::WidgetView* mWebView;
    SettingsChangeEventHandler* mSettingsEventHandler;
    bool mHotkeyInputHasFocus;
    bool mPinHotkeyInputHasFocus;
    bool mPushToTalkHotKeyHasFocus;
};
}
}
#endif
