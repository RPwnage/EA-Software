#pragma once 

#ifndef SONAR_CLIENT_BUILD_CONFIG
#define SONAR_CLIENT_BUILD_CONFIG <SonarClient/DefaultBuildConfig.h>
#endif
#include SONAR_CLIENT_BUILD_CONFIG

namespace sonar
{
	class AudioQueue
	{
	public:
		enum State
		{
			Running,
			Stopped,
		};

		AudioQueue(void);
		~AudioQueue(void);
		void enqueue(const AudioFrame &frame);
		void stop(void);
		bool deque(AudioFrame &outFrame, bool &underrun);
		
	private:
		typedef SonarDeque<AudioFrame> AudioDeque;
		AudioDeque m_queue;
		State m_state;

		AudioFrame m_lastFrame;
		int m_loss;
	};
}