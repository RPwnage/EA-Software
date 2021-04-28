#pragma once

#include <SonarClient/Codec.h>
#include <SonarClient/AudioQueue.h>
#include <SonarClient/ClientTypes.h>
#include <SonarCommon/Common.h>

namespace sonar
{
	class Peer
	{
	public:
		Peer (int chid, CString &userId, CString &userDesc, Codec *codec);
		~Peer (void);

		Codec &getCodec();
		void setMuted(bool state);
        bool getMuted (void);
		void setTalking (bool state);
		bool getTalking (void);
		const PeerInfo &getPeerInfo();

	private:
		Codec *m_codec;

		PeerInfo m_pi;

	};



}