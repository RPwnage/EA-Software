#include <SonarClient/Client.h>
#include <SonarClient/CodecSpeex.h>
#include <assert.h>
#include <speex/speex.h>

namespace sonar
{

struct CodecSpeex::Private
{
	SpeexBits m_encBits;
	SpeexBits m_decBits;
};

CodecSpeex::CodecSpeex(const Options& opts, int bitsPerSecond, int samplesPerSecond, int samplesPerFrame)
{
	m_prv = new Private;

	m_encoderOnce = false;
	m_decoderOnce = false;

	m_config.quality = opts.speexSettingQuality();
	m_config.complexity = opts.speexSettingComplexity();
	m_config.sampleRate = samplesPerSecond;
	m_config.enh = 1;
	m_config.vbr = opts.speexSettingVbrEnable();
	m_config.vbrQuality = opts.speexSettingVbrQuality();
	m_config.vbrBitRate = bitsPerSecond + (bitsPerSecond / 2);
	m_config.bitRate = bitsPerSecond;
	m_config.dtx = 0;
	m_config.vad = 0;
	m_config.abrBitRate = bitsPerSecond;
	m_config.samplesPerFrame = samplesPerFrame;
}

CodecSpeex::~CodecSpeex(void)
{
	destroyEncoder();
	destroyDecoder();

	delete m_prv;
}

void CodecSpeex::destroyEncoder(void)
{
	if (m_encoderOnce)
	{
		speex_encoder_destroy (m_encState);
		speex_bits_destroy (&m_prv->m_encBits);
		m_encoderOnce = false;
	}
}

void CodecSpeex::destroyDecoder(void)
{
	if (m_decoderOnce)
	{
		speex_decoder_destroy (m_decState);
		speex_bits_destroy (&m_prv->m_decBits);
		m_decoderOnce = false;
	}
}

void CodecSpeex::encodeAudio(const AudioFrame &frame, Payload &outPayload)
{
	if (!m_encoderOnce)
	{
		m_encoderOnce = true;

		speex_bits_init (&m_prv->m_encBits);
		m_encState = speex_encoder_init (&speex_wb_mode);
				
		speex_encoder_ctl (m_encState, SPEEX_SET_QUALITY, &m_config.quality);
		speex_encoder_ctl (m_encState, SPEEX_SET_COMPLEXITY, &m_config.complexity);
		speex_encoder_ctl (m_encState, SPEEX_SET_BITRATE, &m_config.bitRate);
		speex_encoder_ctl (m_encState, SPEEX_SET_SAMPLING_RATE, &m_config.sampleRate);
		speex_encoder_ctl (m_encState, SPEEX_SET_VBR_QUALITY, &m_config.vbrQuality);
		speex_encoder_ctl (m_encState, SPEEX_SET_VBR_MAX_BITRATE, &m_config.vbrBitRate);
		speex_encoder_ctl (m_encState, SPEEX_SET_VBR, &m_config.vbr);
		speex_encoder_ctl (m_encState, SPEEX_SET_VAD, &m_config.vad);
		speex_encoder_ctl (m_encState, SPEEX_SET_DTX, &m_config.dtx);
		speex_encoder_ctl (m_encState, SPEEX_SET_ABR, &m_config.abrBitRate);

		int frameSize;
		speex_encoder_ctl (m_encState, SPEEX_GET_FRAME_SIZE, &frameSize);

		assert (frameSize == m_config.samplesPerFrame);
	}
	

	speex_bits_reset(&m_prv->m_encBits); 
	speex_encode (m_encState, (float *) &frame[0], &m_prv->m_encBits);
	
	outPayload.resize(256);
	int cbSize = speex_bits_write(&m_prv->m_encBits, (char *) &outPayload[0], outPayload.capacity());
	outPayload.resize((size_t) cbSize);
}
	
void CodecSpeex::decodeAudio(const Payload &payload, AudioFrame &outFrame)
{
	if (!m_decoderOnce)
	{
		m_decoderOnce = true;

		speex_bits_init (&m_prv->m_decBits);
		m_decState = speex_decoder_init (&speex_wb_mode);

		speex_decoder_ctl (m_decState, SPEEX_SET_SAMPLING_RATE, &m_config.sampleRate);
		speex_decoder_ctl (m_decState, SPEEX_SET_ENH, &m_config.enh);
		//speex_decoder_ctl (m_decState, SPEEX_SET_VAD, &m_config.vad);

    int frameSize;
		speex_encoder_ctl (m_decState, SPEEX_GET_FRAME_SIZE, &frameSize);

		assert (frameSize == m_config.samplesPerFrame);
  }

	outFrame.resize(m_config.samplesPerFrame);

	if (payload.empty())
	{
		speex_decode (m_decState, NULL, &outFrame[0]);
	}
	else 
	{
		speex_bits_read_from (&m_prv->m_decBits, (char *) &payload[0], payload.size()); 
		speex_decode (m_decState, &m_prv->m_decBits, &outFrame[0]);
	}
}



}