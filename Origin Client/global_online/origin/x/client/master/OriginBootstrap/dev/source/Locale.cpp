///////////////////////////////////////////////////////////////////////////////
// Locale.cpp
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "Locale.h"
#include "Resource.h"
#include "LogService_win.h"

#include <stdio.h>
#include <ShlObj.h>
#include <MsXml.h>
#include <atlcomcli.h>
#include <comutil.h>

#include <vector>
#include <algorithm>

using namespace _com_util;

//Singleton
static Locale* mInstance = NULL;
Locale* Locale::GetInstance()
{
	if (!mInstance)
		CreateInstance(L"");

	return mInstance;
}

void Locale::DeleteInstance()
{
	if (mInstance)
		delete mInstance;
	mInstance = NULL;
}

void Locale::CreateInstance(wchar_t* locale)
{
	if (!mInstance)
		mInstance = new Locale(locale);
}

Locale::Locale(wchar_t* locale)
    : mEulaLocale(NULL)
{
	mLocale = new wchar_t[16];

	if (wcsicmp(locale, L"") == 0)
	{
		// If there are the same then check for our locale
		wchar_t* strMatch = wcsstr(GetCommandLine(), L"/Locale:\"");
		wstring locale;
		if (strMatch != 0)
		{
			strMatch += wcslen(L"/Locale:\"");
			locale = strMatch;
			int end = locale.find_first_of(L"\"");
			locale = locale.substr(0, end);
			wcscpy_s(mLocale, 16, locale.c_str());
		}
		else
		{
			// We get this formatted differently from the installer
			strMatch = wcsstr(GetCommandLine(), L"/locale ");
			if (strMatch != 0)
			{
				strMatch += wcslen(L"/locale ");
				locale = strMatch;
				// This is assuming that locale is of the form xx_YY
				locale = locale.substr(0, 5);
				wcscpy_s(mLocale, 16, locale.c_str());
			}
			else
			{
				// Grab our locale
				wchar_t langName[64];
				GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, langName, sizeof(langName)/sizeof(wchar_t));
				wchar_t countryCode[64];
				GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, countryCode, sizeof(langName)/sizeof(wchar_t));
				wsprintfW(mLocale, L"%s_%s", langName, countryCode);
			}
		}
	}
	else
	{
		wcscpy_s(mLocale, 16, locale);
	}

    InitEULALocale(mLocale);

    // This is assuming that locale is of the form xx_YY
    wstring language = mLocale;
    language = language.substr(0,2);

    // convert German languages to german locale
    if (wcsicmp(language.c_str(), L"de") == 0)
    {
        wcscpy_s(mLocale, 16, L"de_DE");
    }

    // convert French languags to french locale
    if (wcsicmp(mLocale, L"fr_CH") == 0 || wcsicmp(mLocale, L"fr_BE") == 0 || wcsicmp(mLocale, L"fr_CA") == 0 || wcsicmp(mLocale, L"fr_FX") == 0)
    {
        wcscpy_s(mLocale, 16, L"fr_FR");
    }

    // convert Italian languags to italian locale
    if (wcsicmp(mLocale, L"it_CH") == 0)
    {
        wcscpy_s(mLocale, 16, L"it_IT");
    }


	// Default to en_US if we don't recognize this locale
	if (!IsValidLocale())
	{
        ORIGINBS_LOG_EVENT << "Unsupported Locale: " << mLocale;
		wcscpy_s(mLocale, 16, L"en_US");
	}

	// Create our langauge map
	CreateLanguages();
}

Locale::~Locale() 
{
	// Need to clean up language map
	map<wchar_t*, wchar_t*, cmp_str>::iterator it = mLocaleMap.begin();
	while (it != mLocaleMap.end())
	{
		// Delete the valid memory and increment so we don't kill our iterator
		delete[] (it++)->first;
	}
	delete[] mLocale;
    delete[] mEulaLocale;
}

void Locale::Localize(wchar_t* stringID, wchar_t** retString, int size)
{
	wcscpy_s(*retString, size, mLocaleMap[stringID]);
}

