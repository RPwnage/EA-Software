///////////////////////////////////////////////////////////////////////////////
// EULAVersions.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EULAVERSIONS_H
#define EULAVERSIONS_H
// Each time we release Origin, we need version numbers for our base EULAs embedded in the client
// These must be updated to match. 
// Version 1 was shipped with 8.4

#include <wchar.h>

#ifdef ORIGIN_MAC
// define _wcsicmp for MAC OS X
int _wcsicmp(const wchar_t* s1, const wchar_t* s2)
{
    char str1[128], str2[128];
    sprintf(str1, "%S", s1);
    sprintf(str2, "%S", s2);
    return strcasecmp(str1, str2);
}
#endif

namespace BootstrapShared
{

class EULA
{
public:
	static int GetVersion(const wchar_t* eula)
	{
		int retVersion = -1;
		
		if (_wcsicmp(eula, L"en_US") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"cs_CZ") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"da_DK") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"de_DE") == 0)
		{
			retVersion = 2;
		}
        else if (_wcsicmp(eula, L"de_EU") == 0)
        {
            retVersion = 3; // new: should match en_GB
        }
        else if (_wcsicmp(eula, L"de_RW") == 0)
        {
            retVersion = 3; // new: should match en_US
        }
		else if (_wcsicmp(eula, L"el_GR") == 0)
		{
			retVersion = 3;
		}
        else if (_wcsicmp(eula, L"en_DE") == 0)
        {
            retVersion = 2; // new: should match de_DE
        }
		else if (_wcsicmp(eula, L"en_GB") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"es_ES") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"es_MX") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"fi_FI") == 0)
		{
			retVersion = 3;
		}
        else if (_wcsicmp(eula, L"fr_CA") == 0)
        {
            retVersion = 3; // new: should match en_US
        }
		else if (_wcsicmp(eula, L"fr_FR") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"hu_HU") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"it_IT") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"ja_JP") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"ko_KR") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"nl_NL") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"no_NO") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"pl_PL") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"pt_BR") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"pt_PT") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"ru_RU") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"sv_SE") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"th_TH") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"zh_CN") == 0)
		{
			retVersion = 3;
		}
		else if (_wcsicmp(eula, L"zh_TW") == 0)
		{
			retVersion = 3;
		}
        else if (_wcsicmp(eula, L"en_DE") == 0)
		{
			retVersion = 3;
		}
        else if (_wcsicmp(eula, L"fr_CA") == 0)
		{
			retVersion = 3;
		}
		
		return retVersion;
	}
};

}

#endif //EULAVERSIONS_H