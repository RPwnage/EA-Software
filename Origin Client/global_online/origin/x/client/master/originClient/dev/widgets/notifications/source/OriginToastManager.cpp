#include "OriginToastManager.h"
#include "ToastViewController.h"
#include "engine/igo/IGOController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "services/platform/PlatformService.h"
#include "services/voice/VoiceService.h"
#include "TelemetryAPIDLL.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QWidget>
#include "ToastView.h"
#include "OriginNotificationDialog.h"
#include "originlabel.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "chat/OriginConnection.h"
#include "engine/login/User.h"
#include "engine/social/SocialController.h"
#include "services/network/NetworkAccessManagerFactory.h"
#include "services/network/NetworkAccessManager.h"

//Custom Content
#include "AchievementToastView.h"
#include "HotkeyToastView.h"
#include "ActionableToastView.h"


namespace Origin
{
namespace Client
{
namespace 
{
    const QString OIG_HOTKEY_IDENTIFIER = "POPUP_OIG_HOTKEY";
    const int DEFAULT_CLOSE_TIMER = 3000; // 3 second
    const int INCOMMING_CALL_CLOSE_TIMER = -1; // Don't want these to close on their own
    const float IN_GAME_MAX_OPACITY = 0.9f; // In-Game notificatoins should have 90% opacity max
}

OriginToastManager::ToastSetting::ToastSetting() : mIconPath(""), mSoundPath(""), mAlertMethod(TOAST_DISABLED) {}
OriginToastManager::ToastSetting::ToastSetting(const QString& name, const QString& iconPath, const QString& soundPath)
: mIconPath(iconPath)
, mSoundPath(soundPath)
, mAlertMethod(static_cast<AlertMethod>((int)Services::readSetting(name, Services::Session::SessionService::currentSession())))
{
}
OriginToastManager::ToastSetting::ToastSetting(const QString& iconPath, const QString& soundPath, const AlertMethod& alert)
: mIconPath(iconPath)
, mSoundPath(soundPath)
, mAlertMethod(alert)
{
}

OriginToastManager::ToastBehavior::ToastBehavior()
: mShowBubble(false)
, mIGOFlags(false)
, mAcceptClick(false)
, mDelayShow(false)
, mCloseTime(DEFAULT_CLOSE_TIMER)
, mType(Origin::Client::ToastViewController::ToastType_Default)
, mOpacity(0.0f)
{
}

OriginToastManager::OriginToastManager()
{
    // Set up ToastViewController
    mController = QSharedPointer<ToastViewController>(new ToastViewController());
#if ENABLE_VOICE_CHAT
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( isVoiceEnabled )
        ORIGIN_VERIFY_CONNECT(mController.data(), SIGNAL(newMiniToast(QString)), this, SLOT(showVoiceChatIdentifier(QString)));
#endif

    ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, Origin::Services::Variant const&)),
        this, SLOT(updateData(const QString&, Origin::Services::Variant const&)));
    //
    // Allocate and null-initialize toast window pointer pool
    //
    {
        using namespace Origin::Services;
        const QString resourcesDirectory = PlatformService::resourcesDirectory();

        mToastSettings.insert(SETTING_NOTIFY_DOWNLOADERROR.name(), ToastSetting(SETTING_NOTIFY_DOWNLOADERROR.name(), 
            QString::fromUtf8(":/notifications/error_download.png"), 
            resourcesDirectory + QString::fromUtf8("/sounds/ErrorDownload.wav")));

        mToastSettings.insert(SETTING_NOTIFY_FINISHEDDOWNLOAD.name(),
            ToastSetting(SETTING_NOTIFY_FINISHEDDOWNLOAD.name(), 
            QString::fromUtf8(":/notifications/finished_download.png"), 
            resourcesDirectory + QString::fromUtf8("/sounds/FinishedDownload.wav")));

        mToastSettings.insert(SETTING_NOTIFY_FINISHEDINSTALL.name(),
            ToastSetting(SETTING_NOTIFY_FINISHEDINSTALL.name(), 
            QString::fromUtf8(":/notifications/finished_install.png"), 
            resourcesDirectory + QString::fromUtf8("/sounds/FinishedInstall.wav")));

        mToastSettings.insert(SETTING_NOTIFY_INCOMINGTEXTCHAT.name(),
            ToastSetting(SETTING_NOTIFY_INCOMINGTEXTCHAT.name(), 
            QString::fromUtf8(":/notifications/message_icon.png"), resourcesDirectory + QString("/sounds/IncomingTextChat.wav")));

        mToastSettings.insert(SETTING_NOTIFY_FRIENDSIGNINGIN.name(),
            ToastSetting(SETTING_NOTIFY_FRIENDSIGNINGIN.name(), 
            QString::fromUtf8(":/notifications/online_icon.png"), resourcesDirectory + QString("/sounds/FriendSigningIn.wav")));

        mToastSettings.insert(SETTING_NOTIFY_FRIENDSIGNINGOUT.name(),
            ToastSetting(SETTING_NOTIFY_FRIENDSIGNINGOUT.name(), 
            QString::fromUtf8(":/notifications/offline_icon.png"), resourcesDirectory + QString("/sounds/FriendSigningOut.wav")));

        mToastSettings.insert(SETTING_NOTIFY_FRIENDSTARTSGAME.name(),
            ToastSetting(SETTING_NOTIFY_FRIENDSTARTSGAME.name(), 
            QString::fromUtf8(":/notifications/startedplaying_icon.png"), resourcesDirectory + QString("/sounds/FriendStartsGame.wav")));

        mToastSettings.insert(SETTING_NOTIFY_FRIENDQUITSGAME.name(),
            ToastSetting(SETTING_NOTIFY_FRIENDQUITSGAME.name(), 
            QString::fromUtf8(":/notifications/stoppedplaying_icon.png"), resourcesDirectory + QString("/sounds/FriendQuitsGame.wav")));

        mToastSettings.insert(SETTING_NOTIFY_NONFRIENDINVITE.name(),
            ToastSetting(SETTING_NOTIFY_NONFRIENDINVITE.name(), 
            QString::fromUtf8(":/notifications/message_icon.png"), resourcesDirectory + QString("/sounds/IncomingTextChat.wav")));

        mToastSettings.insert(SETTING_NOTIFY_GAMEINVITE.name(),
            ToastSetting(SETTING_NOTIFY_GAMEINVITE.name(), 
            QString::fromUtf8(":/notifications/game_invite.png"), resourcesDirectory + QString("/sounds/IncomingGameInvite.wav")));

        mToastSettings.insert(SETTING_NOTIFY_INCOMINGCHATROOMMESSAGE.name(),
            ToastSetting(SETTING_NOTIFY_INCOMINGCHATROOMMESSAGE.name(), 
            QString::fromUtf8(":/notifications/chatinvite.png"), resourcesDirectory + QString("/sounds/IncomingTextChat.wav")));

        mToastSettings.insert(SETTING_NOTIFY_INVITETOCHATROOM.name(),
            ToastSetting(SETTING_NOTIFY_INVITETOCHATROOM.name(), 
            QString::fromUtf8(":/notifications/chatinvite.png"), resourcesDirectory + QString("/sounds/IncomingTextChat.wav")));
       
        mToastSettings.insert(OIG_HOTKEY_IDENTIFIER,
            ToastSetting(QString::fromUtf8(":/notifications/defaultorigin.png"), resourcesDirectory + QString("/sounds/FriendStartsGame.wav"), ToastSetting::TOAST_ALERT_ONLY));

        mToastSettings.insert("POPUP_FREE_TRIAL_EXPIRING",
            ToastSetting(QString::fromUtf8(":/notifications/game_time_expiring.png"), resourcesDirectory + QString("/sounds/GameTime.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert("POPUP_BROADCAST_STOPPED",
            ToastSetting(QString::fromUtf8(":/notifications/stoppedbroadcasting_icon.png"), resourcesDirectory + QString("/sounds/StoppedBroadcasting.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert(SETTING_NOTIFY_FRIENDSTOPBROADCAST.name(),
            ToastSetting(SETTING_NOTIFY_FRIENDSTOPBROADCAST.name(), 
            QString::fromUtf8(":/notifications/stoppedbroadcasting_icon.png"), resourcesDirectory + QString("/sounds/StoppedBroadcasting.wav")));

        mToastSettings.insert(SETTING_NOTIFY_FRIENDSTARTBROADCAST.name(),
            ToastSetting(SETTING_NOTIFY_FRIENDSTARTBROADCAST.name(), 
            QString::fromUtf8(":/notifications/startedbroadcasting_icon.png"), resourcesDirectory + QString("/sounds/FriendStartsBroadcasting.wav")));

        mToastSettings.insert(SETTING_NOTIFY_GROUPCHATINVITE.name(),
            ToastSetting(SETTING_NOTIFY_GROUPCHATINVITE.name(), 
            QString::fromUtf8(":/notifications/chatinvite.png"), resourcesDirectory + QString("/sounds/IncomingGroupChatInvite.wav")));

        mToastSettings.insert("POPUP_BROADCAST_NOT_PERMITTED",
            ToastSetting(QString::fromUtf8(":/notifications/startedbroadcasting_icon.png"), resourcesDirectory + QString("/sounds/StoppedBroadcasting.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert(SETTING_NOTIFY_ACHIEVEMENTUNLOCKED.name(),
            ToastSetting(SETTING_NOTIFY_ACHIEVEMENTUNLOCKED.name(), 
            QString::fromUtf8(":/notifications/achievementicon.png"), resourcesDirectory + QString("/sounds/Achievement.wav")));

        mToastSettings.insert(SETTING_NOTIFY_INCOMINGVOIPCALL.name(),
            ToastSetting(SETTING_NOTIFY_INCOMINGVOIPCALL.name(), 
            QString::fromUtf8(":/notifications/chatinvite.png"), resourcesDirectory + QString("/sounds/IncomingTextChat.wav")));

#if ENABLE_VOICE_CHAT
        bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
        if( isVoiceEnabled )
        {
            mToastSettings.insert(SETTING_EnableVoiceChatIndicator.name(),
                ToastSetting(SETTING_EnableVoiceChatIndicator.name(),
                QString::fromUtf8(":/notifications/voip-talking.png"), resourcesDirectory + QString("/sounds/Achievement.wav")));
        }
#endif

        mToastSettings.insert("POPUP_OIG_KOREAN_WARNING",
            ToastSetting(QString::fromUtf8(":/notifications/korean_timer.png"), resourcesDirectory + QString("/sounds/KoreanTimer.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert("POPUP_IGO_PIN_OFF",
            ToastSetting(QString::fromUtf8(":/notifications/pin_toast_off.png"), resourcesDirectory + QString("/sounds/Achievement.wav"), ToastSetting::TOAST_ALERT_ONLY));

        mToastSettings.insert("POPUP_IGO_PIN_ON",
            ToastSetting(QString::fromUtf8(":/notifications/pin_toast_on.png"), resourcesDirectory + QString("/sounds/Achievement.wav"), ToastSetting::TOAST_ALERT_ONLY));

        mToastSettings.insert("POPUP_ROOM_DESTROYED", 
            ToastSetting(QString::fromUtf8(":/notifications/room_destroyed.png"), resourcesDirectory + QString("/sounds/CallEnded.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert("POPUP_GROUP_DESTROYED", 
            ToastSetting(QString::fromUtf8(":/notifications/room_destroyed.png"), resourcesDirectory + QString("/sounds/CallEnded.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert("POPUP_USER_KICKED_FROM_ROOM", 
            ToastSetting(QString::fromUtf8(":/notifications/user_kicked.png"), resourcesDirectory + QString("/sounds/CallEnded.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert("POPUP_USER_KICKED_FROM_GROUP", 
            ToastSetting(QString::fromUtf8(":/notifications/user_kicked.png"), resourcesDirectory + QString("/sounds/CallEnded.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert("POPUP_GLOBAL_UNMUTE", 
            ToastSetting(QString::fromUtf8(":/notifications/global_unmute.png"), resourcesDirectory + QString("/sounds/IncomingTextChat.wav"), ToastSetting::TOAST_SOUND_ALERT));

        mToastSettings.insert("POPUP_GLOBAL_MUTE", 
            ToastSetting(QString::fromUtf8(":/notifications/global_mute.png"), resourcesDirectory + QString("/sounds/CallEnded.wav"), ToastSetting::TOAST_SOUND_ALERT));

    }
}


OriginToastManager::~OriginToastManager()
{
}

OriginToastManager::ToastSetting::AlertMethod OriginToastManager::ToastSetting::alertMethod()
{
    if (Services::readSetting(Services::SETTING_DisableNotifications))
    {
        return TOAST_DISABLED;
    }
    return mAlertMethod;
}


UIToolkit::OriginNotificationDialog* OriginToastManager::showToast(const QString& name, const QString& title)
{
    return showToast(name, title, "");
}

UIToolkit::OriginNotificationDialog* OriginToastManager::showToast(const QString& name, const QString& title, const QString& message)
{
    
    return showToast(name, title, message, QPixmap());
}

/*
We had an issue https://developer.origin.com/support/browse/EBIBUGS-26720 that was deemed unreproducible under "normal" circumstances.
Work was done to switch to a notifications to a queued system, but this was deemed too risky/complicated for such a small edge case that 
only developers were seeing.  If we determine that we do need this queued system in the future the changes are shelved in CL 262572.
*/

UIToolkit::OriginNotificationDialog* OriginToastManager::showToast(const QString& name, const QString& title, const QString& message, const QPixmap& pixmap)
{
    ToastSetting toastSetting = mToastSettings[name];
    UIScope scope = ((Origin::Engine::IGOController::instance()->isGameInForeground() &&
                      Origin::Engine::IGOController::instance()->igowm()->isScreenLargeEnough()) || 
                      mController->fakeOIGContext()) ? IGOScope : ClientScope;    QVector<QWidget*> customWidgets = setUpCustomWidgets(scope, name, toastSetting);
    
    ToastBehavior behavior = ToastBehavior::ToastBehavior();
    // in most cases we want both of these true;
    behavior.setShowBubble(true);
    behavior.setAcceptClick(true);
    // most notifications will be solid
    behavior.setMaxOpacity(1);
    // OIG HotKey notification has a special styling
    if (name == OIG_HOTKEY_IDENTIFIER)
    {
        behavior.setShowBubble(false);
        behavior.setToastType(ToastViewController::ToastType_IGOhotkey);
	}
	else if(scope == IGOScope && (name != OIG_HOTKEY_IDENTIFIER) && (!Origin::Engine::IGOController::instance()->isActive() || mController->fakeOIGContext()))
	{
		behavior.setShowBubble(false);
		behavior.setIGOFlags(true);
        behavior.setMaxOpacity(IN_GAME_MAX_OPACITY);
	}

    // We do not want to accept clicks on this dialog itself only the buttons
    // Incoming call notification should stay on screen for 30 seconds
    if (name == Services::SETTING_NOTIFY_INCOMINGVOIPCALL.name())
    {
        toastSetting.setAlertMethod(ToastSetting::TOAST_ALERT_ONLY);
        
        behavior.setCloseTime(INCOMMING_CALL_CLOSE_TIMER);
        behavior.setToastType(ToastViewController::ToastType_IncomingCall);
        // don't want desktop or OIG versions clickable
        if (scope != IGOScope || Origin::Engine::IGOController::instance()->isActive())
            behavior.setAcceptClick(false);
    }

    UIToolkit::OriginNotificationDialog* toast = mController->showToast(toastSetting, behavior, name, title, message, pixmap, customWidgets, scope);

    return toast;
}

UIToolkit::OriginNotificationDialog* OriginToastManager::showAchievementToast(const QString& name, const QString& title, const QString& message, int xp, const QString& pixmapStr, bool iconNeedsLoading)
{
    ToastSetting toastSetting = mToastSettings[name];
    UIScope scope = ((Origin::Engine::IGOController::instance()->isGameInForeground() &&
                      Origin::Engine::IGOController::instance()->igowm()->isScreenLargeEnough()) || 
                      mController->fakeOIGContext()) ? IGOScope : ClientScope;
    QVector<QWidget*> customWidgets = setUpCustomWidgets(scope, name, toastSetting);
    customWidgets.append(new AchievementToastView(xp, scope));

    ToastBehavior behavior = ToastBehavior::ToastBehavior();
    behavior.setAcceptClick(true);
    behavior.setDelayShow(true);
    // Set up for IGO and in-game toasts
    if(scope == IGOScope && (!Origin::Engine::IGOController::instance()->isActive() || mController->fakeOIGContext()))
    {
        behavior.setIGOFlags(true);
        behavior.setMaxOpacity(IN_GAME_MAX_OPACITY);
    }
    else
    {
        behavior.setShowBubble(true);
        behavior.setMaxOpacity(1);
    }


    UIToolkit::OriginNotificationDialog* toast = mController->showToast(toastSetting, behavior, name, title, message, pixmapStr, iconNeedsLoading, customWidgets, scope);
    
    return toast;
}

#if ENABLE_VOICE_CHAT
void OriginToastManager::showVoiceChatIdentifier(const QString& userId, int voiceType)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    if (!(Origin::Engine::IGOController::instance()->isGameInForeground() &&
          Origin::Engine::IGOController::instance()->igowm()->isScreenLargeEnough()))
        return;

    ToastSetting toastSetting = mToastSettings[Services::SETTING_EnableVoiceChatIndicator.name()];
    ToastBehavior behavior = ToastBehavior::ToastBehavior();
    behavior.setIGOFlags(true);
    behavior.setCloseTime(-1);
    behavior.setMaxOpacity(IN_GAME_MAX_OPACITY);
    if (voiceType == UserVoiceType_CurrentUserMuted)
    {
        toastSetting.setIconPath(QString::fromUtf8(":/notifications/transmit-mute.png"));
        mController->showMiniToast(toastSetting, behavior, voiceType, userId, toastSetting.iconPath());
    }
    else if (voiceType == UserVoiceType_CurrentUserTalking)
    {
        toastSetting.setIconPath(QString::fromUtf8(":/notifications/transmit-talking.png"));
        mController->showMiniToast(toastSetting, behavior, voiceType, userId, toastSetting.iconPath());
    }
    else
    {
        mController->showMiniToast(toastSetting, behavior, voiceType, userId, toastSetting.iconPath());
    }
}

void OriginToastManager::removeVoiceChatIdentifier(const QString& userId)
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    ToastBehavior behavior = ToastBehavior::ToastBehavior();
    behavior.setIGOFlags(true);
    behavior.setCloseTime(-1);
    mController->removeMiniToast(userId, behavior);
}

void OriginToastManager::removeAllVoiceChatIdentifiers()
{
    bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
    if( !isVoiceEnabled )
        return;

    mController->removeAllMiniToasts();
}
#endif

bool OriginToastManager::processActiveToast(const QString& name)
{
    UIToolkit::OriginNotificationDialog* activeToast = mController->activeIGOToast();
    if (activeToast)
    {
        ToastView* activeView = dynamic_cast<ToastView*>(activeToast->content());
        if (activeView)
        {
            if(activeView->name() == Services::SETTING_NOTIFY_INCOMINGVOIPCALL.name() && name != Services::SETTING_NOTIFY_INCOMINGVOIPCALL.name())
            {
                // need to not show hotkeys on new notification
                return false;
            }
            else
            {
                // Find and remove the HotkeyToastView
                QObjectList children = activeView->children();
                foreach (QObject* child, children)
                {
                    HotkeyToastView* hotkeys = dynamic_cast<HotkeyToastView*>(child);
                    if(hotkeys)
                    {
                        // Qt doesn't like dynamic sizing after removing widgets so we need the 
                        // new height of the view
                        int newHeight = activeView->removeCustomWidget(hotkeys);
                        // Set the height of the notification using the newHeight value
                        // Also need to add "extra" spacing for the margins
                        activeToast->setFixedHeight(newHeight + 10);
                        //Set this to NULL so that next new toast is the active one
                        mController->setActiveIGOToast(NULL);
                    }
                }
            }
        }
    }
    return true;
}

void OriginToastManager::updateData(const QString& settingName, Origin::Services::Variant const& val)
{
    if(mToastSettings.contains(settingName))
    {
        ToastSetting setting = mToastSettings.value(settingName);
        setting.setAlertMethod(static_cast<ToastSetting::AlertMethod>((int)val));
        mToastSettings.insert(settingName, setting);
    }
}


QVector<QWidget*> OriginToastManager::setUpCustomWidgets(Origin::Client::UIScope scope, QString name, OriginToastManager::ToastSetting settings)
{
    using namespace Origin::Services;
    QVector<QWidget*> customWidgets;
    if(name == SETTING_NOTIFY_FRIENDSIGNINGOUT.name() || 
       name == SETTING_NOTIFY_FRIENDSTOPBROADCAST.name()||
       name == SETTING_NOTIFY_FINISHEDINSTALL.name() ||
       name == "POPUP_OIG_KOREAN_WARNING" ||
       name == "POPUP_GROUP_DESTROYED" ||
       name == "POPUP_USER_KICKED_FROM_GROUP" ||
       settings.alertMethod() < ToastSetting::TOAST_ALERT_ONLY)
    {
       return customWidgets;
    }

    bool shouldMakeHotkeyWidget = processActiveToast(name);

    if (name == SETTING_NOTIFY_INCOMINGVOIPCALL.name() &&
             (scope != IGOScope || (scope == IGOScope && 
             Origin::Engine::IGOController::instance()->isActive())))
    {
        customWidgets.append(new ActionableToastView());
    }

    else if(name == OIG_HOTKEY_IDENTIFIER)
    {
        customWidgets.append(new HotkeyToastView(HotkeyToastView::Open));
    }

    else
    {
        if(scope == IGOScope && !Origin::Engine::IGOController::instance()->isActive() && shouldMakeHotkeyWidget)
        {
            if(name == SETTING_NOTIFY_INCOMINGTEXTCHAT.name())
                customWidgets.append(new HotkeyToastView(HotkeyToastView::Reply));
            else if(name == SETTING_NOTIFY_FRIENDSIGNINGIN.name())
                customWidgets.append(new HotkeyToastView(HotkeyToastView::Chat));
            else if(name == "POPUP_IGO_PIN_OFF")
                customWidgets.append(new HotkeyToastView(HotkeyToastView::Repin));
            else if(name == "POPUP_IGO_PIN_ON")
                customWidgets.append(new HotkeyToastView(HotkeyToastView::Unpin));
            else
                customWidgets.append(new HotkeyToastView(HotkeyToastView::View));
        }
    }
    return customWidgets;
}

void OriginToastManager::enableFakeOIG(const bool& fakeOIG)
{
    mController->setFakeOIGContext(fakeOIG);
}

}
}
