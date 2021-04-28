#include <SonarInput/Input.h>
#include <SonarCommon/Common.h>

#include <Windows.h>

namespace sonar
{

static __declspec( thread ) Input *tls_instance = NULL;

Input::Input(void)
{
	m_bindIdCounter = 0;
	memset (m_keyStates, 0, sizeof(m_keyStates));
	m_recordingMode = false;

	m_started = false;
	m_thread = jlib::Thread::createThread(StaticThreadProc, this);
	
	while (!m_started)
	{
		common::sleepFrame();
	}

	m_record.done = false;
	m_ignoreRealKeys = false;
}

Input::~Input(void)
{
	::PostThreadMessageW(m_thread.getId(), WM_QUIT, 0, 0);
	m_thread.join();

}

void *Input::StaticThreadProc(void *arg)
{
	Input *input = (Input *) arg;
	return input->threadProc();
}

LRESULT CALLBACK Input::KeyboardHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *pData = (KBDLLHOOKSTRUCT *) lParam;

	if (code < 0)
	{
		goto CALLNEXTHOOK;
	}

	if (tls_instance->m_ignoreRealKeys && pData->flags & LLKHF_INJECTED)
	{
		goto CALLNEXTHOOK;
	}

	bool bKeyDown = !(pData->flags & LLKHF_UP);	

    int key = pData->scanCode;
    int virtual_key = pData->vkCode;

	if (tls_instance->handleKey (key, bKeyDown, virtual_key))
	{
		goto BLOCKKEY;
	}

CALLNEXTHOOK:
	return CallNextHookEx (NULL, code, wParam, lParam);

BLOCKKEY:
	return 1;

}

LRESULT CALLBACK Input::MouseHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	if (code < 0)
	{
		return CallNextHookEx (NULL, code, wParam, lParam);
	}

	MSLLHOOKSTRUCT *pData = (MSLLHOOKSTRUCT *) lParam;

	if (tls_instance->m_ignoreRealKeys && pData->flags & LLMHF_INJECTED)
	{
		goto CALLNEXTHOOK;
	}

	bool bKeyDown = false;
	int key;

	switch (wParam)
	{
	case WM_LBUTTONDOWN:
		bKeyDown = true;
	case WM_LBUTTONUP:
		key = Stroke::KEY_LBUTTON;
		break;

	case WM_RBUTTONDOWN:
		bKeyDown = true; 
	case WM_RBUTTONUP:
		key = Stroke::KEY_RBUTTON;
		break;

	case WM_MBUTTONDOWN:
		bKeyDown = true; 
	case WM_MBUTTONUP:
		key = Stroke::KEY_MBUTTON;
		break;

	case WM_XBUTTONDOWN:
		bKeyDown = true; 
	case WM_XBUTTONUP:
		{
			int xkey = HIWORD (pData->mouseData);
			xkey -= 1;
			xkey &= 0x01;
			key = (Stroke::KEY_XBUTTON1 + xkey);
			break;
		}
	default:
		goto CALLNEXTHOOK;
		break;
	}

	if (tls_instance->handleKey (key, bKeyDown, 0))
	{
		goto BLOCKKEY;
	}

CALLNEXTHOOK:
	return CallNextHookEx (NULL, code, wParam, lParam);

BLOCKKEY:
	return -1;
}

bool Input::handleKey (int key, bool state, int virtual_key)
{
	jlib::Spinlocker locker(m_lock);

	bool isRep = (m_keyStates[key] == state);
	m_keyStates[key] = state;

	if (m_recordingMode && !isRep)
	{
		return recordKey(key, state);
	}

	bool returnValue = false;

	if (state)
	{
		for (StrokeMap::iterator iter = m_strokes.begin (); iter != m_strokes.end (); iter ++)
		{
			Stroke &stroke = iter->second;

			if (isRep)
			{
				if (stroke.isFinal ())
				{
					if (stroke.shouldBlock())
					{
						returnValue = true;
						continue;
                    }

					returnValue = false;
					continue;
				}
				else
				{
					/* This is a repetition on a stroke not being fully pressed. We ignore it */
					continue;
				}
			}

			if (!stroke.pressKey(key, virtual_key))
			{
				continue;
			}

			if (!stroke.isFinal())
			{
				continue;
			}

			if (stroke.shouldBlock())
			{
				returnValue = true;
				continue;
			}
		}
	}
	else
	{
		for (StrokeMap::iterator iter = m_strokes.begin (); iter != m_strokes.end (); iter ++)
		{
			Stroke &stroke = iter->second;

			bool bWasFinal = stroke.isFinal ();
		
			stroke.releaseKey(key, virtual_key);

			if (bWasFinal && !stroke.isFinal ())
			{
			}
		}
	}


	return returnValue;
}