void Locale::CreateLanguages()
{
	// Grab en_US by default
	HRSRC hRes = GetLocaleResource();

	HGLOBAL hBytes = LoadResource(NULL, hRes);
	DWORD dSize = SizeofResource(NULL, hRes);

	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD, dSize);

	wchar_t* pDest = reinterpret_cast<wchar_t*>(GlobalLock(hGlobal));
	wchar_t* pSrc = reinterpret_cast<wchar_t*>(LockResource(hBytes));

	CopyMemory(pDest, pSrc, dSize);
	FreeResource(hBytes);
	GlobalUnlock(hGlobal);

	CComPtr<IStream> pStream = NULL;
	CreateStreamOnHGlobal(hGlobal,FALSE,&pStream);

	CComPtr<IXMLDOMDocument> iXMLDoc;
	CComPtr<IXMLDOMElement> iRootElm;
	CComPtr<IXMLDOMNodeList> iNodeList;
	long length;
	VARIANT_BOOL bSuccess;

	iXMLDoc.CoCreateInstance(__uuidof(DOMDocument));
	if (iXMLDoc)
	{
		iXMLDoc->load(_variant_t(pStream), &bSuccess);
		iXMLDoc->get_documentElement(&iRootElm);
		if (iRootElm)
		{
			iRootElm->get_childNodes(&iNodeList);
			if (iNodeList)
			{
				iNodeList->get_length(&length);
				for (int i = 0; i < length; i++)
				{
					CComPtr<IXMLDOMNode> iNode;
					CComPtr<IXMLDOMNamedNodeMap> iAttrs;
					iNodeList->nextNode(&iNode);
					iNode->get_attributes(&iAttrs);
					if (iAttrs)
					{
						CComPtr<IXMLDOMNode> locKey;
						iAttrs->getNamedItem(L"Key", &locKey);
						if (locKey)
						{
							// Grab the string ID
							VARIANT val;
							locKey->get_nodeValue(&val);
							// Grab the string
							BSTR* translation = new BSTR[256];
							iNode->get_text(translation);
							wchar_t* key = val.bstrVal;
							mLocaleMap[key] = *translation;
						}
					}
				}
			}
		}
	}
}

HRSRC Locale::GetLocaleResource()
{
	// Default to en_US
	HRSRC hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_en_US), L"LANGUAGE");
	if (wcsicmp(mLocale, L"cs_CZ") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_cs_CZ), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"da_DK") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_da_DK), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"de_DE") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_de_DE), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"el_GR") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_el_GR), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"en_GB") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_en_GB), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"es_ES") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_es_ES), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"es_MX") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_es_MX), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"fi_FI") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_fi_FI), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"fr_FR") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_fr_FR), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"hu_HU") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_hu_HU), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"it_IT") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_it_IT), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"ja_JP") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_ja_JP), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"ko_KR") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_ko_KR), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"nl_NL") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_nl_NL), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"nb_NO") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_no_NO), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"pl_PL") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_pl_PL), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"pt_BR") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_pt_BR), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"pt_PT") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_pt_PT), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"ru_RU") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_ru_RU), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"sv_SE") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_sv_SE), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"th_TH") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_th_TH), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"zh_CN") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_zh_CN), L"LANGUAGE");
	}
	else if (wcsicmp(mLocale, L"zh_TW") == 0)
	{
		hRes = FindResource(NULL, MAKEINTRESOURCE(IDR_LANGUAGE_zh_TW), L"LANGUAGE");
	}
	return hRes;
}

bool Locale::IsValidLocale() const
{
	if (wcsicmp(mLocale, L"en_US") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"cs_CZ") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"da_DK") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"de_DE") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"el_GR") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"en_GB") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"es_ES") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"es_MX") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"fi_FI") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"fr_FR") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"hu_HU") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"it_IT") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"ja_JP") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"ko_KR") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"nl_NL") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"nb_NO") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"pl_PL") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"pt_BR") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"pt_PT") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"ru_RU") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"sv_SE") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"th_TH") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"zh_CN") == 0)
	{
		return true;
	}
	else if (wcsicmp(mLocale, L"zh_TW") == 0)
	{
		return true;
	}

	return false;
}

