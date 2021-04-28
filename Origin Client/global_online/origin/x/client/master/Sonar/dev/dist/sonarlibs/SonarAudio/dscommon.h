#pragma once
#include <Windows.h>
#include <SonarCommon/Common.h>

namespace sonar
{
	GUID *StringToGuid (CString &inStr, GUID *storage);
	String GUIDToString (GUID *guid);
    void clearErrorsSent();
    bool errorSent(CString category, CString message, const int error);
}