bool Input::recordKey(int key, bool state)
{
	if (!state)
	{
		/* In order to not abort prematurely we wait for atleast one release of a key before we abort */
		if (m_record.numReleases >= 0)
		{
			/* We don't let users bind left mouse button as a single stroke */
			if (m_record.stroke.getKeys().size() == 1 &&
					m_record.stroke.getKeys().front() == Stroke::KEY_LBUTTON)
			{
				goto ABORT_RECORDING;
			}

			goto DONE_RECORDING;
		}

		m_record.numReleases ++;
		return true;
	}

	/*
	We abort if ESCAPE 
	*/

	if (key == Stroke::KEY_ESCAPE )
	{
		goto ABORT_RECORDING;
	}

	m_record.stroke.recordKey(key);

	if (m_record.stroke.getKeys().size() > STROKE_MAX_KEYS)
	{
		goto ABORT_RECORDING;
	}

	return true;

DONE_RECORDING:
	m_record.done = true;
	m_recordingMode = false;
	return true;

ABORT_RECORDING:
	m_record.done = true;
	m_recordingMode = false;
	m_record.stroke.clear();
	return true;
}


void *Input::threadProc(void)
{
	tls_instance = this;

	HHOOK hKbHook = NULL; 
	HHOOK hMsHook = NULL;

	bool hookMouse = !(getenv("SONAR_NO_MOUSE_HOOK") != NULL && strcmp(getenv("SONAR_NO_MOUSE_HOOK"), "1") == 0);
	bool hooksEnabled = !(getenv("SONAR_NO_HOOKS") != NULL && strcmp(getenv("SONAR_NO_HOOKS"), "1") == 0);
	m_ignoreRealKeys = !(getenv("SONAR_IGNORE_REAL_INPUT") != NULL && strcmp(getenv("SONAR_IGNORE_REAL_INPUT"), "1") == 0);

	if (hooksEnabled)
	{
		hKbHook = ::SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);

		if (hookMouse)
			hMsHook = ::SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, GetModuleHandle(NULL), 0);
	}

	m_started = true;

	MSG msg = { 0 };

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	if (hKbHook)
		::UnhookWindowsHookEx(hKbHook);
	if (hMsHook)
		::UnhookWindowsHookEx(hMsHook);

	return NULL;
}

void Input::update()
{

}

void Input::beginRecording()
{
	jlib::Spinlocker locker(m_lock);

	m_record.numReleases = 0;
	m_record.stroke.clear();
	m_recordingMode = true;

}

bool Input::recordingInProgress()
{
	jlib::Spinlocker locker(m_lock);
	return m_recordingMode;
}

bool Input::getRecording(Stroke &stroke)
{
	jlib::Spinlocker locker(m_lock);

	if (m_record.done)
	{
		stroke = m_record.stroke;
		m_record.done = false;
		return true;
	}

	return false;
}


int Input::bind(const Stroke &stroke)
{
	if (stroke.getKeys().empty())
	{
        return -1;
	}

	jlib::Spinlocker locker(m_lock);

	int idBind = m_bindIdCounter++;

	m_strokes[idBind] = stroke;
	return idBind;
}

bool Input::unbind(int bind)
{
	jlib::Spinlocker locker(m_lock);

	StrokeMap::iterator iter = m_strokes.find(bind);

	if (iter == m_strokes.end())
	{
		return false;
	}

	m_strokes.erase(iter);
	return true;
}


bool Input::isActive (int bind)
{
	if (bind == -1)
	{
		return false;
	}

	jlib::Spinlocker locker(m_lock);
	return m_strokes[bind].isFinal();
}

}
