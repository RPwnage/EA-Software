#pragma once

#ifndef SONAR_CLIENT_BUILD_CONFIG
#define SONAR_CLIENT_BUILD_CONFIG <SonarClient/DefaultBuildConfig.h>
#endif
#include SONAR_CLIENT_BUILD_CONFIG

#include <SonarCommon/Common.h>

namespace sonar
{
class CodecOpus
{
public:
	CodecOpus(int bitsPerSecond, int samplesPerSecond, int samplesPerFrame);
	~CodecOpus(void);

	typedef SonarVector<UINT8> Payload;
	void encodeAudio(const AudioFrame &frame, Payload &outPayload);
	void decodeAudio(const Payload &payload, AudioFrame &outFrame);
	void destroyEncoder(void);
	void destroyDecoder(void);

private:

	struct Private;
	struct Private *m_prv;
	bool m_encoderOnce;
	bool m_decoderOnce;

	struct
	{
		int quality;
		int complexity;
		int sampleRate;
		int enh;
		int vbr;
		int bitRate;
		int dtx;
		int vad;
		float vbrQuality;
		int vbrBitRate;
		int abrBitRate;
		int samplesPerFrame;
	} m_config;

};

}