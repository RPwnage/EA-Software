#include <SonarCommon/Common.h>
#include <SonarAudio/DSAudioRuntime.h>
#include <SonarAudio/dscommon.h>

#include <windows.h>
#include <Mmdeviceapi.h>
#include <Audiopolicy.h>
#include <dsound.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "dsound.lib")

namespace sonar
{
GUID *DSAudioRuntime::lookupRealGuid (const GUID *_pDeviceGUID, GUID *_buffer)
{
    memcpy (_buffer, _pDeviceGUID, sizeof (GUID));
    
    const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
    const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

    IMMDevice *pDevice = NULL;
    IPropertyStore *pProps = NULL;
    IMMDeviceEnumerator* deviceEnumerator = NULL;

    HRESULT coInitResult = CoInitialize(NULL);
    if (coInitResult < S_OK)
        goto CLEANUP;

    HRESULT hr = CoCreateInstance (
        CLSID_MMDeviceEnumerator,
        NULL,
        CLSCTX_ALL,
        IID_IMMDeviceEnumerator,
        (void **) &deviceEnumerator);

    if (hr != S_OK)
    {
        // hr == REGDB_E_CLASSNOTREG (0x80040154) is reported on XP where the MMDevice API is not supported
        // don't bother sending to telemetry
        if( (hr != REGDB_E_CLASSNOTREG) && !errorSent("SAudioRuntime", "create", hr) )
        {
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SAudioRuntime", "create", hr);
        }

        deviceEnumerator = NULL;
        goto CLEANUP;
    }

    if (memcmp (_pDeviceGUID, &DSDEVID_DefaultVoiceCapture, sizeof (GUID)) == 0)
    {
        hr = deviceEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &pDevice);

        if (FAILED(hr))
        {
            // if device not found or error has already been sent, don't bother sending telemetry
            if( (hr != E_NOTFOUND) && !errorSent("SAudioRuntime", "lookup_ep1", hr) )
            {
                common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SAudioRuntime", "lookup_ep1", hr);
            }
            goto CLEANUP;
        }
    }
    else
    if (memcmp (_pDeviceGUID, &DSDEVID_DefaultVoicePlayback, sizeof (GUID)) == 0)
    {
        hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eCommunications, &pDevice);

        if (FAILED(hr))
        {
            // if device not found or error has already been sent, don't bother sending telemetry
            if( (hr != E_NOTFOUND) && !errorSent("SAudioRuntime", "lookup_ep2", hr) )
            {
                common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SAudioRuntime", "lookup_ep2", hr);
            }
            goto CLEANUP;
        }
    }
    else
    {
        memcpy (_buffer, _pDeviceGUID, sizeof (GUID));
        goto CLEANUP;
    }

    PROPVARIANT var;

    hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);

    if (FAILED(hr))
    {
        if( !errorSent("SAudioRuntime", "lookup_prop1", hr) )
        {
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SAudioRuntime", "lookup_prop1", hr);
        }
        goto CLEANUP;
    }

    PropVariantInit(&var);
    hr = pProps->GetValue(PKEY_AudioEndpoint_GUID, &var);

    if (FAILED(hr))
    {
        PropVariantClear(&var);
        if( !errorSent("SAudioRuntime", "lookup_prop2", hr) )
        {
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SAudioRuntime", "lookup_prop2", hr);
        }
        goto CLEANUP;
    }

    GUID deviceGUID;
    hr = CLSIDFromString(var.pwszVal, &deviceGUID);

    if (FAILED(hr))
    {
        PropVariantClear(&var);
        if( !errorSent("SAudioRuntime", "lookup_prop3", hr) )
        {
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SAudioRuntime", "lookup_prop3", hr);
        }
        goto CLEANUP;
    }
    PropVariantClear(&var);

    memcpy (_buffer, &deviceGUID, sizeof (GUID));


CLEANUP:
    if (pProps) pProps->Release();
    if (pDevice) pDevice->Release();

    if (deviceEnumerator)
    {
        deviceEnumerator->Release();
        deviceEnumerator = NULL;
    }

    if (coInitResult >= S_OK)
        CoUninitialize();

    return _buffer;
}
}
