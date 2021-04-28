#ifndef TOASTVIEWCONTROLLER_H
#define TOASTVIEWCONTROLLER_H

#include <QObject>
#include <QString>
#include <QVector>

#include "services/settings/Setting.h"
#include "services/settings/SettingsManager.h"
#include "services/plugin/PluginAPI.h"
#include "OriginToastManager.h"
#include "OriginToastLayoutManager.h"
#ifdef ORIGIN_PC
#include "qt_windows.h"
#endif //ORIGIN_PC

//class QNetworkAccessManager;
class QNetworkReply;
class QMediaPlayer;

namespace Origin
{
    namespace Services
    {
        class NetworkAccessManager;
    }
namespace UIToolkit
{
    class OriginNotificationDialog;
}
       
namespace Client
{
    class OriginToastLayoutManager;

class ORIGIN_PLUGIN_API ToastViewController : public QObject
{
	Q_OBJECT

    // This is passed to the Layout Manager to specail case notifications

public:
    // Specail types used for placment by the OriginToastLayoutManager
    enum ToastType
    {
        ToastType_Default = OriginToastLayoutManager::GenericID,
        ToastType_IGOhotkey,
        ToastType_IncomingCall,
        ToastType_VoiceIdentifierCounter,
        ToastType_UserVoiceIdentifier
    };
	
    ToastViewController();
    ~ToastViewController();
    // For toast creation
    // Create mini toast for voice indicator
    void showMiniToast(OriginToastManager::ToastSetting setting, OriginToastManager::ToastBehavior behavior, int voiceType, const QString& title, const QPixmap& pixmap);

    // Remove mini toast, currently these do not fade in or out and are not removed until this is called
    void removeMiniToast(const QString& title, OriginToastManager::ToastBehavior behavior);
    
    //Remove all mini toasts
    void removeAllMiniToasts();

    // Create toast for display
    UIToolkit::OriginNotificationDialog* showToast(OriginToastManager::ToastSetting setting, OriginToastManager::ToastBehavior behavior, const QString& name, const QString& title, const QString& message, const QPixmap& pixmap, QVector<QWidget*> customWidgets, UIScope scope);
    UIToolkit::OriginNotificationDialog* showToast(OriginToastManager::ToastSetting setting, OriginToastManager::ToastBehavior behavior, const QString& name, const QString& title, const QString& message, const QString& pixmapStr, bool iconNeedsLoading, QVector<QWidget*> customWidgets, UIScope scope);

    // Returns that active IGO toast
    UIToolkit::OriginNotificationDialog* activeIGOToast(){return mActiveIGOToast;};

    // Set the active IGO toast for hotkey activation
    void setActiveIGOToast(UIToolkit::OriginNotificationDialog*);

    // Debug Menu optionto aid in visual debugging
    void setFakeOIGContext(const bool& fakeOIG) {mFakeOIG = fakeOIG;}
    bool fakeOIGContext() const {return mFakeOIG;}

#ifdef ORIGIN_PC
    void HandleWindowEvents(HWND hwnd);
#endif
signals:
    // This is called after removing a voice identifier so we keep out max count of three
    void newMiniToast(QString);

private slots:
    void onToastDone();
    // For Display purposes
    void showToast(QWidget* toast, UIScope scope){return showToast(toast, scope, ToastType_Default);};
    void showToast(QWidget* toast, UIScope scope, int type);
    // Currently these are only used for in-game voice identifier notifications
    void showMiniToast(QWidget* toast){return showMiniToast(toast, ToastType_Default);};
    void showMiniToast(QWidget* toast, int type);
    // Used to simulate a click event for the active IGO toast
    void onIGOActivated();

    // waiting for the url to load for some pixmaps
    void onUrlLoaded(QNetworkReply*);

private:
    // Creates a specail case toast for additional(4+) voice indicators
    void showVoiceCounter(const QString& title, OriginToastManager::ToastBehavior behavior);

    // Updates a specail case toast for additional(4+) voice indicators
    void updateVoiceCounter();

    // Prepares the notification for IGO
    bool showNotification(QWidget* toast);

    // used to determine if a full screen application is running on PC
    bool isFullscreenAppRunning() const;

    // Grabs empty/oldest toast from pool or created new one
    UIToolkit::OriginNotificationDialog* requestToast(QString name, UIScope scope);

    // return the proper pool given the type and scope
    QList<UIToolkit::OriginNotificationDialog*>& getProperPool(QString type, UIScope scope);

    // Grabs empty/oldest toast from pool or created new one
    UIToolkit::OriginNotificationDialog* requestMiniToast();

    // Returns the oldest toast from the given pool based on lifetime counter
	UIToolkit::OriginNotificationDialog* oldestToast(QList<UIToolkit::OriginNotificationDialog*>& toastPool);

    // Used to determine how many of the voice identifers are currently visible
    int visibleVoiceIdentifiers();

    /// Used to play the sound
    void playSound(const QString& path);

#ifdef ORIGIN_PC
    void SetUpWinHook();
    void ShutdownWinHook();
#endif

    // Pool of *strong pointers* to popup windows.
    // Windows are created lazily as needed, so a
    // null pointer is possible and indicates that the
    // "slot" in the array is available and needs to be
    // created.
    QList<UIToolkit::OriginNotificationDialog*> mToastViewPool;
    QList<UIToolkit::OriginNotificationDialog*> mIGOToastViewPool;
    // These are special cased and need to be in their own pool
    QList<UIToolkit::OriginNotificationDialog*> mVoipIncomingCallPool;
    QList<UIToolkit::OriginNotificationDialog*> mVoiceChatIdentifierPool;

    // Map if icon URL's to toasts
    QMap<QUrl, UIToolkit::OriginNotificationDialog*> mUrlToastViewMap;
    Services::NetworkAccessManager* mNetworkManager;
    
    // Specail case toasts 
    UIToolkit::OriginNotificationDialog* mActiveIGOToast;
    UIToolkit::OriginNotificationDialog* mVoiceChatCounterToast;
    UIToolkit::OriginNotificationDialog* mUserVoiceChatToast;

    // Layout managers for each type of toast
    QSharedPointer<OriginToastLayoutManager> mIGOLayoutManager;
    QSharedPointer<OriginToastLayoutManager> mDesktopLayoutManager;
    QSharedPointer<OriginToastLayoutManager> mVoiceChatIdentifierLayoutManager;
    
    // List of users reprsented by the mVoiceChatCounterToast
    QVector<QString> mVoiceChatCounterList;
    QMediaPlayer* mMediaPlayer;

    // debug menu option
    bool mFakeOIG;

#ifdef ORIGIN_PC
    HWINEVENTHOOK g_hook;
    HWND g_foregroundWindow;
    mutable HWND fullScreenApp;
#endif
};

inline QList<UIToolkit::OriginNotificationDialog*>& ToastViewController::getProperPool(QString type, UIScope scope)
{
    if (type == Services::SETTING_NOTIFY_INCOMINGVOIPCALL.name())
    {
        return mVoipIncomingCallPool;
    }
    else
    {
        return (scope == IGOScope) ? mIGOToastViewPool : mToastViewPool;
    }
}
}
}
#endif // TOASTVIEWCONTROLLER_H