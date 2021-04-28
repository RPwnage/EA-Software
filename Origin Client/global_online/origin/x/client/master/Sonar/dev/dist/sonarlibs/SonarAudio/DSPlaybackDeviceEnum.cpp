#include <SonarAudio/DSPlaybackDeviceEnum.h>
#include <SonarAudio/dscommon.h>

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

namespace sonar
{
static BOOL CALLBACK PlaybackDeviceEnum (LPGUID lpGuid, LPCWSTR lpwstrDescription, LPCWSTR lpwstrModule, LPVOID lpContext)
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
		pList->push_front (device);
	else		
		pList->push_back (device);

	return TRUE;
}

DSPlaybackDeviceEnum::DSPlaybackDeviceEnum()
{
	DirectSoundEnumerateW (PlaybackDeviceEnum, (void *) &m_list);
}

DSPlaybackDeviceEnum::~DSPlaybackDeviceEnum()
{
}

const AudioDeviceList &DSPlaybackDeviceEnum::getDevices()
{
	return m_list;
}

}