#include <SonarClient/Peer.h>

namespace sonar
{
Peer::Peer (int chid, CString &userId, CString &userDesc, Codec *codec) 
	: m_codec(codec)
{
	m_pi.chid = chid;
	m_pi.isMuted = false;
	m_pi.isTalking = false;
	m_pi.userId = userId;
	m_pi.userDesc = userDesc;
}

Peer::~Peer (void)
{
	delete m_codec;
}

void Peer::setMuted(bool state)
{
	m_pi.isMuted = state;
}

bool Peer::getMuted()
{
    return m_pi.isMuted;
}

void Peer::setTalking(bool state)
{
	m_pi.isTalking = state;
}

bool Peer::getTalking (void)
{
	return m_pi.isTalking;
}


Codec &Peer::getCodec()
{
	return *m_codec;
}

const PeerInfo &Peer::getPeerInfo()
{
	return m_pi;
}

}