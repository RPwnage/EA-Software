#include <SonarInput/Input.h>
#include <stdlib.h>
#include <SonarCommon/Common.h>
#include <assert.h>

typedef sonar::String String;

static void WideAssert(const wchar_t *expr, const wchar_t *file, int line)
{
	wchar_t message[1024 + 1];
	_snwprintf (message, 1024, L"TEST FAILED at %s:%d> %s\n", file, line, expr);
	fwprintf (stderr, L"%s", message);
	fflush(stderr);
	exit(-1);
}

#define ASSERT(_Expression) (void)( (!!(_Expression)) || (WideAssert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )

class Timeout
{
public:

	Timeout()
	{
	}

	Timeout(long long length)
	{
		m_expires = sonar::common::getTimeAsMSEC() + length;
	}

	bool expired()
	{
		return (sonar::common::getTimeAsMSEC() > m_expires);
	}

private:
	long long m_expires;

};

#define RUN_TEST(_func) \
	fprintf (stderr, "%s is running\n", #_func);\
	_func();\
	fprintf (stderr, "%s is done\n", #_func);


static void InjectKeyboard(int scanCode, bool state)
{
	UINT vk = MapVirtualKeyExW(scanCode, MAPVK_VSC_TO_VK, NULL);
	ASSERT(vk != 0);

	keybd_event( (BYTE) vk, (BYTE) scanCode, state ? 0 : KEYEVENTF_KEYUP, NULL);
}

static void InjectMouse(int flags)
{
	DWORD dwFlags = flags;

	mouse_event(dwFlags, 0, 0, 0, NULL);
}

//=============================================================================
//= test_RecordStrokeComplete
//=============================================================================
void test_RecordStrokeComplete()
{
	sonar::Input input;

	String stringStroke;
	stringStroke += sonar::Stroke::getKeyName(0x1d);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(0x2a);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(0x31);
	

	input.beginRecording();
	Timeout t(1000);

	sonar::Stroke stroke;
	
	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT
	InjectKeyboard(0x31, true); // N
	InjectKeyboard(0x31, false); // N
	InjectKeyboard(0x2a, false); // SHIFT
	InjectKeyboard(0x1d, false); // CTRL
	
	while (!t.expired() && !input.getRecording(stroke))
	{
		sonar::common::sleepFrame();
	}

	String s = stroke.toString();

	ASSERT(s == stringStroke);
	ASSERT (!t.expired());
}

//=============================================================================
//= test_RecordStrokeCompleteWithMouse
//=============================================================================
void test_RecordStrokeCompleteWithMouse()
{
	sonar::Input input;

	String stringStroke;
	stringStroke += sonar::Stroke::getKeyName(0x1d);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(0x2a);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(sonar::Stroke::KEY_LBUTTON);

	input.beginRecording();
	Timeout t(1000);

	sonar::Stroke stroke;
	
	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT
	InjectMouse(MOUSEEVENTF_LEFTDOWN);
	InjectMouse(MOUSEEVENTF_LEFTUP);
	InjectKeyboard(0x2a, false); // SHIFT
	InjectKeyboard(0x1d, false); // CTRL
		
	while (!t.expired() && !input.getRecording(stroke))
	{
		sonar::common::sleepFrame();
	}
	
	String s = stroke.toString();

	ASSERT(s == stringStroke);
	ASSERT (!t.expired());
}

//=============================================================================
//= test_RecordStrokeAbortLeftMouse
//=============================================================================
void test_RecordStrokeAbortLeftMouse()
{
	sonar::Input input;

	input.beginRecording();
	Timeout t(1000);

	sonar::Stroke stroke;

	InjectMouse(MOUSEEVENTF_LEFTDOWN);
	InjectMouse(MOUSEEVENTF_LEFTUP);
		
	while (!t.expired() && !input.getRecording(stroke))
	{
		sonar::common::sleepFrame();
	}

	ASSERT(stroke.getKeys().empty());
	ASSERT (!t.expired());
}

//=============================================================================
//= test_RecordStrokeAbortEscape
//=============================================================================
void test_RecordStrokeAbortEscape()
{
	sonar::Input input;

	input.beginRecording();
	Timeout t(1000);

	sonar::Stroke stroke;
	
	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT

	InjectKeyboard(0x01, true); // Escape
	InjectKeyboard(0x01, false); // Escape

	InjectKeyboard(0x2a, false); // SHIFT
	InjectKeyboard(0x1d, false); // CTRL
	
	while (!t.expired() && !input.getRecording(stroke))
	{
		sonar::common::sleepFrame();
	}

	ASSERT (stroke.getKeys().empty());
	ASSERT (!t.expired());
}

//=============================================================================
//= test_RecordStrokeMaximum
//=============================================================================
void test_RecordStrokeMaximum()
{
	sonar::Input input;

	String stringStroke;
	stringStroke += sonar::Stroke::getKeyName(0x1d);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(0x2a);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(0x31);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(0x32);


	input.beginRecording();
	Timeout t(1000);

	sonar::Stroke stroke;
	
	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT
	InjectKeyboard(0x31, true); // N
	InjectKeyboard(0x32, true); // M

	InjectKeyboard(0x32, false); // M
	InjectKeyboard(0x31, false); // M
	InjectKeyboard(0x2a, false); // M
	InjectKeyboard(0x1d, false); // M
	
	while (!t.expired() && !input.getRecording(stroke))
	{
		sonar::common::sleepFrame();
	}

	String s = stroke.toString();

	ASSERT (s == stringStroke);
	ASSERT (!t.expired());
}

//=============================================================================
//= test_RecordStrokeOne
//=============================================================================
void test_RecordStrokeOne()
{
	sonar::Input input;

	input.beginRecording();
	Timeout t(1000);

	sonar::Stroke stroke;
	
	InjectKeyboard(0x31, true); // N
	InjectKeyboard(0x31, false); // N
	
	while (!t.expired() && !input.getRecording(stroke))
	{
		sonar::common::sleepFrame();
	}

	String s = stroke.toString();

	ASSERT (s == "N");
	ASSERT (!t.expired());
}


//=============================================================================
//= test_RecordStrokeAbortTooMany
//=============================================================================
void test_RecordStrokeAbortTooMany()
{
	sonar::Input input;

	input.beginRecording();
	Timeout t(1000);

	sonar::Stroke stroke;
	
	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT
	InjectKeyboard(0x31, true); // N
	InjectKeyboard(0x32, true); // M
	InjectKeyboard(0x33, true); // O

	InjectKeyboard(0x33, false); // O
	InjectKeyboard(0x32, false); // M
	InjectKeyboard(0x31, false); // M
	InjectKeyboard(0x2a, false); // M
	InjectKeyboard(0x1d, false); // M
	
	while (!t.expired() && !input.getRecording(stroke))
	{
		sonar::common::sleepFrame();
	}

	ASSERT (stroke.getKeys().empty());
	ASSERT (!t.expired());
}

//=============================================================================
//= test_BindPressAndRelease
//=============================================================================
void test_BindPressAndRelease()
{
	sonar::Input input;
	Timeout t;

	sonar::Stroke stroke("B", true);
	int idBind = input.bind(stroke);

	t = Timeout(1000);

	while (!t.expired() && input.isActive(idBind))
	{
		sonar::common::sleepFrame();
	}

	ASSERT (!t.expired());

	InjectKeyboard(0x30, true);

	t = Timeout(1000);

	while (!t.expired() && !input.isActive(idBind))
	{
		sonar::common::sleepFrame();
	}

	ASSERT (!t.expired());

	InjectKeyboard(0x30, false);


	t = Timeout(1000);

	while (!t.expired() && input.isActive(idBind))
	{
		sonar::common::sleepFrame();
	}

	ASSERT (!t.expired());
}

//=============================================================================
//= test_BindUnbind
//=============================================================================
void test_BindUnbind()
{
	sonar::Input input;
	Timeout t;
	sonar::Stroke stroke("B", true);
	int idBind = input.bind(stroke);
	ASSERT (input.unbind(idBind));
}

//=============================================================================
//= test_BindPressHoldAndReleaseComplex
//=============================================================================
void test_BindPressHoldAndReleaseComplex()
{
	sonar::Input input;

	String stringStroke;
	stringStroke += sonar::Stroke::getKeyName(0x1d);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(0x2a);
	stringStroke += " + ";
	stringStroke += sonar::Stroke::getKeyName(sonar::Stroke::KEY_LBUTTON);

	sonar::Stroke stroke(stringStroke.c_str(), true);
	int idBind = input.bind(stroke);
	
	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT
	InjectMouse(MOUSEEVENTF_LEFTDOWN);

	Timeout t(1000);
	while (!t.expired() && !input.isActive(idBind))
	{
		sonar::common::sleepFrame();
	}

	ASSERT(!t.expired());

	t = Timeout(3000);

	while (!t.expired())
	{
		InjectKeyboard(0x1d, true); // CTRL
		InjectKeyboard(0x2a, true); // SHIFT
		InjectMouse(MOUSEEVENTF_LEFTDOWN);

		ASSERT (input.isActive(idBind));

    for (int x = 0; x < 10; x ++)
  		sonar::common::sleepFrame();
	}

	InjectMouse(MOUSEEVENTF_LEFTUP);

	t = Timeout(1000);

	while (!t.expired() && input.isActive(idBind))
	{
		sonar::common::sleepFrame();
	}

	ASSERT (!t.expired ());

	InjectKeyboard(0x2a, false); // SHIFT
	InjectKeyboard(0x1d, false); // CTRL
}

int wmain(int argc, wchar_t **argv)
{
	//ASSERT(::LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | KLF_SETFORPROCESS));

	SetEnvironmentVariable(L"SONAR_IGNORE_REAL_INPUT", L"1");

	RUN_TEST(test_BindPressHoldAndReleaseComplex);
	RUN_TEST(test_BindUnbind);
	RUN_TEST(test_BindPressAndRelease);
	RUN_TEST(test_RecordStrokeComplete);
	RUN_TEST(test_RecordStrokeOne);
	RUN_TEST(test_RecordStrokeMaximum);
	RUN_TEST(test_RecordStrokeCompleteWithMouse);
	RUN_TEST(test_RecordStrokeAbortLeftMouse);
	RUN_TEST(test_RecordStrokeAbortEscape);
	RUN_TEST(test_RecordStrokeAbortTooMany);

	return 0;
}