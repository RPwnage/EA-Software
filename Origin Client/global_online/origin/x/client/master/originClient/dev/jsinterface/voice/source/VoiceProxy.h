#ifndef _VOICEPROXY_H
#define _VOICEPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#if ENABLE_VOICE_CHAT

#include <QObject>
#include <QSharedPointer>
#include <QStringList>
#include <QMap>
#include <QTcpSocket>

#include "services/settings/SettingsManager.h"

class QSoundEffect;

namespace Origin
{
    namespace Engine
    {
        namespace Voice
        {
            class VoiceController;
            class VoiceClient;
        }
    }

    namespace Services
    {
        class CreateChannelResponse;
        class GetChannelTokenResponse;

        namespace Voice
        {
            enum AudioDeviceType : int;
            enum AudioDeviceRole : int;

            class AudioNotificationClient;
        }
    }

    namespace Client
    {
        namespace JsInterface
        {

        class VoiceProxy : public QObject
        {
            Q_OBJECT
        public:
            enum ConnectState
            {
                ConnectNone,
                ConnectPending,
                ConnectComplete
            };

            VoiceProxy(QObject *parent = NULL);
            ~VoiceProxy();

            /// \brief Is this proxy supported?
            Q_PROPERTY(bool supported READ supported);
            bool supported() const;

            /// \brief Returns true if friend identified by their nucleusId supports voice; false otherwise.
            /// The friend must currently be connected in a voice chat for this function to return a valid value.
            Q_INVOKABLE bool isSupportedBy(QString const& friendNucleusId) const;

            /// \brief Set whether we are in the Voice Settings page
            Q_INVOKABLE void setInVoiceSettings(bool inVoiceSettings);

            /// \brief Start a voice channel
            Q_INVOKABLE void startVoiceChannel();

            /// \brief Stop a voice channel
            Q_INVOKABLE bool stopVoiceChannel();

            /// \brief Start testing the users microphone
            Q_INVOKABLE void testMicrophoneStart();

            /// \brief Stop testing the users microphone
            Q_INVOKABLE void testMicrophoneStop();

            /// \brief Changes the input device
            Q_INVOKABLE void changeInputDevice(const QString& device);

            /// \brief Changes the output device
            Q_INVOKABLE void changeOutputDevice(const QString& device);

            /// \brief Plays the incoming ring sound
            Q_INVOKABLE void playIncomingRing();

            /// \brief Plays the outgoing ring sound
            Q_INVOKABLE void playOutgoingRing();

            /// \brief Stops the incoming ring sound
            Q_INVOKABLE void stopIncomingRing();

            /// \brief Stops the outgoing ring sound
            Q_INVOKABLE void stopOutgoingRing();

            ////////////////////////////////////////
            // Origin X - from ConversationProxy.h
            ////////////////////////////////////////

            /// \brief Join a Voice Chat
            Q_INVOKABLE void joinVoice(const QString& id, const QStringList& participants = QStringList());

            /// \brief Leave the Voice Chat
            Q_INVOKABLE void leaveVoice(const QString& id);

            /// \brief Mute self
            Q_INVOKABLE void muteSelf();

            /// \brief Unmute self
            Q_INVOKABLE void unmuteSelf();

            /// \brief Show Toasty Notification
            Q_INVOKABLE void showToast(const QString& event, const QString& originId, const QString& conversationId);

            /// \brief Show Voice Survey
            Q_INVOKABLE void showSurvey(const QString& voiceChannelId);

            Q_PROPERTY(QStringList audioInputDevices READ audioInputDevices)
            /// \brief List of audio input devices
            QStringList audioInputDevices();

            Q_PROPERTY(QStringList audioOutputDevices READ audioOutputDevices)
            /// \brief List of audio output devices
            QStringList audioOutputDevices();

            Q_PROPERTY(QString selectedAudioInputDevice READ selectedAudioInputDevice)
            /// \brief Selected audio input device
            QString selectedAudioInputDevice();

            Q_PROPERTY(QString selectedAudioOutputDevice READ selectedAudioOutputDevice)
            /// \brief Selected audio output device
            QString selectedAudioOutputDevice();

            Q_PROPERTY(int networkQuality READ networkQuality)
            /// \brief Quality of network (0-3): 0 - no connection, 1 - poor, 2 - ok, 3 - good
            int networkQuality();

