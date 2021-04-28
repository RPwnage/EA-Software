// Copyright (C) 2013 Electronic Arts. All rights reserved.

#include "services/log/LogService.h"
#include "VoiceDeviceService.h"
#include "services/debug/DebugService.h"

#if ENABLE_VOICE_CHAT

#include <windows.h>
#include <endpointvolume.h> 
#include <Functiondiscoverykeys_devpkey.h>

// XP
#include <QString>
#include "SetupAPI.h"
#include "DSound.h"
#include <SonarAudio/dscommon.h>

namespace Origin
{
    namespace Services
    { 
        namespace Voice
        {
	
	        AudioNotificationClient* audioNotificationClient = NULL;
	
	        AudioNotificationClient::AudioNotificationClient()
	            : refCount(1)
	        {

	        }
	
	        AudioNotificationClient::~AudioNotificationClient()
	        {
	        }
	
	        ULONG STDMETHODCALLTYPE AudioNotificationClient::AddRef()
	        {
	            return InterlockedIncrement(&refCount);
	        }
	
	        ULONG STDMETHODCALLTYPE AudioNotificationClient::Release()
	        {
	            ULONG ulRef = InterlockedDecrement(&refCount);
	            if (0 == ulRef)
	            {
	                delete this;
	            }
	            return ulRef;
	        }
	
	        HRESULT STDMETHODCALLTYPE AudioNotificationClient::QueryInterface(REFIID riid, VOID **ppvInterface)
	        {
	            if (IID_IUnknown == riid)
	            {
	                AddRef();
	                *ppvInterface = (IUnknown*)this;
	            }
	            else if (__uuidof(IMMNotificationClient) == riid)
	            {
	                AddRef();
	                *ppvInterface = (IMMNotificationClient*)this;
	            }
	            else
	            {
	                *ppvInterface = NULL;
	                return E_NOINTERFACE;
	            }
	            return S_OK;
	        }
	
	        HRESULT STDMETHODCALLTYPE AudioNotificationClient::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId)
	        {
	            char  *pszFlow = "?????";
	            char  *pszRole = "?????";
	
	            switch (flow)
	            {
	            case eRender:
	                pszFlow = "eRender";
	                break;
	            case eCapture:
	                pszFlow = "eCapture";
	                break;
	            }
	
	            AudioDeviceRole deviceRole;
	            switch (role)
	            {
	            case eConsole:
	                pszRole = "eConsole";
	                deviceRole = AUDIO_DEVICE_CONSOLE;
	                break;
	            case eMultimedia:
	                pszRole = "eMultimedia";
	                deviceRole = AUDIO_DEVICE_MULTIMEDIA;
	                break;
	            case eCommunications:
	                pszRole = "eCommunications";
	                deviceRole = AUDIO_DEVICE_COMMUNICATIONS;
	                break;
	            }
	
	            printf("  -->New default device: flow = %s, role = %s\n",
	                pszFlow, pszRole);
	
	            DeviceInfo* info = GetDeviceInfo(pwstrDeviceId);
	            if (info && info->valid)
	            {
	                QString deviceName = QString::fromStdWString(info->deviceName);
	                QString deviceId = QString::fromStdWString(pwstrDeviceId);
	
	                emit (defaultDeviceChanged(deviceName, deviceId, info->type, deviceRole));
	                delete info;
	                return S_OK;
	            }
	            return S_FALSE;
	        }
	
	        HRESULT STDMETHODCALLTYPE AudioNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
	        {
	            DeviceInfo* info = GetDeviceInfo(pwstrDeviceId);
	            if (info && info->valid)
	            {
	                QString deviceName = QString::fromStdWString(info->deviceName);
	                QString deviceId = QString::fromStdWString(pwstrDeviceId);
	
	                emit (deviceAdded(deviceName, deviceId, info->type));
	                delete info;
	                return S_OK;
	            }
	
	            return S_FALSE;
	        };
	
