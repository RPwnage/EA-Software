#ifndef ORIGINTOASTMANAGER_H
#define ORIGINTOASTMANAGER_H

#include <QtCore>
#include <QObject>
#include <QString>
#include <QVector>
#include "UIScope.h"
#include "services/settings/Setting.h"
#include "services/plugin/PluginAPI.h"

//class QNetworkAccessManager;
class QNetworkReply;

namespace Origin
{
    namespace UIToolkit
    {
        class OriginNotificationDialog;
    }
    namespace Client
    {
        class ToastViewController;

        class ORIGIN_PLUGIN_API OriginToastManager : public QObject
        {
            Q_OBJECT

        public:
 
            enum UserVoiceType
            {
                UserVoiceType_RemoteUser,
                UserVoiceType_CurrentUserTalking,
                UserVoiceType_CurrentUserMuted
            };

            struct ORIGIN_PLUGIN_API ToastSetting
            {
                enum AlertMethod
                {
                    TOAST_DISABLED,
                    TOAST_SOUND_ONLY,
                    TOAST_ALERT_ONLY,
                    TOAST_SOUND_ALERT
                };

                ToastSetting();
                ToastSetting(const QString& name, const QString& iconPath, const QString& soundPath);
                ToastSetting(const QString& iconPath, const QString& soundPath, const AlertMethod& alert);

                QString iconPath(){return mIconPath;};
                QString soundPath(){return mSoundPath;};
                AlertMethod alertMethod();
                void setAlertMethod(AlertMethod newMethod){mAlertMethod = newMethod;};
                void setIconPath(const QString& iconPath){mIconPath = iconPath;};
            private:
                QString mIconPath;
                QString mSoundPath;
                AlertMethod mAlertMethod;

            };

        struct ORIGIN_PLUGIN_API ToastBehavior
            {
                ToastBehavior();

                void setShowBubble(bool show){mShowBubble = show;};
                void setIGOFlags(bool setting){mIGOFlags = setting;};
                void setAcceptClick(bool setting){mAcceptClick = setting;};
                void setDelayShow(bool delay){mDelayShow = delay;};
                void setCloseTime(int time){mCloseTime = time;};
                void setToastType(int type){mType = type;};
                void setMaxOpacity(float max){mOpacity = max;};

                const bool  showBubble(){return mShowBubble;};
                const bool  IGOFlags(){return mIGOFlags;};
                const bool  acceptClick(){return mAcceptClick;};
                const bool  delayShow(){return mDelayShow;};
                const int   closeTime(){return mCloseTime;};
                const int   toastType(){return mType;};
                const float maxOpacity(){return mOpacity;};

            private:
                bool  mShowBubble;
                bool  mIGOFlags;
                bool  mAcceptClick;
                bool  mDelayShow;
                int   mCloseTime;
                int   mType;
                float mOpacity;
            };

            OriginToastManager();
            ~OriginToastManager();

            QVector<QWidget*> setUpCustomWidgets(Origin::Client::UIScope scope, QString name, OriginToastManager::ToastSetting settings);

            UIToolkit::OriginNotificationDialog* showToast(const QString& name, const QString& title);
            UIToolkit::OriginNotificationDialog* showToast(const QString& name, const QString& title, const QString& message);
            UIToolkit::OriginNotificationDialog* showToast(const QString& name, const QString& title, const QString& message, const QPixmap& pixmap);
            UIToolkit::OriginNotificationDialog* showAchievementToast(const QString& name, const QString& title, const QString& message, int xp, const QString& pixmapStr, bool iconNeedsLoading);
#if ENABLE_VOICE_CHAT
            void removeVoiceChatIdentifier(const QString& userId);
            void removeAllVoiceChatIdentifiers();
#endif
            bool processActiveToast(const QString& name);
            QSharedPointer<ToastViewController> controller(){return mController;};

        public slots:
            void enableFakeOIG(const bool& fakeOIG);
#if ENABLE_VOICE_CHAT
            void showVoiceChatIdentifier(const QString& userID){showVoiceChatIdentifier(userID, UserVoiceType_RemoteUser);};
            void showVoiceChatIdentifier(const QString& userID, int voiceType);
#endif
        private slots:
            void updateData(const QString& settingName, Origin::Services::Variant const& val);

        private:
            bool isFullscreenAppRunning() const;
            UIToolkit::OriginNotificationDialog* requestToast(int& index, QString name);

            typedef QMap<QString, ToastSetting> ToastSettingMap;
            ToastSettingMap mToastSettings;

            QSharedPointer<ToastViewController> mController;
        };
    } // namespace Client
} // namespace Origin
#endif // ORIGINTOASTMANAGER_H
