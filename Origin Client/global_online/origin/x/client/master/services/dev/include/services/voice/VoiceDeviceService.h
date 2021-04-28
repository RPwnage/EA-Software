// Copyright (C) 2013 Electronic Arts. All rights reserved.

#ifndef VOICE_DEVICE_SERVICE_H
#define VOICE_DEVICE_SERVICE_H

#if ENABLE_VOICE_CHAT

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p) { if (p) { (p)->Release();   (p)=NULL; } }
#endif

#include <QMap>
#include <QObject>
#include <mmdeviceapi.h>

// XP
#include "Dbt.h"

namespace Origin
{
    namespace Services
    {
        namespace Voice
        {
	        enum AudioDeviceType
	        {
                AUDIO_DEVICE_INVALID,
	            AUDIO_DEVICE_INPUT,
	            AUDIO_DEVICE_OUTPUT,
                AUDIO_DEVICE_INPUT_OUTPUT // XP
	        };
	
	        enum AudioDeviceRole
	        {
	            AUDIO_DEVICE_CONSOLE,
	            AUDIO_DEVICE_COMMUNICATIONS,
	            AUDIO_DEVICE_MULTIMEDIA
	        };
	
	        struct DeviceInfo
	        {
	            DeviceInfo(LPWSTR name, AudioDeviceType t)
	            {
	                if(name != NULL)
	                {
	                    valid = true;
	                    deviceName = new WCHAR[256];
	                    wcsncpy_s(deviceName, 256, name, wcslen(name));
	                    type = t;
	                }
	                else
	                {
	                    valid = false;
	                }
	            }
	
	
	            DeviceInfo()
	            {
	                valid = false;
	            }
	
	            ~DeviceInfo()
	            {
	                if (valid)
	                {
	                    delete[] deviceName;
	                }        
	            }
	
	            LPWSTR deviceName;
	            AudioDeviceType type;
	            bool valid;
	        };
	
	        class AudioNotificationClient : public QObject, public IMMNotificationClient
	        {
	            Q_OBJECT
	        public:
	            AudioNotificationClient();
	            virtual ~AudioNotificationClient();
	
	            ULONG STDMETHODCALLTYPE AddRef();
	
	            ULONG STDMETHODCALLTYPE Release();
	
	            HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface);
	
	            // Callback methods for device-event notifications.
	
	            HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId);
	
	            HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId);
	
	            HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId);
	
	            HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState);
	
	            HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key);
	
	        private:
	            DeviceInfo* GetDeviceInfo(LPCWSTR deviceId);
	
	        signals:
	            void deviceAdded(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type);
	            void deviceRemoved(const QString& deviceRemoved, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type);
	            void defaultDeviceChanged(const QString& deviceName, const QString& deviceId, Origin::Services::Voice::AudioDeviceType type, Origin::Services::Voice::AudioDeviceRole role);

	        private:
	
	            LONG refCount;
	        };
	
	        bool IsAudioDevicePresent(AudioDeviceType device);
	        QMap<QString, QString> GetAudioDevices(AudioDeviceType device);
	        QString GetDefaultAudioDevice(AudioDeviceType device);
	        AudioNotificationClient* RegisterDeviceCallback();
	        void UnregisterDeviceCallback();

            // XP
            bool RegisterDeviceNotification(HWND hWnd, HDEVNOTIFY *hDeviceNotify);
            bool UnregisterDeviceNotification(HDEVNOTIFY handle);
            bool GetDeviceName( QString& deviceName, PDEV_BROADCAST_DEVICEINTERFACE pDevInf, WPARAM wParam );
            QString GetDefaultAudioDeviceGuid(Origin::Services::Voice::AudioDeviceType deviceType);
        }
    }
}

#endif //ENABLE_VOICE_CHAT

#endif //VOICE_DEVICE_SERVICE_H