	        HRESULT STDMETHODCALLTYPE AudioNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
	        {
	            DeviceInfo* info = GetDeviceInfo(pwstrDeviceId);
	            if (info && info->valid)
	            {
	                QString deviceName = QString::fromStdWString(info->deviceName);
	                QString deviceId = QString::fromStdWString(pwstrDeviceId);
	
	                emit (deviceRemoved(deviceName, deviceId, info->type));
	                delete info;
	                return S_OK;
	            }
	
	            return S_FALSE;
	        }
	
	        HRESULT STDMETHODCALLTYPE AudioNotificationClient::OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState)
	        {
	            HRESULT result = S_FALSE;
	            
	            switch (dwNewState)
	            {
	            case DEVICE_STATE_ACTIVE:
	                {
	                    DeviceInfo* info = GetDeviceInfo(pwstrDeviceId);
	                    if (info && info->valid)
	                    {
	                        QString deviceName = QString::fromStdWString(info->deviceName);
	                        QString deviceId = QString::fromStdWString(pwstrDeviceId);
	
	                        emit (deviceAdded(deviceName, deviceId, info->type));
	                        delete info;
	                        result = S_OK;
	                    }
	                }
	                break;
	            case DEVICE_STATE_DISABLED:
	            case DEVICE_STATE_NOTPRESENT:
	            case DEVICE_STATE_UNPLUGGED:
	            default:
	                {
	                    DeviceInfo* info = GetDeviceInfo(pwstrDeviceId);
	                    if (info && info->valid)
	                    {
	                        QString deviceName = QString::fromStdWString(info->deviceName);
	                        QString deviceId = QString::fromStdWString(pwstrDeviceId);
	
	                        emit (deviceRemoved(deviceName, deviceId, info->type));
	                        delete info;
	                        result = S_OK;
	                    }
	                }
	                break;
	            }
	
	            return result;
	        }
	
