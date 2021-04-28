#ifndef SONAR_CLIENT_BUILD_CONFIG
#define SONAR_CLIENT_BUILD_CONFIG <SonarClient/DefaultBuildConfig.h>
#endif
#include SONAR_CLIENT_BUILD_CONFIG

#include <SonarCommon/Common.h>
#include <deque>

#define ASSERT(_Expression) exit(-1)

int wmain(int argc, wchar_t **argv)
{
	_set_abort_behavior(0, _WRITE_ABORT_MSG );
	::CoInitialize(NULL);

	const int SAMPLES_PER_SECOND = 16000;
	const int SAMPLES_PER_FRAME = SAMPLES_PER_SECOND / 50;
	const int FRAMES_PER_SECOND = (SAMPLES_PER_SECOND / SAMPLES_PER_FRAME);
	const int FRAME_LENGTH = (1000 / FRAMES_PER_SECOND);

	sonar::AudioPlaybackEnum playbackEnum;
	sonar::AudioCaptureEnum captureEnum;

	sonar::AudioRuntime runtime;

	sonar::AudioPlaybackDevice playback(runtime, playbackEnum.getDevices().front(), SAMPLES_PER_SECOND, SAMPLES_PER_FRAME, 16);
	sonar::AudioCaptureDevice capture(runtime, captureEnum.getDevices().front(), SAMPLES_PER_SECOND, SAMPLES_PER_FRAME, 16, FRAMES_PER_SECOND);
	sonar::AudioPlaybackBuffer *buffer = playback.createBuffer(SAMPLES_PER_SECOND, SAMPLES_PER_FRAME);

	SonarDeque<sonar::AudioFrame> frames;
	sonar::AudioFrame lastFrame;

	lastFrame.resize(SAMPLES_PER_FRAME);

	int threshold = 1;
	int LIMIT_ADJUST_MAX = FRAMES_PER_SECOND;
	int limitAdjust = LIMIT_ADJUST_MAX / 2;
	int underruns = 0;
	int frameCount = 0;
	

	double average = 0.0;

	sonar::INT64 tsStart = sonar::common::getTimeAsMSEC();
	sonar::INT64 timeToExecute = 0;

	argc --;
	argv ++;

	timeToExecute = 100000;
	
	while (sonar::common::getTimeAsMSEC() - tsStart < timeToExecute && timeToExecute > 0)
	{
		for (;;)
		{
			sonar::AudioFrame frame;
			sonar::CaptureResult cr = capture.poll(frame);

			if (cr != sonar::CR_Success)
			{
				break;
			}

			for (size_t index = 0; index < frame.size(); index ++)
			{
				average += abs(frame[index]);
			}

			frames.push_back(frame);
		}

		for (;;)
		{
			sonar::PlaybackResult pr = buffer->query();

			if (pr == sonar::PR_Success ||
					pr == sonar::PR_Underrun)
			{
				frameCount ++;

				if (!frames.empty())
				{
					lastFrame = frames.front();
					frames.pop_front();
				}
				else
				{
					underruns ++;
				}

				if (frameCount > FRAMES_PER_SECOND)
				{
					average /= (double) FRAMES_PER_SECOND * (double) SAMPLES_PER_FRAME;

					fprintf (stderr, "%010d, %010d, %010u, %010d, %g\n", threshold, limitAdjust, frames.size (), underruns, average);

					if (underruns > 0)
						threshold += 2;
					else
						threshold --;

					if (threshold > FRAMES_PER_SECOND)
						threshold = FRAMES_PER_SECOND;
					if (threshold < 1)
						threshold = 1;

					frameCount = 0;
					underruns = 0;
					average = 0.0;
				}

				buffer->enqueue(lastFrame);
			}
			else
			{
				break;
			}
			
		}
		

		while (frames.size () > (size_t) threshold)
		{
			frames.pop_front();
		}

		sonar::common::sleepFrame();
	}

	return 0;
}