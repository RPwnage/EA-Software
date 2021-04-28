#pragma once

#include <SonarCommon/Common.h>

namespace sonar
{

namespace protocol
{
	enum Messages
	{
		MSG_REGISTER = 0x01,
		MSG_CHALLENGE = 0x02,
		MSG_REGISTERREPLY,
		MSG_UNREGISTER,
		MSG_PEERLIST_FRAGMENT,
		MSG_PING,
		MSG_ECHO,
		MSG_AUDIO_TO_SERVER,
		MSG_AUDIO_TO_CLIENT,
		MSG_AUDIO_STOP,
		MSG_CHANNEL_JOINNOTIF,
		MSG_CHANNEL_PARTNOTIF,
        MSG_CHANNEL_INACTIVITY,
        MSG_BLOCK_LIST_FRAGMENT,
        MSG_BLOCK_LIST_COMPLETE,
        MSG_CLIENT_STATS,
        MSG_AUDIO_END,               // this is a local command used as a placeholder to terminate audio takes in the jitter buffer
        MSG_NETWORK_PING,
        MSG_NETWORK_PING_REPLY
	};
    static const unsigned long PROTOCOL_CLIENTONLYJITTERBUFFERAWARE = 0x00090000;
    static const unsigned long PROTOCOL_NETWORK_STATS = 0x00080000;
    static const unsigned long PROTOCOL_FRAME_COUNTER = 0x00070000;
	static const unsigned long PROTOCOL_VERSION_ORIGINAL = 0x00060000;
    static const unsigned long PROTOCOL_VERSION = PROTOCOL_CLIENTONLYJITTERBUFFERAWARE;
	static const int CHANNEL_MAX_CLIENTS = 64;
	static const int SERVER_MAX_CLIENTS = 65536;
	static const int REGISTER_TIMEOUT = 10000;
    static const int REGISTER_MAX_RETRIES = 0;
    static const int NETWORK_PING_INTERVAL = 100;
    static const int NETWORK_GOOD_LOSS = 1;
    static const int NETWORK_GOOD_JITTER = 2;
    static const int NETWORK_GOOD_PING = 90;
    static const int NETWORK_OK_LOSS = 2;
    static const int NETWORK_OK_JITTER = 3;
    static const int NETWORK_OK_PING = 180;
    static const int NETWORK_POOR_LOSS = 3;
    static const int NETWORK_POOR_JITTER = 4;
    static const int NETWORK_POOR_PING = 300;
    static const int PAYLOAD_MAX_SIZE = 128;
	static const int CHALLENGE_REPLY_MIN_INTERVAL = 1000;
	static const int RSA_SIGNATURE_BASE64_MAX_LENGTH = ((2084 / 8) * 2);
	static const int TOKEN_MAX_AGE = 90;
	static const int MESSAGE_BUFFER_MAX_SIZE = 1400;
	static const int RESEND_TIMEOUT_MSEC = 1000;
	static const int RESEND_COUNT = 10;
	static const int BACKEND_KEEPALIVE_INTERVAL_SEC = 30;
	static const int PACKETTYPE_MASK = 0x03;
	static const int PACKETTYPE_SHIFT = 0x00;
	static const int FRAME_TIMESLICE_MSEC = 20;
	static const int FRAMES_PER_SECOND = (1000 / FRAME_TIMESLICE_MSEC);
	static const int IN_TAKE_MAX_FRAMES = (FRAMES_PER_SECOND * 900); // 15 minutes
  static const int SAMPLES_PER_SECOND = 16000;
  static const int SAMPLES_PER_FRAME = SAMPLES_PER_SECOND / FRAMES_PER_SECOND;
  static const int MAX_AUDIO_LOSS = FRAMES_PER_SECOND / 2;

	enum PacketType
	{
		PT_NULL		= 0x00,
		PT_ACK		= 0x01, 
		PT_SEGREL	= 0x02,
		PT_SEGUNREL	= 0x03
	};


#pragma pack(push)
#pragma pack(1)

	typedef UINT32 SEQUENCE;

	struct MessageHeader 
	{
		UINT8 flags;
		UINT16 source;
        UINT8 cmd;
		SEQUENCE seq;
	} ;
	
#pragma pack(pop)

}

}