	        HRESULT STDMETHODCALLTYPE AudioNotificationClient::OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key)
	        {
	            printf("  -->Changed device property "
	                "{%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x}#%d\n",
	                key.fmtid.Data1, key.fmtid.Data2, key.fmtid.Data3,
	                key.fmtid.Data4[0], key.fmtid.Data4[1],
	                key.fmtid.Data4[2], key.fmtid.Data4[3],
	                key.fmtid.Data4[4], key.fmtid.Data4[5],
	                key.fmtid.Data4[6], key.fmtid.Data4[7],
	                key.pid);
	            return S_OK;
	        }
	
	        DeviceInfo* AudioNotificationClient::GetDeviceInfo(LPCWSTR deviceId)
	        {
	            HRESULT hr = S_OK;
	            IMMDevice *pDevice = NULL;
	            IMMEndpoint *pEndpoint = NULL;
	            IPropertyStore *pProps = NULL;
	            PROPVARIANT varString;
	            EDataFlow flow;
	
	            HRESULT hrCoInit = CoInitialize(NULL);
	            PropVariantInit(&varString);
	
	            IMMDeviceEnumerator *deviceEnumerator = NULL;
	            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(deviceEnumerator);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return new DeviceInfo();
	            }
	
	            if (deviceEnumerator == NULL)
	            {
	                // Get enumerator for audio endpoint devices.
	                hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
	                    NULL, CLSCTX_INPROC_SERVER,
	                    __uuidof(IMMDeviceEnumerator),
	                    (void**)&deviceEnumerator);
	            }
	            if (hr == S_OK)
	            {
	                hr = deviceEnumerator->GetDevice(deviceId, &pDevice);
	            }
	            if (hr == S_OK)
	            {
	                hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
	            }
	            if (hr == S_OK)
	            {
	                // Get the endpoint device's friendly-name property.
	                hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
	            }
	            if (hr == S_OK)
	            {
	                hr = pDevice->QueryInterface(__uuidof(IMMEndpoint), (void**)&pEndpoint);
	            }
	            if (hr == S_OK)
	            {
	                hr = pEndpoint->GetDataFlow(&flow);
	            }
	
	            SAFE_RELEASE(pProps)
	            SAFE_RELEASE(pDevice)
	            CoUninitialize();
	            
	            DeviceInfo* info = NULL;
	            if (hr == S_OK)
	            {
	                info = new DeviceInfo(varString.pwszVal, (flow == eRender) ? AUDIO_DEVICE_OUTPUT : AUDIO_DEVICE_INPUT);
	            }
	            else
	            {
	                ORIGIN_LOG_WARNING << "Error getting device info: id = " << deviceId << ", hr = " << hr;
	            }
	
	            PropVariantClear(&varString);
	
	            return info;
	        }

	        bool IsAudioDevicePresent(AudioDeviceType device)
	        {
	            bool devicePresent = true;
	            HRESULT hrCoInit = CoInitialize(NULL);
	            IMMDeviceEnumerator *deviceEnumerator = NULL;
	            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	            IMMDevice* defaultDevice = NULL;
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(deviceEnumerator);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return devicePresent;
	            }
	
	            ERole role = eConsole;
	            EDataFlow flow = (device == AUDIO_DEVICE_OUTPUT) ? eRender : eCapture;
	
	            hr = deviceEnumerator->GetDefaultAudioEndpoint(flow, role, &defaultDevice);
	            SAFE_RELEASE(deviceEnumerator);
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(defaultDevice);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	
	                // device was not found !!!
	                if (hr == E_NOTFOUND)
	                    devicePresent = false;
	            }
	
	            SAFE_RELEASE(defaultDevice);
	
	            if (SUCCEEDED(hrCoInit))
	                CoUninitialize();
	
	            return devicePresent;
	        }
	
	        QMap<QString, QString> GetAudioDevices(AudioDeviceType device)
	        {
	            QMap<QString, QString> deviceMap;
	            HRESULT hrCoInit = CoInitialize(NULL);
	            IMMDeviceEnumerator *deviceEnumerator = NULL;
	            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	            IMMDeviceCollection* deviceCollection = NULL;
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(deviceEnumerator);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return deviceMap;
	            }
	
	            EDataFlow flow = (device == AUDIO_DEVICE_OUTPUT) ? eRender : eCapture;
	
	            hr = deviceEnumerator->EnumAudioEndpoints(flow, DEVICE_STATE_ACTIVE, &deviceCollection);
	            SAFE_RELEASE(deviceEnumerator);
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(deviceCollection);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return deviceMap;
	            }
	
	            UINT numDevices = 0;
	            hr = deviceCollection->GetCount(&numDevices);
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(deviceCollection);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return deviceMap;
	            }
	
	            for (UINT i = 0; i < numDevices; ++i)
	            {
	                IMMDevice* currentDevice = NULL;
	                hr = deviceCollection->Item(i, &currentDevice);
	                if (FAILED(hr))
	                {
	                    SAFE_RELEASE(deviceCollection);
	                    SAFE_RELEASE(currentDevice);
	                    if (SUCCEEDED(hrCoInit))
	                        CoUninitialize();
	                    return deviceMap;
	                }
	
	                IPropertyStore* propertyStore;
	                hr = currentDevice->OpenPropertyStore(STGM_READ, &propertyStore);
	                if (FAILED(hr))
	                {
	                    SAFE_RELEASE(deviceCollection);
	                    SAFE_RELEASE(currentDevice);
	                    SAFE_RELEASE(propertyStore);
	                    if (SUCCEEDED(hrCoInit))
	                        CoUninitialize();
	                    return deviceMap;
	                }
	
	                PROPVARIANT deviceName;
	                PropVariantInit(&deviceName);
	
	                hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &deviceName);
	                if (FAILED(hr))
	                {
	                    SAFE_RELEASE(deviceCollection);
	                    SAFE_RELEASE(currentDevice);
	                    SAFE_RELEASE(propertyStore);
	                    if (SUCCEEDED(hrCoInit))
	                        CoUninitialize();
	                    return deviceMap;
	                }
	
	                LPWSTR deviceId;
	                hr = currentDevice->GetId(&deviceId);
	                if (FAILED(hr))
	                {
	                    SAFE_RELEASE(deviceCollection);
	                    SAFE_RELEASE(currentDevice);
	                    SAFE_RELEASE(propertyStore);
	                    if (SUCCEEDED(hrCoInit))
	                        CoUninitialize();
	                    return deviceMap;
	                }
	
	                deviceMap[QString::fromStdWString(deviceName.pwszVal)] = QString::fromStdWString(deviceId);
	
	                SAFE_RELEASE(currentDevice);
	            }
	
	            SAFE_RELEASE(deviceCollection);
	
	            if (SUCCEEDED(hrCoInit))
	                CoUninitialize();
	
	            return deviceMap;
	        }

            // WinXP
            QString GetDefaultAudioDeviceGuid(AudioDeviceType deviceType)
            {
                QString deviceGuid;

                // WinXP
                GUID guid;
                LPCGUID pGuidSrc = (deviceType == AUDIO_DEVICE_OUTPUT) ? &DSDEVID_DefaultVoicePlayback : &DSDEVID_DefaultVoiceCapture;
                HRESULT hrGDI = GetDeviceID(pGuidSrc, &guid);
                if( DS_OK == hrGDI )
                {
                    deviceGuid = QString::fromStdString(sonar::GUIDToString(&guid));
                }

                return deviceGuid;
            }

	        QString GetDefaultAudioDevice(AudioDeviceType deviceType)
	        {
	            QString device;
	
	            // Init
	            HRESULT hrCoInit = CoInitialize(NULL);
	            IMMDeviceEnumerator *deviceEnumerator = NULL;
	            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	            IMMDevice* currentDevice = NULL;
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(deviceEnumerator);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return device;
	            }
	
	            EDataFlow flow = (deviceType == AUDIO_DEVICE_OUTPUT) ? eRender : eCapture;
	
	            // Get default device
	            hr = deviceEnumerator->GetDefaultAudioEndpoint(flow, eCommunications, &currentDevice);
	            SAFE_RELEASE(deviceEnumerator);
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(currentDevice);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return device;
	            }
	
	            IPropertyStore* propertyStore;
	            hr = currentDevice->OpenPropertyStore(STGM_READ, &propertyStore);
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(currentDevice);
	                SAFE_RELEASE(propertyStore);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return device;
	            }
	
	            // Get the device name
	            PROPVARIANT deviceName;
	            PropVariantInit(&deviceName);
	
	            hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &deviceName);
	            if (FAILED(hr))
	            {
	                SAFE_RELEASE(currentDevice);
	                SAFE_RELEASE(propertyStore);
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	                return device;
	            }
	
	            device = QString::fromStdWString(deviceName.pwszVal);
	
	            SAFE_RELEASE(currentDevice);
	
	            // Clean up
	            if (SUCCEEDED(hrCoInit))
	                CoUninitialize();
	
	            return device;
	        }
	
	        AudioNotificationClient* RegisterDeviceCallback()
	        {
                if (audioNotificationClient == NULL)
                {
                    HRESULT hrCoInit = CoInitialize(NULL);
                    IMMDeviceEnumerator *deviceEnumerator = NULL;
                    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
                    if (FAILED(hr))
                    {
                        SAFE_RELEASE(deviceEnumerator);
                        if (SUCCEEDED(hrCoInit))
                            CoUninitialize();
                        return NULL;
                    }

                    audioNotificationClient = new AudioNotificationClient();
                    hr = deviceEnumerator->RegisterEndpointNotificationCallback(audioNotificationClient);

                    if (SUCCEEDED(hrCoInit))
                        CoUninitialize();
                }

	            return audioNotificationClient;
	        }
	
	        void UnregisterDeviceCallback()
	        {
	            if (audioNotificationClient)
	            {
	                HRESULT hrCoInit = CoInitialize(NULL);
	                IMMDeviceEnumerator *deviceEnumerator = NULL;
	                HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
	                if (FAILED(hr))
	                {
	                    SAFE_RELEASE(deviceEnumerator);
	                    if (SUCCEEDED(hrCoInit))
	                        CoUninitialize();
	                    return;
	                }
	
	                hr = deviceEnumerator->UnregisterEndpointNotificationCallback(audioNotificationClient);
	                delete audioNotificationClient;
	                audioNotificationClient = NULL;
	
	                if (SUCCEEDED(hrCoInit))
	                    CoUninitialize();
	            }
	        }

            // XP
            // This GUID is for all USB serial host PnP drivers, but you can replace it 
            // with any valid device class guid.
            GUID MultimediaGUID = { 0x4d36e96c, 0xe325, 0x11ce, 
                0xbf,0xc1,0x08,0x00,0x2b,0xe1,0x03,0x18 };

            bool RegisterDeviceNotification(HWND hWnd, HDEVNOTIFY *hDeviceNotify)
            {
                DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

                ZeroMemory( &NotificationFilter, sizeof(NotificationFilter) );
                NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
                NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
                CopyMemory(&NotificationFilter.dbcc_classguid, &MultimediaGUID, sizeof(GUID));

                *hDeviceNotify = RegisterDeviceNotification( 
                    hWnd,                       // events recipient
                    &NotificationFilter,        // type of device
                    DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES // type of recipient handle
                    );

                if ( NULL == *hDeviceNotify ) 
                {
                    return FALSE;
                }

                return TRUE;
            }

            bool UnregisterDeviceNotification(HDEVNOTIFY handle)
            {
                ::UnregisterDeviceNotification(handle);
                return true;
            }

            bool GetDeviceName( QString& deviceName, PDEV_BROADCAST_DEVICEINTERFACE pDevInf, WPARAM wParam )
            {
                // dbcc_name:
                // \\?\USB#Vid_04e8&Pid_503b#0002F9A9828E0F06#{a5dcbf10-6530-11d2-901f-00c04fb951ed}
                // convert to
                // USB\Vid_04e8&Pid_503b\0002F9A9828E0F06
                deviceName.clear();
                QString dbccName = QString::fromWCharArray(pDevInf->dbcc_name);

                ORIGIN_ASSERT(dbccName.length() > 4);
                if( dbccName.length() <= 4 )
                    return false;

                QString szDevId = dbccName.mid(4);
                int idx = szDevId.lastIndexOf("#");
                szDevId = szDevId.left(idx);
                szDevId = szDevId.replace("#", "\\");
                szDevId = szDevId.toUpper();

                idx = szDevId.indexOf("\\");
                QString szClass = szDevId.left(idx);

                // if we are adding device, we only need present devices
                // otherwise, we need all devices
                DWORD dwFlag = DBT_DEVICEARRIVAL != wParam
                    ? DIGCF_ALLCLASSES : (DIGCF_ALLCLASSES | DIGCF_PRESENT);
                HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, szClass.toStdWString().c_str(), NULL, dwFlag);
                if( INVALID_HANDLE_VALUE == hDevInfo )
                {
                    return false;
                }

                bool retVal = true;

                SP_DEVINFO_DATA* pspDevInfoData =
                    (SP_DEVINFO_DATA*)HeapAlloc(GetProcessHeap(), 0, sizeof(SP_DEVINFO_DATA));
                pspDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
                for(int i=0; SetupDiEnumDeviceInfo(hDevInfo,i,pspDevInfoData); i++)
                {
                    DWORD DataT ;
                    DWORD nSize=0 ;
                    TCHAR buf[MAX_PATH];

                    if ( !SetupDiGetDeviceInstanceId(hDevInfo, pspDevInfoData, buf, sizeof(buf), &nSize) )
                    {
                        retVal = false;
                        break;
                    }

                    QString bufString = QString::fromWCharArray(buf);
                    if ( szDevId == bufString )
                    {
                        //
                        // device found
                        //

                        // get name
                        if ( SetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData,
                            SPDRP_FRIENDLYNAME, &DataT, (PBYTE)buf, sizeof(buf), &nSize) ) {
                                deviceName = QString::fromWCharArray(buf);
                        } else if ( SetupDiGetDeviceRegistryProperty(hDevInfo, pspDevInfoData,
                            SPDRP_DEVICEDESC, &DataT, (PBYTE)buf, sizeof(buf), &nSize) ) {
                                deviceName = QString::fromWCharArray(buf);
                        } else {
                            DWORD err = GetLastError();
                            retVal = false;
                        }

                        break;
                    }
                }

                if ( pspDevInfoData ) HeapFree(GetProcessHeap(), 0, pspDevInfoData);
                SetupDiDestroyDeviceInfoList(hDevInfo);

                return retVal;
            }
        }
    }
}

#endif //ENABLE_VOICE_CHAT