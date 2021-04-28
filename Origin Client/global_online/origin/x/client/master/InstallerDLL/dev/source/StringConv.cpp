#include <windows.h>
#include <string>
#include "StringConv.h"

std::string WideToMB(const std::wstring& str, UINT codePage) 
{
	std::string ret;
	if(str.length() > 0)
	{
		DWORD charsNeeded = WideCharToMultiByte(codePage, 0, str.c_str(), -1, NULL, 0, NULL, NULL);
		_ASSERTE(charsNeeded > 0);
		if(charsNeeded > 0)
		{
			char* buf = new char[charsNeeded];
			_ASSERTE( buf != NULL );
			if( buf != NULL )
			{
				ZeroMemory(buf, charsNeeded);

				DWORD charsConverted = WideCharToMultiByte(codePage, 0, str.c_str(), -1, buf, charsNeeded, NULL, NULL);
				_ASSERTE( charsConverted > 0 );
				_ASSERTE( charsConverted <= charsNeeded );
                (void)charsConverted;

				buf[charsNeeded - 1] = 0;
				ret = buf;

				delete[] buf;
			}
		}
	}
	return ret;
}

std::wstring MBToWide(const std::string& str, UINT codePage) 
{
	std::wstring ret;
	if(str.length() > 0)
	{
		DWORD charsNeeded = MultiByteToWideChar(codePage, 0, str.c_str(), -1, NULL, 0);
		_ASSERTE(charsNeeded > 0);
		if(charsNeeded > 0)
		{
			wchar_t* buf = new wchar_t[charsNeeded];
			_ASSERTE( buf != NULL );
			if( buf != NULL )
			{
				size_t bytesNeeded = sizeof(wchar_t)*charsNeeded;
				ZeroMemory(buf, bytesNeeded);

				DWORD charsConverted = MultiByteToWideChar(codePage, 0, str.c_str(), -1, buf, charsNeeded);
				_ASSERTE( charsConverted > 0 );
				_ASSERTE( charsConverted <= charsNeeded );
                (void)charsConverted;

				buf[charsNeeded - 1] = 0;
				ret = buf;

				delete[] buf;
			}
		}
	}
	return ret;
}

std::string ANSIToUTF8(const std::string& str) 
{ 
	return WideToUTF8(MBToWide(str)); 
}
std::string UTF8ToANSI(const std::string& str) 
{
	return WideToMB(UTF8ToWide(str)); 
}
