///////////////////////////////////////////////////////////////////////////////
// EULAVersion.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef EULA_H
#define EULA_H
// Each time we release Origin, we need version numbers for our base EULAs embedded in the client
// These must be updated to match. 
// Version 1 was shipped with 8.4

#include "Locale.h"
#include "Resource.h"
#include "LogService_win.h"

#include <EULAVersions.h>

class EULA
{
public:
	static void GetEULAVersion(int* retVersion, int* retResource)
	{
		const wchar_t* eula = Locale::GetInstance()->GetEulaLocale();

        int eulaVersion = BootstrapShared::EULA::GetVersion(eula);

        if(eulaVersion == -1)
        {
            ORIGINBS_LOG_ERROR << "Cannot determine resource-based EULA version for locale: " << eula;
        }
        else
        {
            *retVersion = eulaVersion;
        }

		if (_wcsicmp(eula, L"en_US") == 0)
		{
			*retResource = IDR_HTML_en_US;
		}
		else if (_wcsicmp(eula, L"cs_CZ") == 0)
		{
			*retResource = IDR_HTML_cs_CZ;
		}
		else if (_wcsicmp(eula, L"da_DK") == 0)
		{
			*retResource = IDR_HTML_da_DK;
		}
		else if (_wcsicmp(eula, L"de_DE") == 0)
		{
			*retResource = IDR_HTML_de_DE;
		}
        else if (_wcsicmp(eula, L"de_EU") == 0)
        {
            *retResource = IDR_HTML_de_EU;
        }
        else if (_wcsicmp(eula, L"de_RW") == 0)
        {
            *retResource = IDR_HTML_de_RW;
        }
		else if (_wcsicmp(eula, L"el_GR") == 0)
		{
			*retResource = IDR_HTML_el_GR;
		}
        else if (_wcsicmp(eula, L"en_DE") == 0)
        {
            *retResource = IDR_HTML_en_DE;
        }
		else if (_wcsicmp(eula, L"en_GB") == 0)
		{
			*retResource = IDR_HTML_en_GB;
		}
		else if (_wcsicmp(eula, L"es_ES") == 0)
		{
			*retResource = IDR_HTML_es_ES;
		}
		else if (_wcsicmp(eula, L"es_MX") == 0)
		{
			*retResource = IDR_HTML_es_MX;
		}
		else if (_wcsicmp(eula, L"fi_FI") == 0)
		{
			*retResource = IDR_HTML_fi_FI;
		}
        else if (_wcsicmp(eula, L"fr_CA") == 0)
        {
            *retResource = IDR_HTML_fr_CA;
        }
		else if (_wcsicmp(eula, L"fr_FR") == 0)
		{
			*retResource = IDR_HTML_fr_FR;
		}
		else if (_wcsicmp(eula, L"hu_HU") == 0)
		{
			*retResource = IDR_HTML_hu_HU;
		}
		else if (_wcsicmp(eula, L"it_IT") == 0)
		{
			*retResource = IDR_HTML_it_IT;
		}
		else if (_wcsicmp(eula, L"ja_JP") == 0)
		{
			*retResource = IDR_HTML_ja_JP;
		}
		else if (_wcsicmp(eula, L"ko_KR") == 0)
		{
			*retResource = IDR_HTML_ko_KR;
		}
		else if (_wcsicmp(eula, L"nl_NL") == 0)
		{
			*retResource = IDR_HTML_nl_NL;
		}
		else if (_wcsicmp(eula, L"no_NO") == 0)
		{
			*retResource = IDR_HTML_no_NO;
		}
		else if (_wcsicmp(eula, L"pl_PL") == 0)
		{
			*retResource = IDR_HTML_pl_PL;
		}
		else if (_wcsicmp(eula, L"pt_BR") == 0)
		{
			*retResource = IDR_HTML_pt_BR;
		}
		else if (_wcsicmp(eula, L"pt_PT") == 0)
		{
			*retResource = IDR_HTML_pt_PT;
		}
		else if (_wcsicmp(eula, L"ru_RU") == 0)
		{
			*retResource = IDR_HTML_ru_RU;
		}
		else if (_wcsicmp(eula, L"sv_SE") == 0)
		{
			*retResource = IDR_HTML_sv_SE;
		}
		else if (_wcsicmp(eula, L"th_TH") == 0)
		{
			*retResource = IDR_HTML_th_TH;
		}
		else if (_wcsicmp(eula, L"zh_CN") == 0)
		{
			*retResource = IDR_HTML_zh_CN;
		}
		else if (_wcsicmp(eula, L"zh_TW") == 0)
		{
			*retResource = IDR_HTML_zh_TW;
		}
        else
        {
            ORIGINBS_LOG_ERROR << "Cannot determine EULA resource ID for locale: " << eula << " defaulting to en_US";
            *retVersion = BootstrapShared::EULA::GetVersion(L"en_US");
            *retResource = IDR_HTML_en_US;
        }
            
	}
};

#endif //EULA_H
