#include "VoiceProxy.h"

#if ENABLE_VOICE_CHAT

#include "engine/login/LoginController.h"
// NOTE: This MUST be included before VoiceDeviceService. Otherwise we get errors for including
// windows.h before winsock.h
#include "engine/voice/VoiceController.h"
#include "engine/voice/VoiceClient.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/platform/PlatformService.h"
#include "services/voice/VoiceDeviceService.h"
#include "services/rest/VoiceServiceResponse.h"
#include "services/rest/VoiceServiceClient.h"
#include "services/voice/VoiceService.h"

#include <QUuid>
#include <QtMultimedia/QSoundEffect>
#include <QStringList>

// XP
#include "OriginApplication.h"

namespace Origin
{
    namespace Client
    {
        namespace JsInterface
        {

            VoiceProxy::VoiceProxy(QObject *parent) 
                : QObject(parent)
                , mCreateChannelResponse(NULL)
                , mGetChannelTokenResponse(NULL)
                , mVoiceController(NULL)
                , mVoiceClient(NULL)
                , mVoiceActivationThreshold(0)
                , mNoneFound(tr("ebisu_client_none_found"))
                , mIsInVoiceSettings(false)
                , mConnectState(ConnectNone)
                , mPlayingIncomingRing()
                , mPlayingIncomingRingCount(0)
                , mPlayingOutgoingRing()
                , mPlayingOutgoingRingCount(0)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                resetChannelId();

                if ((Origin::Services::PlatformService::OSMajorVersion() > 5) /*non Windows XP*/)
                {
                    // Populate our device map
                    mInputDevices = Services::Voice::GetAudioDevices(Services::Voice::AUDIO_DEVICE_INPUT);
                    mOutputDevices = Services::Voice::GetAudioDevices(Services::Voice::AUDIO_DEVICE_OUTPUT);
                    mDefaultInputDevice = Services::Voice::GetDefaultAudioDevice(Services::Voice::AUDIO_DEVICE_INPUT);
                    mDefaultOutputDevice = Services::Voice::GetDefaultAudioDevice(Services::Voice::AUDIO_DEVICE_OUTPUT);

                    // Need to register a callback
                    mAudioNotificationClient = Services::Voice::RegisterDeviceCallback();

                    ORIGIN_VERIFY_CONNECT_EX(mAudioNotificationClient, SIGNAL(deviceAdded(const QString&, const QString&, Origin::Services::Voice::AudioDeviceType)), this, SLOT(onDeviceAdded(const QString&, const QString&, Origin::Services::Voice::AudioDeviceType)), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(mAudioNotificationClient, SIGNAL(deviceRemoved(const QString&, const QString&, Origin::Services::Voice::AudioDeviceType)), this, SLOT(onDeviceRemoved(const QString&, const QString&, Origin::Services::Voice::AudioDeviceType)), Qt::QueuedConnection);
                    ORIGIN_VERIFY_CONNECT_EX(mAudioNotificationClient, SIGNAL(defaultDeviceChanged(const QString&, const QString&, Origin::Services::Voice::AudioDeviceType, Origin::Services::Voice::AudioDeviceRole)), this, SLOT(onDefaultDeviceChanged(const QString&, const QString&, Origin::Services::Voice::AudioDeviceType, Origin::Services::Voice::AudioDeviceRole)), Qt::QueuedConnection);
                }

                Origin::Engine::UserRef user = Engine::LoginController::currentUser();
                if (!user.isNull())
                {
                    mVoiceController = user->voiceControllerInstance();
                    ORIGIN_ASSERT(mVoiceController);
                    mVoiceClient = mVoiceController->getVoiceClient();
                    ORIGIN_ASSERT(mVoiceClient);
                    if (mVoiceClient)
                    {
                        ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceLevel(float)), this, SLOT(onVoiceLevelChanged(float)) );
                        ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceConnected(int, const QString&, const QString&)), this, SLOT(onVoiceConnected(int, const QString&, const QString&)));
                        ORIGIN_VERIFY_CONNECT(mVoiceClient, SIGNAL(voiceClientDisconnectCompleted(bool)), this, SLOT(onVoiceClientDisconnectCompleted(bool)));

