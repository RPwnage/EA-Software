/////////////////////////////////////////////////////////////////////////////
// ApplicationSettingsViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "ApplicationSettingsViewController.h"
#include "SettingsChangeEventHandler.h"
#include "services/debug/DebugService.h"
#include "NativeBehaviorManager.h"
#include "NavigationController.h"
#include "engine/login/LoginController.h"
#include "WebWidget/WidgetView.h"
#include "WebWidget/WidgetPage.h"
#include "WebWidgetController.h"
#include "originwebview.h"
#include <QtWebKitWidgets/QWebFrame>
#include <QKeyEvent>
#include <QMouseEvent>
#include "services/settings/InGameHotKey.h"
#include "UIScope.h"
#include "services/log/LogService.h"
#include "engine/igo/IGOController.h"
#include "services/voice/VoiceService.h"

namespace Origin
{
namespace Client
{

const QString SettingsGeneralLinkRole("http://widgets.dm.origin.com/linkroles/generalview");
const QString SettingsNotificationsLinkRole("http://widgets.dm.origin.com/linkroles/notificationsview");
const QString SettingsInGameLinkRole("http://widgets.dm.origin.com/linkroles/ingameview");
const QString SettingsAdvancedLinkRole("http://widgets.dm.origin.com/linkroles/advancedview");
const QString SettingsVoiceLinkRole("http://widgets.dm.origin.com/linkroles/voiceview");

ApplicationSettingsViewController::ApplicationSettingsViewController(QWidget* parent, const int& context)
: QObject(parent)
, mContext(context)
, mWebViewContainer(NULL)
, mWebView(new WebWidget::WidgetView())
, mSettingsEventHandler(new SettingsChangeEventHandler(this, context))
, mHotkeyInputHasFocus(false)
, mPinHotkeyInputHasFocus(false)
, mPushToTalkHotKeyHasFocus(false)
{
    mWebViewContainer = new UIToolkit::OriginWebView(parent);
    // Give the web view a gray background by default
    mWebViewContainer->setPalette(QPalette(QColor(238,238,238)));
    mWebViewContainer->setAutoFillBackground(true);
    mWebViewContainer->setIsContentSize(false);
    mWebViewContainer->setShowSpinnerAfterInitLoad(false);
    mWebViewContainer->setHasSpinner(true);
    mWebViewContainer->setWebview(mWebView);

    // Act and look more native
    NativeBehaviorManager::applyNativeBehavior(mWebView);

    ORIGIN_VERIFY_CONNECT(mWebView->page()->mainFrame(), SIGNAL(javaScriptWindowObjectCleared()), this, SLOT(populatePageJavaScriptWindowObject()));
    
    if (mContext != IGOScope) // don't record navigation for IGO, this crashes(because IGO removes the object once settings is closed!) and is not necessary(because we have no forward/backward IGO buttons) !!!
    {    
        NavigationController::instance()->addWebHistory(SETTINGS_APPLICATION_TAB, mWebView->page()->history());
        ORIGIN_VERIFY_CONNECT(mWebView, SIGNAL(urlChanged(const QUrl&)), this, SLOT(onUrlChanged(const QUrl&)));
    }

    mWebView->installEventFilter(this);
    
    if (context == IGOScope)
    {
        if (Origin::Engine::IGOController::instance()->showWebInspectors())
            Origin::Engine::IGOController::instance()->showWebInspector(mWebView->page());
    }
}


ApplicationSettingsViewController::~ApplicationSettingsViewController()
{
}


void ApplicationSettingsViewController::onUrlChanged(const QUrl& url)
{
    NavigationController::instance()->recordNavigation(SETTINGS_APPLICATION_TAB, url.toString());
}


QWidget* ApplicationSettingsViewController::view() const
{
    return mWebViewContainer;
}


void ApplicationSettingsViewController::initialLoad(SettingsTab tab)
{
    switch(tab)
    {
    case DefaultTab:
        {
            if(mWebView->url() == "")
            {
                WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "settings", Engine::LoginController::instance()->currentUser());
            }
            break;
        }

    case GeneralTab:
        {
            showGeneralView();
            break;
        }

    case NotificationsTab:
        {
            showNotificationsView();
            break;
        }

    case OIGTab:
        {
            showInGameView();
            break;
        }

    case AdvancedTab:
        {
            showAdvancedView();
            break;
        }

    case VoiceTab:
        {
            showVoiceView();
            break;
        }
    }
}

void ApplicationSettingsViewController::showGeneralView()
{
    if(mWebView->url() == "")
        WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "settings", Engine::LoginController::instance()->currentUser(), SettingsGeneralLinkRole);
    else
        mWebView->loadLink(SettingsGeneralLinkRole);
}

