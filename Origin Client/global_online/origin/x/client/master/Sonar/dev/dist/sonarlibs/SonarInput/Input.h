#pragma once

#include <SonarCommon/Common.h>
#include <jlib/Thread.h>
#include <jlib/Spinlock.h>
#include "Stroke.h"

namespace sonar
{

class Input
{

public:

	Input(void);
	~Input(void);

	void update();

	void beginRecording();
	bool recordingInProgress();
	bool getRecording(Stroke &stroke);

	int bind(const Stroke &stroke);
	bool unbind(int bind);
	bool isActive (int bind);

	const static int STROKE_MAX_KEYS = 4;


private:
	bool recordKey(int key, bool state);

	void *threadProc();
	static void *StaticThreadProc(void *arg);
	jlib::Thread m_thread;
	jlib::Spinlock m_lock;

	static LRESULT CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam);
	bool handleKey (int key, bool state, int virtual_key);

	struct
	{
		Stroke stroke;
		int numReleases;
		bool done;
	} m_record;

	bool m_recordingMode;

	bool m_keyStates[Stroke::KEY_MAX];
	typedef SonarMap<int, Stroke> StrokeMap;
	StrokeMap m_strokes;

	int m_bindIdCounter;

	volatile bool m_started;

	bool m_ignoreRealKeys;

};


}