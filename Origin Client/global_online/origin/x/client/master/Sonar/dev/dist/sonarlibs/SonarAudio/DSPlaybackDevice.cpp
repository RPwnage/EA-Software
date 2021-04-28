#include <SonarAudio/DSPlaybackDevice.h>
#include <SonarAudio/DSPlaybackBuffer.h>
#include <SonarAudio/dscommon.h>
#include <SonarCommon/Common.h>


namespace sonar
{

DSPlaybackDevice::DSPlaybackDevice (
    AudioRuntime &runtime,
    const AudioDeviceId &_device,
    int _samplesPerSec,
    int _samplesPerFrame,
    int _bitsPerSample)
	: m_pDS(NULL)
    , m_samplesPerSec(_samplesPerSec)
	, m_bitsPerSample(_bitsPerSample)
{
	GUID guid;

	if (_device.id == "DEFAULT")
	{
		runtime.lookupRealGuid(&DSDEVID_DefaultVoicePlayback, &guid);
	}

	else
	{
		if (!StringToGuid (_device.id, &guid))
		{
            if( !errorSent("SPlaybackDevice", "create_guid", internal::lastPlaybackDeviceError) )
            {
                common::Log("sonar::DSPlaybackDevice::DSPlaybackDevice convert guid failed, id=", _device.id.c_str());
                common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SPlaybackDevice", "create_guid", internal::lastPlaybackDeviceError);
            }
			return;
		}
	}

    if (common::ENABLE_CLIENT_DSOUND_TRACING)
        common::Log("sonar::DSPlaybackDevice::DSPlaybackDevice creating playback device");

	if (FAILED (internal::lastPlaybackDeviceError = DirectSoundCreate8 (&guid, &m_pDS, NULL)))
	{
        if( !errorSent("SPlaybackDevice", "create", internal::lastPlaybackDeviceError) )
        {
            common::Log("sonar::DSPlaybackDevice::DSPlaybackDevice DirectSoundCreate8 err=%d", internal::lastPlaybackDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SPlaybackDevice", "create", internal::lastPlaybackDeviceError);
        }
		return;
	}

	// FIXME: Set cooperative level here, we need HWND!
	if (FAILED (internal::lastPlaybackDeviceError = m_pDS->SetCooperativeLevel (GetDesktopWindow (), DSSCL_PRIORITY)))
	{
        if( !errorSent("SPlaybackDevice", "create_coop", internal::lastPlaybackDeviceError) )
        {
            common::Log("sonar::DSPlaybackDevice::DSPlaybackDevice SetCooperativeLevel err=%d", internal::lastPlaybackDeviceError);
            common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SPlaybackDevice", "create_coop", internal::lastPlaybackDeviceError);
        }
		return;
	}
}

DSPlaybackDevice::~DSPlaybackDevice (void)
{
	if (m_pDS)
    {
        m_pDS->Release ();

        if (common::ENABLE_CLIENT_DSOUND_TRACING)
            common::Log("sonar::DSPlaybackDevice::~DSPlaybackDevice released playback device");
    }
}

int DSPlaybackDevice::getSamplesPerSec (void)
{
	return m_samplesPerSec;
}

int DSPlaybackDevice::getBitsPerSample (void)
{
	return m_bitsPerSample;
}

LPDIRECTSOUND8 DSPlaybackDevice::getDS8 (void)
{
	return m_pDS;
}

bool DSPlaybackDevice::isCreated(void)
{
	return (m_pDS != NULL);
}

DSPlaybackBuffer *DSPlaybackDevice::createBuffer (int _numSamples, int _samplesPerFrame)
{
	DSPlaybackBuffer *pRet = new DSPlaybackBuffer (*this, _numSamples, _samplesPerFrame);
	return pRet;
}

}