void Locale::InitEULALocale(wchar_t *locale)
{
    if( mEulaLocale == NULL )
    {
        mEulaLocale = new wchar_t[16];
        wcscpy_s(mEulaLocale, 16, L"en_US"); // default
    }

    // This is assuming that locale is of the form xx_YY
    if(locale)
    {
        wstring language = locale;
        wstring country = locale;

        language = language.substr(0,2);
        country = country.substr(3,5);
        
        // Is user in Germany?
        if( wcsicmp(country.c_str(), L"DE") == 0 )
        {
            // Does user speak English?
            if( wcsicmp(language.c_str(), L"en") == 0 )
            {
                wcscpy_s(mEulaLocale, 16, L"en_DE");
            }
            else
            {
                // default for Germany
                wcscpy_s(mEulaLocale, 16, L"de_DE");
            }
        }
        // Is user in France?
        else if( wcsicmp(country.c_str(), L"FR") == 0 )
        {
            // Always set to French EULA
            wcscpy_s(mEulaLocale, 16, L"fr_FR");
        }
        else
        {
            // check if we are in EU
            wstring er[24] = { L"AT", L"BE", L"CY", L"CZ", L"DK", L"EE", L"FI", L"GR", L"HU", L"IE", L"IT", L"LV", L"LT", L"LU", L"MT", L"NL", L"PL", L"PT", L"RO", L"SK", L"SI", L"ES", L"SE", L"GB" };
            vector<wstring> europeanRegions(&er[0], &er[0] + 24);
            vector<wstring>::iterator erIt = find(europeanRegions.begin(), europeanRegions.end(), country.c_str());
            if( erIt != europeanRegions.end() )
            {
                // default for EU
                wcscpy_s(mEulaLocale, 16, L"en_GB");

                wstring el[14] = { L"cs", L"da", L"el", L"en", L"es", L"fi", L"fr", L"hu", L"it", L"nl", L"pl", L"pt", L"sv", L"de" };
                vector<wstring> europeanLanguages(&el[0], &el[0] + 14);
                vector<wstring>::iterator elIt = find(europeanLanguages.begin(), europeanLanguages.end(), language.c_str()); 
                if( elIt != europeanLanguages.end() )
                {
                    if( wcsicmp((*elIt).c_str(), L"cs") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"cs_CZ");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"da") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"da_DK");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"el") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"el_GR");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"es") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"es_ES");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"fi") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"fi_FI");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"fr") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"fr_FR");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"hu") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"hu_HU");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"it") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"it_IT");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"nl") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"nl_NL");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"pl") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"pl_PL");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"pt") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"pt_PT");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"sv") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"sv_SE");
                    }
                    else if( wcsicmp((*elIt).c_str(), L"de") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"de_EU"); // special case for German
                    }
                    else // en or other
                    {
                        wcscpy_s(mEulaLocale, 16, L"en_GB");
                    }
                }
            }
            else // we are in ROW
            {
                // default for ROW
                wcscpy_s(mEulaLocale, 16, L"en_US");

                wstring rowl[11] = { L"en", L"es", L"fr", L"ja", L"ko", L"nb", L"pt", L"ru", L"th", L"zh", L"de" };
                vector<wstring> rowLanguages(&rowl[0], &rowl[0] + 11);
                vector<wstring>::iterator rowlIt = find(rowLanguages.begin(), rowLanguages.end(), language.c_str()); 
                if( rowlIt != rowLanguages.end() )
                {
                    if( wcsicmp((*rowlIt).c_str(), L"es") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"es_MX");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"fr") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"fr_CA");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"ja") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"ja_JP");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"ko") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"ko_KR");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"nb") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"no_NO");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"pt") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"pt_BR");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"ru") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"ru_RU");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"th") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"th_TH");
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"zh") == 0 )
                    {
                        if( wcsicmp(country.c_str(), L"TW") == 0 )
                        {
                            wcscpy_s(mEulaLocale, 16, L"zh_TW");
                        }
                        else
                        {
                            wcscpy_s(mEulaLocale, 16, L"zh_CN");
                        }
                    }
                    else if( wcsicmp((*rowlIt).c_str(), L"de") == 0 )
                    {
                        wcscpy_s(mEulaLocale, 16, L"de_RW"); // special case for Germany
                    }
                    else // en or other
                    {
                        wcscpy_s(mEulaLocale, 16, L"en_US");
                    }
                }
           }
        }
    }
}
