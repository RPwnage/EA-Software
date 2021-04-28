#include "AudioQueue.h"
#include <SonarConnection/Protocol.h>

namespace sonar
{

AudioQueue::AudioQueue(void) 
	: m_state(Stopped)
	, m_loss(0)
{
}

AudioQueue::~AudioQueue(void)
{
}

void AudioQueue::enqueue(const AudioFrame &frame)
{
	if (m_state == Stopped)
	{
		m_queue.clear();
	}

	if (m_queue.size() > 4)
	{
		m_queue.pop_front();
	}

	m_queue.push_back(frame);
	m_state = Running;
	m_loss = 0;
}

void AudioQueue::stop(void)
{
	if (m_state != Running)
	{
		return;
	}

    m_queue.clear();

	AudioFrame nullFrame;

	m_queue.push_back(nullFrame);

}

bool AudioQueue::deque(AudioFrame &outFrame, bool &underrun)
{
	underrun = false;

	switch (m_state)
	{
	case Stopped:
		return false;

	case Running:

		if (m_queue.empty())
		{
			m_loss ++;

			if (m_loss >= protocol::MAX_AUDIO_LOSS)
			{
				m_state = Stopped;
				return deque(outFrame, underrun);
			}

			underrun = true;

			// Loss
			if (m_lastFrame.empty())
			{
				return true;
			}

			outFrame = m_lastFrame;
			return true;
		}

		outFrame = m_queue.front();
		m_queue.pop_front();

		if (outFrame.empty())
		{
			if (m_queue.empty())
			{
				m_state = Stopped;
				return false;
			}
			
			return deque(outFrame, underrun);
		}

		m_lastFrame = outFrame;
		return true;
	}

	return false;
}


}