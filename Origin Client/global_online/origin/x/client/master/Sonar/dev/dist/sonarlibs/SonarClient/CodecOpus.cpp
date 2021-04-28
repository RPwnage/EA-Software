#include <SonarClient/Client.h>
#include <SonarClient/CodecOpus.h>
#include <assert.h>
#include <speex/speex.h>

#include "opus.h"

namespace sonar
{

typedef std::vector<opus_int16> Int16Frame;

struct CodecOpus::Private
{
	OpusEncoder *enc;
	OpusDecoder *dec;
};

CodecOpus::CodecOpus(int bitsPerSecond, int samplesPerSecond, int samplesPerFrame)
{
	m_prv = new Private;

	m_encoderOnce = false;
	m_decoderOnce = false;

	m_config.quality = 10;
	m_config.complexity = 10;
	m_config.sampleRate = samplesPerSecond;
	m_config.bitRate = bitsPerSecond;
	m_config.vbrQuality = 10.0f;
	m_config.samplesPerFrame = samplesPerFrame;
}

CodecOpus::~CodecOpus(void)
{
	destroyEncoder();
	destroyDecoder();

	delete m_prv;
}

void CodecOpus::destroyEncoder(void)
{
	if (m_encoderOnce)
	{
		opus_encoder_destroy(m_prv->enc);
		m_encoderOnce = false;
	}
}

void CodecOpus::destroyDecoder(void)
{
	if (m_decoderOnce)
	{
		opus_decoder_destroy(m_prv->dec);
		m_decoderOnce = false;
	}
}

void CodecOpus::encodeAudio(const AudioFrame &frame, Payload &outPayload)
{
	if (!m_encoderOnce)
	{
		int err;
		m_prv->enc = opus_encoder_create(m_config.sampleRate, 1, OPUS_APPLICATION_VOIP, &err);

		opus_encoder_ctl(m_prv->enc, OPUS_SET_COMPLEXITY(m_config.complexity));
		opus_encoder_ctl(m_prv->enc, OPUS_SET_VBR(1));
		opus_encoder_ctl(m_prv->enc, OPUS_SET_VBR_CONSTRAINT(1));
		opus_encoder_ctl(m_prv->enc, OPUS_SET_DTX(0));
		opus_encoder_ctl(m_prv->enc, OPUS_SET_BITRATE(m_config.bitRate));

		m_encoderOnce = true;
	}
	
	outPayload.resize(256);

	Int16Frame intFrame;
	size_t length = frame.size();
	intFrame.resize(length);

	for (size_t index = 0; index < length; index ++)
		intFrame[index] = (short) frame[index];
	
	size_t cbSize = opus_encode(m_prv->enc, &intFrame[0], intFrame.size(), &outPayload[0], 256);

	if (cbSize < 1)
		cbSize = 0;

	outPayload.resize((size_t) cbSize);
}
	
void CodecOpus::decodeAudio(const Payload &payload, AudioFrame &outFrame)
{
	if (!m_decoderOnce)
	{
		int err;
		m_prv->dec = opus_decoder_create(m_config.sampleRate, 1, &err);

		m_decoderOnce = true;
  }

	outFrame.resize(m_config.samplesPerFrame);
	size_t samplesDecoded;

	Int16Frame intFrame;
	size_t length = m_config.samplesPerFrame;
	intFrame.resize(length);

	if (payload.empty())
	{
		samplesDecoded = (size_t) opus_decode(m_prv->dec, NULL, 0, &intFrame[0], intFrame.size(), 1);
	}
	else 
	{
		samplesDecoded = (size_t) opus_decode(m_prv->dec, &payload[0], payload.size(), &intFrame[0], intFrame.size(), 0);
	}

	for (size_t index = 0; index < length; index ++)
		outFrame[index] = intFrame[index];

	assert (samplesDecoded == m_config.samplesPerFrame);
}



}