            Q_PROPERTY(bool isInVoice READ isInVoice)
            /// \brief Returns whether a voice channel is established
            bool isInVoice();

            Q_PROPERTY(QString channelId READ channelId)
            /// \brief Returns channel id of current voice chat
            QString channelId();

        signals:
            void deviceAdded(const QString& deviceName);
            void deviceRemoved();
            void defaultDeviceChanged();
            void deviceChanged(); // XP

            void voiceLevel(float level);
            void underThreshold();
            void overThreshold();
            void voiceConnected();
            void voiceDisconnected();
            void enableTestMicrophone(bool resetButton);
            void disableTestMicrophone(bool resetButton);
            void clearLevelIndicator();

            ////////////////////////////////////////
            // Origin X - sent to JS
            ////////////////////////////////////////
            void voiceCallEvent(QVariantMap voiceCallEventObject);

        protected slots:
            void onVoiceChannelCreated();
            void onVoiceChannelError(Origin::Services::HttpStatusCode);
            void onVoiceTokenReceived();
            void onNowOfflineUser();
            void onNowOnlineUser();

            void onDeviceAdded(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type);
            void onDeviceRemoved(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type);
            void onDefaultDeviceChanged(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type, Origin::Services::Voice::AudioDeviceRole role);
            void onVoiceLevelChanged(float level);
            void onDeviceChanged(); // XP

            // for settings
            void onVoiceConnected(int chid, const QString& channelId, const QString& channelDescription);
            void onVoiceDisconnected(const QString& reasonType, const QString& reasonDescription);

            void onVoiceClientDisconnectCompleted(bool joiningVoice);

            void onPlayingIncomingRingChanged();
            void onPlayingOutgoingRingChanged();

        private slots:
            void onSettingChanged(const QString& setting, const Origin::Services::Variant& value);

        private:
            enum AudioDeviceChangeType { AUDIO_DEVICE_NO_CHANGE, AUDIO_DEVICE_ADDED, AUDIO_DEVICE_REMOVED, AUDIO_DEVICE_CHANGED };

            // Determines which device to select and activate depending on:
            // 1) device preference
            // 2) availability of devices
            // 3) default OS device
            //
            // Returns type (input or output) of device changed (for XP)
            Origin::Services::Voice::AudioDeviceType activateDevices(AudioDeviceChangeType changeType, Origin::Services::Voice::AudioDeviceType type, const QString& deviceName);

            void getAudioDevices(); // XP

            typedef QSharedPointer<QSoundEffect> SoundRef;

            SoundRef playAudio(const QString& audioFilePath, int numberOfLoops, const QObject * receiver, const char* member);
            void stopAudio(SoundRef soundToStop);

            void resetChannelId();
            void updateTestMicrophone(bool stop = false);

        private:
            QScopedPointer<Services::CreateChannelResponse> mCreateChannelResponse;
            QScopedPointer<Services::GetChannelTokenResponse> mGetChannelTokenResponse;

            QMap<QString, QString> mOutputDevices;
            QMap<QString, QString> mInputDevices;

            Services::Voice::AudioNotificationClient* mAudioNotificationClient;

            QPointer<Engine::Voice::VoiceController> mVoiceController;
            Engine::Voice::VoiceClient* mVoiceClient;
            int mVoiceActivationThreshold;

            QString mDefaultInputDevice;
            QString mDefaultOutputDevice;
            QString mSelectedAudioInputDevice;
            QString mSelectedAudioOutputDevice;
            const QString mNoneFound;

            QString mChannelId;
            bool mIsInVoiceSettings;
            ConnectState mConnectState;

            SoundRef mPlayingIncomingRing;
            unsigned int mPlayingIncomingRingCount;
            SoundRef mPlayingOutgoingRing;
            unsigned int mPlayingOutgoingRingCount;
        };

        }
    }
}

#else

#include <QObject>

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {
            
            class VoiceProxy : public QObject
            {
                Q_OBJECT
            public:
                VoiceProxy(QObject *parent = NULL) { ; }
                ~VoiceProxy() { ; }
                
                /// \brief Is this proxy supported?
                Q_PROPERTY(bool supported READ supported);
                bool supported() const { return false; }
            };
            
        }
    }
}

#endif //ENABLE_VOICE_CHAT

#endif //VOICE_PROXY_H