void ApplicationSettingsViewController::showNotificationsView()
{
    if(mWebView->url() == "")
        WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "settings", Engine::LoginController::instance()->currentUser(), SettingsNotificationsLinkRole);
    else
        mWebView->loadLink(SettingsNotificationsLinkRole);
}

void ApplicationSettingsViewController::showInGameView()
{
    if(mWebView->url() == "")
        WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "settings", Engine::LoginController::instance()->currentUser(), SettingsInGameLinkRole);
    else
        mWebView->loadLink(SettingsInGameLinkRole);
}

void ApplicationSettingsViewController::showAdvancedView()
{
    if(mWebView->url() == "")
        WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "settings", Engine::LoginController::instance()->currentUser(), SettingsAdvancedLinkRole);
    else
        mWebView->loadLink(SettingsAdvancedLinkRole);
}

void ApplicationSettingsViewController::showVoiceView()
{
    if(mWebView->url() == "")
        WebWidgetController::instance()->loadUserSpecificWidget(mWebView->widgetPage(), "settings", Engine::LoginController::instance()->currentUser(), SettingsVoiceLinkRole);
    else
        mWebView->loadLink(SettingsVoiceLinkRole);
}

bool ApplicationSettingsViewController::eventFilter(QObject* obj, QEvent* event)
{
    bool stopEvent = false;
    switch(event->type())
    {
    case QEvent::MouseButtonPress:
        {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(event);
            if (mouseEvent && mPushToTalkHotKeyHasFocus)
            {
                mPushToTalkHotKeyHasFocus = false;
                handlePushToTalkMouseInput(mouseEvent);
            }
        }
        break;
    case QEvent::KeyPress:
        {
            QKeyEvent* keyEvent = dynamic_cast<QKeyEvent*>(event);
            if(keyEvent && (mHotkeyInputHasFocus || mPinHotkeyInputHasFocus))
            {
                //we don't want the escape key to run through the OIG logic as that forces code to revert the hotkey to the default
                //fixes a bug where hitting ESC after setting a hotkey puts the hotkey back at Shift-f1
                if(keyEvent->key() != Qt::Key_Escape)
                {
                    if(mHotkeyInputHasFocus)
                        handleHotkeyInput(Services::SETTING_INGAME_HOTKEY, Services::SETTING_INGAME_HOTKEYSTRING, keyEvent);
                    else if(mPinHotkeyInputHasFocus)
                        handleHotkeyInput(Services::SETTING_PIN_HOTKEY, Services::SETTING_PIN_HOTKEYSTRING, keyEvent);
                }
                // IF the user pressed Backspace eat the event. If we don't - the
                // web page will think we are trying to navigate to the previous URL.
                if(keyEvent->key() == Qt::Key_Backspace)
                    stopEvent = true;
            }
            if(keyEvent && mPushToTalkHotKeyHasFocus)
            {
                //we don't want the escape key to run through the OIG logic as that forces code to revert the hotkey to the default
                //fixes a bug where hitting ESC after setting a hotkey puts the hotkey back at Ctrl
                if(keyEvent->key() != Qt::Key_Escape)
                {
                    handlePushToTalkHotkeyInput(keyEvent);
                }
                // IF the user pressed Backspace eat the event. If we don't - the
                // web page will think we are trying to navigate to the previous URL.
                if (keyEvent->key() == Qt::Key_Backspace)
                    stopEvent = true;
            }
        }
        break;
    case QEvent::KeyRelease:
        {
            mPushToTalkHotKeyHasFocus = false;
        }
        break;

    default:
        break;
    }

    return stopEvent ? stopEvent : QObject::eventFilter(obj, event);
}


void ApplicationSettingsViewController::populatePageJavaScriptWindowObject()
{
    mWebView->page()->mainFrame()->addToJavaScriptWindowObject("helper", this);
}