                        if ((Origin::Services::PlatformService::OSMajorVersion() < 6) /* Windows XP */)
                        {
                            // get audio devices for XP
                            getAudioDevices();
                            ORIGIN_VERIFY_CONNECT(&OriginApplication::instance(), SIGNAL(deviceChanged()), this, SLOT(onDeviceChanged()));
                        }
                    }
                }

                // Voice activation threshold
                mVoiceActivationThreshold = Services::readSetting(Services::SETTING_VoiceActivationThreshold);

                ORIGIN_VERIFY_CONNECT(Services::SettingsManager::instance(), SIGNAL(settingChanged(const QString&, const Origin::Services::Variant&)),
                    this, SLOT(onSettingChanged(const QString&, const Origin::Services::Variant&)));

                mSelectedAudioInputDevice = mNoneFound;
                mSelectedAudioOutputDevice = mNoneFound;
                activateDevices(AUDIO_DEVICE_NO_CHANGE, Services::Voice::AUDIO_DEVICE_INPUT, "");
                activateDevices(AUDIO_DEVICE_NO_CHANGE, Services::Voice::AUDIO_DEVICE_OUTPUT, "");
                
                ORIGIN_VERIFY_CONNECT(Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOfflineUser()),
                this, SLOT(onNowOfflineUser()));
                ORIGIN_VERIFY_CONNECT(Services::Connection::ConnectionStatesService::instance(), SIGNAL(nowOnlineUser()),
                this, SLOT(onNowOnlineUser()));

                // Origin X - pass voiceCallEvent signals from VoiceController directly to JS
                ORIGIN_VERIFY_CONNECT(mVoiceController, SIGNAL(voiceCallEvent(QVariantMap)), this, SIGNAL(voiceCallEvent(QVariantMap)));
            }

            VoiceProxy::~VoiceProxy()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                Services::Voice::UnregisterDeviceCallback();
                mAudioNotificationClient = NULL;
            }

            bool VoiceProxy::supported() const
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                return isVoiceEnabled; // ENABLE_VOICE_CHAT
            }

            bool VoiceProxy::isSupportedBy(QString const& friendNucleusId) const {
                if (!mVoiceController)
                    return false;

                return mVoiceController->doesFriendSupportVoice(friendNucleusId);
            }


            void VoiceProxy::setInVoiceSettings(bool inVoiceSettings)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if( mIsInVoiceSettings == inVoiceSettings )
                    return;

                mIsInVoiceSettings = inVoiceSettings;

                if( !mIsInVoiceSettings )
                    stopVoiceChannel();
                else
                {
                    mConnectState = ConnectNone;
                    resetChannelId();

                    bool stop = (mInputDevices.size() == 0);
                    updateTestMicrophone(stop);
                }
            }

            void VoiceProxy::startVoiceChannel()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                // 1. If we are in a Voice channel, but it's the one for settings OR
                // 2. A connection is in process
                if (mChannelId == mVoiceClient->getChannelId() || mConnectState == ConnectPending)
                {
                    return;
                }
                // If we are not in a voice channel, create one for the settings
                else if( !mVoiceClient->isInVoice() )
                {
                    mConnectState = ConnectPending;
                    QString randomString = QUuid::createUuid().toString();
                    randomString.replace('{', "").replace('}', "").replace('-', "");

                    QString channelName = "VoiceSettings";
                    Origin::Engine::UserRef user = Engine::LoginController::currentUser();
                    if (!user.isNull())
                    {
                        channelName += "_";
                        channelName += user->eaid();
                    }

                    mCreateChannelResponse.reset(Services::VoiceServiceClient::createChannel(Services::Session::SessionService::currentSession(), "Origin", randomString, channelName));
                    ORIGIN_VERIFY_CONNECT(mCreateChannelResponse.data(), SIGNAL(success()), this, SLOT(onVoiceChannelCreated()));
                    ORIGIN_VERIFY_CONNECT(mCreateChannelResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onVoiceChannelError(Origin::Services::HttpStatusCode)));
                    mChannelId = randomString;
                }
                // Otherwise we are in an actual voice channel and need to disable the button
                else
                {
                    mConnectState = ConnectNone;
                    mVoiceClient->setConnectedToSettings(false);
                    resetChannelId();
                    emit(disableTestMicrophone(true));
                }
            }

            bool VoiceProxy::stopVoiceChannel()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return false;

                bool stopping = false;
                if( mVoiceClient->isConnectedToSettings() )
                {
                    mVoiceClient->stopVoiceConnection("User disconnection", "stopVoiceChannel isConnectedToSettings");
                    stopping = true;
                }
                // If the state is none or pending, but we have the same channel id we could be mid-connection and still need to stop
                if( (mConnectState == ConnectPending || mConnectState == ConnectNone ) && mChannelId.compare(mVoiceClient->getChannelId()) == 0)
                {
                    mVoiceClient->stopVoiceConnection("User disconnection", "stopVoiceChannel ConnectPending");
                    stopping = true;
                }

                return stopping;
            }

            void VoiceProxy::testMicrophoneStart()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                startVoiceChannel();
            }

            void VoiceProxy::testMicrophoneStop()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                stopVoiceChannel();
            }

            void VoiceProxy::changeInputDevice(const QString& device)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if( !mVoiceClient->isInVoice() )
                {
                    mVoiceClient->changeInputDevice(""); // deselect input device
                    return;
                }

                ORIGIN_LOG_EVENT << "Changed input device: " << device;

                // do not inadvertently add a device that is not currently available
                // i.e., mInputDevices[device] adds 'device' if it does not already exists in mInputDevices[]
                QMap<QString, QString>::const_iterator it = mInputDevices.constFind(device);
                if(it != mInputDevices.constEnd())
                {
                    QString deviceId = mInputDevices[device];
                    mVoiceClient->changeInputDevice(deviceId);
                    mSelectedAudioInputDevice = device;
                }
            }

            void VoiceProxy::changeOutputDevice(const QString& device)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if( !mVoiceClient->isInVoice() )
                {
                    mVoiceClient->changeOutputDevice(""); // deselect output device
                    return;
                }

                ORIGIN_LOG_EVENT << "Changed output device: " << device;

                // do not inadvertently add a device that is not currently available
                // i.e., mOutputDevices[device] adds 'device' if it does not already exists in mOutputDevices[]
                QMap<QString, QString>::const_iterator it = mOutputDevices.constFind(device);
                if(it != mOutputDevices.constEnd())
                {
                    QString deviceId = mOutputDevices[device];
                    mVoiceClient->changeOutputDevice(deviceId);
                    mSelectedAudioOutputDevice = device;
                }
            }

            void VoiceProxy::onVoiceChannelCreated()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if(mCreateChannelResponse != NULL)
                {       
                    Services::CreateChannelResponse* resp = static_cast<Services::CreateChannelResponse*>(sender());

                    QString channelId = resp->getChannelId();
                    mGetChannelTokenResponse.reset(Services::VoiceServiceClient::getChannelToken(Services::Session::SessionService::currentSession(), "Origin", channelId));
                    ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(onVoiceTokenReceived()));
                    ORIGIN_VERIFY_CONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onVoiceChannelError(Origin::Services::HttpStatusCode)));
                }
            }

            void VoiceProxy::onVoiceChannelError(Origin::Services::HttpStatusCode)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                emit(enableTestMicrophone(true));
                stopVoiceChannel();
                resetChannelId();
                mConnectState = ConnectNone;
            }

            void VoiceProxy::onVoiceTokenReceived()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if(mGetChannelTokenResponse != NULL)
                {
                    Services::GetChannelTokenResponse* resp = static_cast<Services::GetChannelTokenResponse*>(sender());
                    QString token = resp->getToken();

                    ORIGIN_VERIFY_DISCONNECT(mGetChannelTokenResponse.data(), SIGNAL(success()), this, SLOT(onVoiceTokenReceived()));
                    ORIGIN_VERIFY_DISCONNECT(mGetChannelTokenResponse.data(), SIGNAL(error(Origin::Services::HttpStatusCode)), this, SLOT(onVoiceChannelError(Origin::Services::HttpStatusCode)));

                    if (token.isEmpty())
                        return;

                    QString voipAddressBefore = Origin::Services::readSetting(Origin::Services::SETTING_VoipAddressBefore);
                    QString voipAddressAfter = Origin::Services::readSetting(Origin::Services::SETTING_VoipAddressAfter);
                    if (!voipAddressBefore.isEmpty() && !voipAddressAfter.isEmpty())
                    {
                        token.replace(voipAddressBefore, voipAddressAfter);
                    }

                    bool connected = mVoiceClient->createVoiceConnection(token, Engine::Voice::VoiceSettings(), false);
                    if(!connected)
                        onVoiceChannelError(Origin::Services::Http200Ok); // not sure what the appropriate error is
                }
            }

            void VoiceProxy::onDeviceAdded(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (type == Services::Voice::AUDIO_DEVICE_INPUT)
                {
                    mInputDevices[deviceName] = deviceId;
                }
                else if (type == Services::Voice::AUDIO_DEVICE_OUTPUT)
                {
                    mOutputDevices[deviceName] = deviceId;
                }
                
                activateDevices(AUDIO_DEVICE_ADDED, type, deviceName);

                emit (deviceAdded(deviceName));  // update device list in UI

                if (type == Services::Voice::AUDIO_DEVICE_INPUT)
                {
                    updateTestMicrophone();
                }
            }

            void VoiceProxy::onDeviceRemoved(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (type == Services::Voice::AUDIO_DEVICE_INPUT)
                {
                    mInputDevices.remove(deviceName);
                }
                else if (type == Services::Voice::AUDIO_DEVICE_OUTPUT)
                {
                    mOutputDevices.remove(deviceName);
                }

                activateDevices(AUDIO_DEVICE_REMOVED, type, deviceName);

                emit (deviceRemoved()); // update device list in UI

                if (type == Services::Voice::AUDIO_DEVICE_INPUT)
                {
                    bool stop = (mInputDevices.size() == 0);
                    updateTestMicrophone(stop);
                }
            }

            // gets called AFTER onDeviceAdded() or onDeviceRemoved()
            void VoiceProxy::onDefaultDeviceChanged(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type, Origin::Services::Voice::AudioDeviceRole role)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                // only consider default Communications device changes
                if (role == Services::Voice::AUDIO_DEVICE_COMMUNICATIONS)
                {
                    if (type == Services::Voice::AUDIO_DEVICE_INPUT)
                    {
                        mDefaultInputDevice = deviceName;

                    }
                    else if (type == Services::Voice::AUDIO_DEVICE_OUTPUT)
                    {
                        mDefaultOutputDevice = deviceName;
                    }

                    // at this point, no devices are being plugged or unplugged
                    activateDevices(AUDIO_DEVICE_NO_CHANGE, type, "");

                    emit (defaultDeviceChanged());
                }
            }

            // XP
            void VoiceProxy::onDeviceChanged()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (!isVoiceEnabled)
                    return;

                getAudioDevices();
                Origin::Services::Voice::AudioDeviceType type = activateDevices(AUDIO_DEVICE_CHANGED, Services::Voice::AUDIO_DEVICE_INPUT_OUTPUT, "");

                emit (deviceChanged()); // update device list in UI

                if (type == Services::Voice::AUDIO_DEVICE_INPUT)
                {
                    bool stop = (mInputDevices.size() == 0);
                    updateTestMicrophone(stop);
                }
            }

            void VoiceProxy::onVoiceLevelChanged(float level)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                emit voiceLevel(level);

                if( level > mVoiceActivationThreshold )
                    emit(overThreshold());
                else
                    emit(underThreshold());
            }

            void VoiceProxy::onVoiceConnected(int chid, const QString& channelId, const QString& channelDescription)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                // The connected 'channelId' is the same as what the VoiceSettings was trying to connect to (i.e., mChannelId)
                if( channelId == mChannelId )
                {
                    mConnectState = ConnectComplete;
                    mVoiceClient->setConnectedToSettings(true);
                    emit(enableTestMicrophone(false));

                    // allow users to hear themselves through the mic
                    mVoiceClient->enableRemoteEcho(true);
                    mVoiceClient->enablePushToTalk(true);
                    emit(voiceConnected());
                }
                else // a Voice Chat started
                {
                    mVoiceClient->setConnectedToSettings(false);
                    resetChannelId();
                    emit(disableTestMicrophone(true));
                    emit(voiceConnected());
                }

                activateDevices(AUDIO_DEVICE_NO_CHANGE, Services::Voice::AUDIO_DEVICE_INPUT_OUTPUT, "");

                // Notify the voice client that the audio devices have been activated (successful or not)
                // Note: it's really shitty that the audio devices are being activated in the proxy - should be
                // happening in the voice client.
                QMetaObject::invokeMethod(mVoiceClient, "onVoiceConnectedAndDevicesActivated", Qt::QueuedConnection);
            }

            void VoiceProxy::onVoiceDisconnected(const QString& reasonType, const QString& reasonDescription)
            {
            }

            void VoiceProxy::onVoiceClientDisconnectCompleted(bool joiningVoice)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                bool userOnline = false;
                Origin::Engine::UserRef user = Engine::LoginController::currentUser();
                if (!user.isNull())
                {
                    if( Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
                    {
                        userOnline = true;
                    }
                }

                // was in Test Microphone
                if( mVoiceClient->isConnectedToSettings() ) // mConnectState == ConnectComplete
                {
                    mVoiceClient->setConnectedToSettings(false);
                    resetChannelId();
                    mConnectState = ConnectNone;

                    // only enable if user is online and a mic is available
                    if (userOnline && !mInputDevices.empty())
                        emit(enableTestMicrophone(true));

                    // prevent user from hearing their own echo
                    mVoiceClient->enableRemoteEcho(false);
                    mVoiceClient->enablePushToTalk(false);

                    // let UI know that the voice channel was disconnected
                    emit(voiceDisconnected());
                }
                else // was in Voice Chat
                {
                    // only enable if user is online and a mic is available
                    if (userOnline && !mInputDevices.empty())
                        emit(enableTestMicrophone(true));

                    emit(voiceDisconnected());
                }

                activateDevices(AUDIO_DEVICE_NO_CHANGE, Services::Voice::AUDIO_DEVICE_INPUT_OUTPUT, "");
            }

            void VoiceProxy::onSettingChanged(const QString& setting, const Origin::Services::Variant& value)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (setting == Origin::Services::SETTING_VoiceActivationThreshold.name())
                {
                    mVoiceActivationThreshold = value;
                }
            }

            QStringList VoiceProxy::audioInputDevices()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return QStringList();

                QStringList devices;
                QMapIterator<QString, QString> iter(mInputDevices);
                while (iter.hasNext()) 
                {
                    iter.next();
                    devices.push_back(iter.key());
                }

                return devices;
            }

            QStringList VoiceProxy::audioOutputDevices()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return QStringList();

                QStringList devices;
                QMapIterator<QString, QString> iter(mOutputDevices);
                while (iter.hasNext()) 
                {
                    iter.next();
                    devices.push_back(iter.key());
                }

                return devices;
            }

            QString VoiceProxy::selectedAudioInputDevice()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return QString();

                return mSelectedAudioInputDevice;
            }

            QString VoiceProxy::selectedAudioOutputDevice()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return QString();

                return mSelectedAudioOutputDevice;
            }

            int VoiceProxy::networkQuality()
            {
                if( mVoiceClient )
                    return mVoiceClient->getNetworkQuality();
                return 0; 
            }

            bool VoiceProxy::isInVoice()
            {
                if( mVoiceClient )
                    return mVoiceClient->isInVoice();
                return false;
            }

            Origin::Services::Voice::AudioDeviceType VoiceProxy::activateDevices(AudioDeviceChangeType changeType, Origin::Services::Voice::AudioDeviceType type, const QString& deviceName)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return Services::Voice::AUDIO_DEVICE_INVALID;

                uint outputDeviceChanged = 0;
                uint inputDeviceChanged = 0;
                Origin::Services::Voice::AudioDeviceType deviceType = Services::Voice::AUDIO_DEVICE_INVALID;

                // determine the appropriate device selection
                if (type == Services::Voice::AUDIO_DEVICE_INPUT || type == Services::Voice::AUDIO_DEVICE_INPUT_OUTPUT)
                {
                    QString savedInputDevice = Origin::Services::readSetting(Origin::Services::SETTING_VoiceInputDevice);
                    bool validSavedInputDevice = false;

                    // check for a valid saved input device
                    if (!savedInputDevice.isEmpty())
                    {
                        QMap<QString, QString>::const_iterator it = mInputDevices.constFind(savedInputDevice);
                        if(it != mInputDevices.constEnd())
                        {
                            validSavedInputDevice = true;
                        }
                    }

                    // activate input device
                    if (validSavedInputDevice)
                    {
                        if (mSelectedAudioInputDevice != savedInputDevice)
                            inputDeviceChanged = 1;

                        mSelectedAudioInputDevice = savedInputDevice;
                        changeInputDevice(savedInputDevice);
                    }
                    else // no valid saved device
                    {
                        if (mInputDevices.size())
                        {
                            if (mSelectedAudioInputDevice != mDefaultInputDevice)
                                inputDeviceChanged = 1;

                            mSelectedAudioInputDevice = mDefaultInputDevice;
                            // pick the 'Default Communications Device'
                            changeInputDevice(mDefaultInputDevice);
                        }
                        else
                        {
                            if (mSelectedAudioInputDevice != mNoneFound)
                                inputDeviceChanged = 1;

                            mSelectedAudioInputDevice = mNoneFound;

                            //mVoiceClient->stopVoiceConnection();
                        }
                    }
                }
                
                if (type == Services::Voice::AUDIO_DEVICE_OUTPUT || type == Services::Voice::AUDIO_DEVICE_INPUT_OUTPUT)
                {
                    QString savedOutputDevice = Origin::Services::readSetting(Origin::Services::SETTING_VoiceOutputDevice);
                    bool validSavedOutputDevice = false;

                    // check for a valid saved output device
                    if (!savedOutputDevice.isEmpty())
                    {
                        QMap<QString, QString>::const_iterator it = mOutputDevices.constFind(savedOutputDevice);
                        if(it != mOutputDevices.constEnd())
                        {
                            validSavedOutputDevice = true;
                        }
                    }

                    // activate output device
                    if (validSavedOutputDevice)
                    {
                        if (mSelectedAudioOutputDevice != savedOutputDevice)
                            outputDeviceChanged = 1;

                        mSelectedAudioOutputDevice = savedOutputDevice;
                        changeOutputDevice(savedOutputDevice);
                    }
                    else // no valid saved device
                    {
                        if (mOutputDevices.size())
                        {
                            if (mSelectedAudioOutputDevice != mDefaultOutputDevice)
                                outputDeviceChanged = 1;

                            mSelectedAudioOutputDevice = mDefaultOutputDevice;
                            // pick 'Default Communications Device'
                            changeOutputDevice(mDefaultOutputDevice);
                        }
                        else
                        {
                            if (mSelectedAudioOutputDevice != mNoneFound)
                                outputDeviceChanged = 1;

                            mSelectedAudioOutputDevice = mNoneFound;

                            //mVoiceClient->stopVoiceConnection();
                        }
                    }
                }

                // make sure only one or neither changed
                ORIGIN_ASSERT(inputDeviceChanged + outputDeviceChanged < 2);
                if(outputDeviceChanged == 1)
                    deviceType = Services::Voice::AUDIO_DEVICE_OUTPUT;
                else if(inputDeviceChanged == 1)
                    deviceType = Services::Voice::AUDIO_DEVICE_INPUT;

                return deviceType;
            }

            void VoiceProxy::getAudioDevices()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                mInputDevices = mVoiceClient->getCaptureDevices();
                mOutputDevices = mVoiceClient->getPlaybackDevices();

                if( mInputDevices.size() )
                {
                    // get default input device
                    QString defaultInputDeviceGuid = Services::Voice::GetDefaultAudioDeviceGuid(Services::Voice::AUDIO_DEVICE_INPUT);
                    if( defaultInputDeviceGuid.isEmpty() )
                    {
                         // If there is no default input device, there must be no input devices
                        mInputDevices.clear();
                    }
                    QMapIterator<QString,QString> inputIt(mInputDevices);
                    while( inputIt.hasNext() ) {
                        inputIt.next();
                        if( inputIt.value() == defaultInputDeviceGuid )
                        {
                            mDefaultInputDevice = inputIt.key();
                        }
                    }
                }

                if( mOutputDevices.size() )
                {
                    // get default output device
                    QString defaultOutputDeviceGuid = Services::Voice::GetDefaultAudioDeviceGuid(Services::Voice::AUDIO_DEVICE_OUTPUT);
                    if( defaultOutputDeviceGuid.isEmpty() )
                    {
                        // If there is no default output device, there must be no output devices
                        mOutputDevices.clear();
                    }
                    QMapIterator<QString,QString> outputIt(mOutputDevices);
                    while( outputIt.hasNext() ) {
                        outputIt.next();
                        if( outputIt.value() == defaultOutputDeviceGuid )
                        {
                            mDefaultOutputDevice = outputIt.key();
                        }
                    }
                }
            }

            VoiceProxy::SoundRef VoiceProxy::playAudio(const QString& audioFilePath, int numberOfLoops, const QObject * receiver, const char* member)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return VoiceProxy::SoundRef();

                QString fullPath(QDir::toNativeSeparators(Origin::Services::PlatformService::resourcesDirectory() + "/" + audioFilePath));
                ORIGIN_LOG_DEBUG << "Playing " << fullPath;
                VoiceProxy::SoundRef soundToPlay(new QSoundEffect(), &QObject::deleteLater);
                if (!soundToPlay.isNull())
                {
                    soundToPlay->setSource(QUrl::fromLocalFile(fullPath));
                    soundToPlay->setLoopCount((numberOfLoops == -1) ? QSoundEffect::Infinite : numberOfLoops);
                    soundToPlay->setVolume(Origin::Services::PlatformService::GetCurrentApplicationVolume());
                    ORIGIN_VERIFY_CONNECT(soundToPlay.data(), SIGNAL(playingChanged()), receiver, member);
                    soundToPlay->play();
                }
                return soundToPlay;
            }

            void VoiceProxy::stopAudio(VoiceProxy::SoundRef soundToStop)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (soundToStop.isNull())
                    return;

                soundToStop->stop();
            }

            void VoiceProxy::playIncomingRing()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (mPlayingIncomingRing.isNull())
                {
                    mPlayingIncomingRing = playAudio("sounds/IncomingVoiceChat.wav", 120 /* should be -1? */, this, SLOT(onPlayingIncomingRingChanged()));
                }

                if (!mPlayingIncomingRing.isNull())
                {
                    ++mPlayingIncomingRingCount;
                }
            }

            void VoiceProxy::stopIncomingRing()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (mPlayingIncomingRingCount > 0)
                    --mPlayingIncomingRingCount;

                if (mPlayingIncomingRingCount == 0 && !mPlayingIncomingRing.isNull())
                {
                    ORIGIN_LOG_DEBUG << "*** Stopping incoming ring";
                    stopAudio(mPlayingIncomingRing);
                    mPlayingIncomingRing.clear();
                }
            }

            void VoiceProxy::onPlayingIncomingRingChanged()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (!mPlayingIncomingRing.isNull() && !mPlayingIncomingRing->isPlaying())
                {
                    disconnect(mPlayingIncomingRing.data(), SIGNAL(playingChanged()));
                    mPlayingIncomingRing.clear();
                    mPlayingIncomingRingCount = 0;
                }
            }

            void VoiceProxy::playOutgoingRing()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (mPlayingOutgoingRing.isNull())
                {
                    mPlayingOutgoingRing = playAudio("sounds/OutgoingVoiceChat.wav", 120 /* should be -1? */, this, SLOT(onPlayingOutgoingRingChanged()));
                }

                if (!mPlayingOutgoingRing.isNull())
                {
                    ++mPlayingOutgoingRingCount;
                }
            }

            void VoiceProxy::onPlayingOutgoingRingChanged()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (!mPlayingOutgoingRing.isNull() && !mPlayingOutgoingRing->isPlaying())
                {
                    disconnect(mPlayingOutgoingRing.data(), SIGNAL(playingChanged()));
                    mPlayingOutgoingRing.clear();
                    mPlayingOutgoingRingCount = 0;
                }
            }

            void VoiceProxy::stopOutgoingRing()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if (mPlayingOutgoingRingCount > 0)
                    --mPlayingOutgoingRingCount;

                if (mPlayingOutgoingRingCount == 0 && !mPlayingOutgoingRing.isNull())
                {
                    ORIGIN_LOG_DEBUG << "*** Stopping outgoing ring";
                    stopAudio(mPlayingOutgoingRing);
                    mPlayingOutgoingRing.clear();
                }
            }

            void VoiceProxy::resetChannelId()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                mChannelId = "NOT_SET";
            }

            void VoiceProxy::updateTestMicrophone(bool stop)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                // in Voice Chat
                if (mVoiceClient->isInVoice() && !mVoiceClient->isConnectedToSettings())
                {
                    emit (disableTestMicrophone(true));
                    testMicrophoneStop();
                    return;
                }

                if (stop)
                {
                    emit (disableTestMicrophone(true));
                    testMicrophoneStop();
                }
                else
                {
                    Origin::Engine::UserRef user = Engine::LoginController::currentUser();
                    if (!user.isNull())
                    {
                        if( Services::Connection::ConnectionStatesService::isUserOnline(user->getSession()))
                        {
                            emit (enableTestMicrophone(false/*resetButton*/));
                        }
                        else
                        {
                            emit (disableTestMicrophone(true/*resetButton*/));
                        }
                    }
                }
            }

            void VoiceProxy::onNowOfflineUser()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if( mIsInVoiceSettings )
                {
                    emit (disableTestMicrophone(true));

                    // Testing Mic
                    if(mVoiceClient && mVoiceClient->isConnectedToSettings())
                        testMicrophoneStop();
               }
            }

            void VoiceProxy::onNowOnlineUser()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if( !isVoiceEnabled)
                    return;

                if( mIsInVoiceSettings )
                {
                    if (!mInputDevices.empty())
                        emit (enableTestMicrophone(false));
                }
            }

            // Origin X - from ConversationProxy.h
            void VoiceProxy::joinVoice(const QString& id, const QStringList& participants)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (isVoiceEnabled)
                {
                    if (mVoiceController)
                    {
                        mVoiceController->joinVoice(id, participants);
                    }
                }
            }

            void VoiceProxy::leaveVoice(const QString& id)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (isVoiceEnabled)
                {
                    if (mVoiceController)
                    {
                        mVoiceController->leaveVoice(id);
                    }
                }
            }

            void VoiceProxy::muteSelf()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (isVoiceEnabled)
                {
                    if (mVoiceController)
                    {
                        mVoiceController->muteSelf();
                    }
                }
            }

            void VoiceProxy::unmuteSelf()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (isVoiceEnabled)
                {
                    if (mVoiceController)
                    {
                        mVoiceController->unmuteSelf();
                    }
                }
            }

            void VoiceProxy::showToast(const QString& event, const QString& originId, const QString& conversationId)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (isVoiceEnabled)
                {
                    if (mVoiceController)
                    {
                        mVoiceController->showToast(event, originId, conversationId);
                    }
                }
            }

            void VoiceProxy::showSurvey(const QString& voiceChannelId)
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (isVoiceEnabled)
                {
                    if (mVoiceController)
                    {
                        mVoiceController->showSurvey(voiceChannelId);
                    }
                }
            }

            QString VoiceProxy::channelId()
            {
                bool isVoiceEnabled = Origin::Services::VoiceService::isVoiceEnabled();
                if (isVoiceEnabled)
                {
                    if (mVoiceController)
                    {
                        return mVoiceController->voiceChannelId();
                    }
                }

                return QString();
            }
        }
    }
}

#endif // ENABLE_VOICE_CHAT