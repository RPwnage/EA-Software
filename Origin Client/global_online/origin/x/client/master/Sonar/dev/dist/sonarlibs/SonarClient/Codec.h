#pragma once

#ifdef SONAR_USE_OPUS
#include <SonarClient/CodecOpus.h>
#else
#include <SonarClient/CodecSpeex.h>
#endif

namespace sonar
{
#ifdef SONAR_USE_OPUS
	typedef CodecOpus Codec;
#else
	typedef CodecSpeex Codec;
#endif
}