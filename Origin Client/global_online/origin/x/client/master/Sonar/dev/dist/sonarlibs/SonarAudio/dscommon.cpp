#include <SonarCommon/Common.h>

#include <windows.h>
#include <wtypes.h>
#include <set>
#include <iostream>

#define SONAR_DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const PROPERTYKEY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#define SONAR_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

SONAR_DEFINE_PROPERTYKEY(PKEY_AudioEndpoint_GUID, 0x1da5d803, 0xd492, 0x4edd, 0x8c, 0x23, 0xe0, 0xc0, 0xff, 0xee, 0x7f, 0x0e, 4);
SONAR_DEFINE_GUID(DSDEVID_DefaultVoicePlayback, 0xdef00002, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03);
SONAR_DEFINE_GUID(DSDEVID_DefaultVoiceCapture, 0xdef00003, 0x9c6d, 0x47ed, 0xaa, 0xf1, 0x4d, 0xda, 0x8f, 0x2b, 0x5c, 0x03);

namespace sonar
{
	
String GUIDToString (GUID *guid)
{
	char strTemp[256 + 1];
	_snprintf (strTemp, 256, "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
		guid->Data1,
		guid->Data2, 
		guid->Data3, 
		guid->Data4[0],
		guid->Data4[1],
		guid->Data4[2],
		guid->Data4[3],
		guid->Data4[4],
		guid->Data4[5],
		guid->Data4[6],
		guid->Data4[7]);

	return String(strTemp);
}

GUID *StringToGuid (CString &inStr, GUID *storage)
{
	unsigned long data[12];

	if (_snscanf(inStr.c_str(), inStr.size(), "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", 
		&data[0],
		&data[1],
		&data[2], 
		&data[3],
		&data[4],
		&data[5],
		&data[6],
		&data[7],
		&data[8],
		&data[9],
		&data[10]) != 11)
	{
		return NULL;
	}

	storage->Data1 = data[0];
	storage->Data2 = (unsigned short) data[1];
	storage->Data3 = (unsigned short) data[2];
	storage->Data4[0] = (unsigned char) data[3];
	storage->Data4[1] = (unsigned char) data[4];
	storage->Data4[2] = (unsigned char) data[5];
	storage->Data4[3] = (unsigned char) data[6];
	storage->Data4[4] = (unsigned char) data[7];
	storage->Data4[5] = (unsigned char) data[8];
	storage->Data4[6] = (unsigned char) data[9];
	storage->Data4[7] = (unsigned char) data[10];

	return storage;
}


std::set<CString> gDirectSoundErrorsSent;
void clearErrorsSent()
{
    gDirectSoundErrorsSent.clear();
}

bool errorSent(CString category, CString message, const int error)
{
    bool sent = false;

    std::stringstream errorId;
    errorId << category.c_str() << ":" << message.c_str() << ":" << error;
    CString errorIdStr = errorId.str().c_str();
    std::set<CString>::iterator it = gDirectSoundErrorsSent.find(errorIdStr);
    if( it != gDirectSoundErrorsSent.end() )
        sent = true;
    else
        gDirectSoundErrorsSent.insert(errorIdStr);

    return sent;
}

}