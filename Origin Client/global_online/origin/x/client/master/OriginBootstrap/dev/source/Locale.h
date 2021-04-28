///////////////////////////////////////////////////////////////////////////////
// Locale.h
// 
// Created by Kevin Moore
// Copyright (c) 2011 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef LOCALE_H
#define LOCALE_H

#include <windows.h>
#include <map>

using namespace std;

struct cmp_str
{
	bool operator()(wchar_t* a, wchar_t* b)
	{
		return wcscmp(a, b) < 0;
	}
};

class Locale
{
public:
	wchar_t* GetSystemLocale() {return mLocale;}
    wchar_t* GetEulaLocale() {return mEulaLocale;}
	void Localize(wchar_t* stringID, wchar_t** retString, int size);

	static void CreateInstance(wchar_t* locale);
	static Locale* GetInstance();
	void DeleteInstance();

private:
	Locale(wchar_t* locale);
	Locale(const Locale &obj);
	~Locale();

	void CreateLanguages();
	HRSRC GetLocaleResource();
	bool IsValidLocale() const;

    void InitEULALocale(wchar_t *locale);

	wchar_t* mLocale;
    wchar_t* mEulaLocale;
	map<wchar_t*, wchar_t*, cmp_str> mLocaleMap;
};

#endif //LOCALE_H