void ApplicationSettingsViewController::handleHotkeyInput(const Services::Setting& hotkey, const Services::Setting& hotkeyStr, QKeyEvent* keyEvent)
{
    if(keyEvent)
    {
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        
#ifdef ORIGIN_MAC
        // Swap the Qt::ControlModifier and the Qt::MetaModifier so that the control key would be the same if we were to share the hot key settings between platform one day
        Qt::KeyboardModifier ctrlModifier = (modifiers & Qt::MetaModifier) ? Qt::ControlModifier : Qt::NoModifier;
        Qt::KeyboardModifier metaModifier = (modifiers & Qt::ControlModifier) ? Qt::MetaModifier: Qt::NoModifier;
        modifiers = modifiers & ~(Qt::ControlModifier | Qt::MetaModifier);
        modifiers |= ctrlModifier | metaModifier;
#endif
        
        Services::InGameHotKey settingHotKeys(modifiers, keyEvent->key(), keyEvent->nativeVirtualKey());
                    ORIGIN_LOG_DEBUG << settingHotKeys.toULongLong() << " " << settingHotKeys.asNativeString();
        switch(settingHotKeys.status())
        {

        case Services::InGameHotKey::Valid:
            Services::writeSetting(hotkey, settingHotKeys.toULongLong(), Services::Session::SessionService::currentSession());
            Services::writeSetting(hotkeyStr, settingHotKeys.asNativeString(), Services::Session::SessionService::currentSession());
            break;

        case Services::InGameHotKey::NotValid_Revert: // If the user put in something bad: e.g. Esc
            if(Services::isDefault(hotkey) == false)
            {
                Services::reset(hotkey);
                settingHotKeys = Services::InGameHotKey::fromULongLong(Services::readSetting(hotkey, Services::Session::SessionService::currentSession()));
                Services::writeSetting(hotkeyStr, settingHotKeys.asNativeString(), Services::Session::SessionService::currentSession());
            }
            break;

        case Services::InGameHotKey::NotValid_Ignore:
        default:
            break;
        }
    }
}

void ApplicationSettingsViewController::handlePushToTalkHotkeyInput(QKeyEvent* keyEvent)
{
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    if(keyEvent)
    {
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

        Origin::Services::InGameHotKey settingHotKeys(modifiers, keyEvent->key(), keyEvent->nativeVirtualKey());
        Origin::Services::InGameHotKey::HotkeyStatus status = settingHotKeys.status();

        // Bad key blacklist
        if ( (keyEvent->key() == Qt::Key_NumLock) ||        // Num Lock
             (modifiers & Qt::KeypadModifier)               // Any keypad key
           )
        {
            status = Origin::Services::InGameHotKey::NotValid_Ignore;
        }

        switch(status)
        {
        case Origin::Services::InGameHotKey::Valid:
            Services::writeSetting(Services::SETTING_PushToTalkKey, settingHotKeys.toULongLong(), Origin::Services::Session::SessionService::currentSession());
            Services::writeSetting(Services::SETTING_PushToTalkKeyString, settingHotKeys.asNativeString(), Origin::Services::Session::SessionService::currentSession());
            break;

        case Origin::Services::InGameHotKey::NotValid_Revert: // If the user put in something bad: e.g. Esc
            if(Services::isDefault(Services::SETTING_PushToTalkKey) == false)
            {
                Services::reset(Services::SETTING_PushToTalkKey);
                settingHotKeys = Origin::Services::InGameHotKey::fromULongLong(Origin::Services::readSetting(Services::SETTING_PushToTalkKey, Origin::Services::Session::SessionService::currentSession()));
                Services::writeSetting(Services::SETTING_PushToTalkKeyString, settingHotKeys.asNativeString(), Origin::Services::Session::SessionService::currentSession());
            }
            break;

        case Origin::Services::InGameHotKey::NotValid_Ignore:
        default:
            break;
        }
    }
#endif
}

void ApplicationSettingsViewController::handlePushToTalkMouseInput(QMouseEvent* mouseEvent)
{
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    if(mouseEvent)
    {
        Qt::MouseButton button = mouseEvent->button();
        if (button)
        {
            int num = 0;
            switch (button)
            {
                case Qt::LeftButton:
                    num = 1;
                    break;
                case Qt::MidButton:
                    num = 2;
                    break;
                case Qt::RightButton:
                    num = 3;
                    break;
                case Qt::XButton1:
                    num = 4;
                    break;
                case Qt::XButton2:
                    num = 5;
                    break;
            }
            if (num)
            {
                QString str = QString("mouse%1").arg(QString::number(num));
                Services::writeSetting(Services::SETTING_PushToTalkKey, 0, Origin::Services::Session::SessionService::currentSession());
                Services::writeSetting(Services::SETTING_PushToTalkMouse, str, Origin::Services::Session::SessionService::currentSession());
                Services::writeSetting(Services::SETTING_PushToTalkKeyString, str, Origin::Services::Session::SessionService::currentSession());
            }
        }
    }
#endif
}

}
}
