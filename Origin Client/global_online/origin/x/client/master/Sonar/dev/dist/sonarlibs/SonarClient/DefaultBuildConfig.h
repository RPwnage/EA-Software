#pragma once

#include <SonarCommon/Common.h>

/*
===============================================================================
PROVIDERS
This section configure the additional time, transport, network and 
debugging providers.

Bottom line:
After this section the following classes must be defined within the Sonar 
namespace: 
sonar::NetworkProvider (based on INetworkProvider)
sonar::Transport (based on ITransport)
sonar::TimeProvider (based on ITimeProvider)
sonar::DebugLogger (based on IDebugLogger)

=============================================================================*/

#ifdef _WIN32
#include <SonarClient/win32/providers.h>

namespace sonar
{
	typedef Udp4NetworkProvider NetworkProvider;
	typedef Udp4Transport Transport;
	typedef DefaultTimeProvider TimeProvider;
	typedef StderrLogger DebugLogger;
}

#else
#error "Sonar build: No portable transport, network and time provider alternatives are available"
#endif

/*
===============================================================================
AUDIO 
This section must include and setup the Audio framework needed by the client
by default the Win32 specific DirectSound8 ref driver is included. 

Bottom line:
After this section the following classes must be defined in the sonar namespace
and be interface compatible with the DirectSound8 ref driver.

sonar::AudioPlaybackBuffer
sonar::AudioCaptureEnum
sonar::AudioCaptureDevice
sonar::AudioPlaybackEnum
sonar::AudioPlaybackDevice
sonar::AudioRuntime
=============================================================================*/
#include <SonarAudio/Audio.h>

namespace sonar
{
typedef DSPlaybackBuffer AudioPlaybackBuffer;
typedef DSCaptureDeviceEnum AudioCaptureEnum;
typedef DSCaptureDevice AudioCaptureDevice;
typedef DSPlaybackDeviceEnum AudioPlaybackEnum;
typedef DSPlaybackDevice AudioPlaybackDevice;
typedef DSAudioRuntime AudioRuntime;
}

/*
===============================================================================
INPUT
This section configure the input driver for the Sonar client. 
The input driver is optional.

Bottom line:
After this section either SONAR_CLIENT_HAS_INPUT MUST NOT be defined or a
an input driver compatible with sonar::Input has to be included and defined
in the sonar namespace:

sonar::Input 
sonar::Stroke
=============================================================================*/
#define SONAR_CLIENT_HAS_INPUT

#ifdef SONAR_CLIENT_HAS_INPUT
#ifdef WIN32
#include <SonarInput/Input.h>
#else
#error "Sonar build: No portable input alternative is available"
#endif
#endif

