#include <SonarAudio/DSCaptureDeviceEnum.h>
#include <SonarAudio/dscommon.h>

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>


namespace sonar
{

static BOOL CALLBACK CaptureDeviceEnum (LPGUID lpGuid, LPCWSTR lpwstrDescription, LPCWSTR lpwstrModule, LPVOID lpContext)
{	
	String strGuid;
	bool isDefault = false;
	if (lpGuid)
	{
		strGuid = GUIDToString (lpGuid);
	}
	else
	{
		isDefault = true;
		strGuid = "DEFAULT";
	}

	AudioDeviceList *pList = (AudioDeviceList *) lpContext;

	AudioDeviceId device;
	device.name = common::wideToUtf8(lpwstrDescription);
	device.id = strGuid;

	if (isDefault)
		pList->push_front(device);
	else
		pList->push_back (device);

	return TRUE;
}

DSCaptureDeviceEnum::DSCaptureDeviceEnum()
{
	DirectSoundCaptureEnumerateW (CaptureDeviceEnum, (void *) &m_list);
}

DSCaptureDeviceEnum::~DSCaptureDeviceEnum()
{
}

const AudioDeviceList &DSCaptureDeviceEnum::getDevices()
{
	return m_list;
}

}