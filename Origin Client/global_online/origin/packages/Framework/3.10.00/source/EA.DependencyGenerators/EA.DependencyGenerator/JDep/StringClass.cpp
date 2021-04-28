/*

	StringClass.h - String support class

*/

#include "StdAfx.h"
#include "StringClass.h"
#include <String.h>
#include <StdLib.h>
#include <Assert.h>


String::String(const char *pSrc)
{
	bAllocd = false;
	if(pSrc == 0)
		pBuf = 0;
	else
	{
		int Len = (int)strlen(pSrc);
		if(Len < STRING_Prealloc)
			pBuf = szString;
		else
		{
			pBuf = new char [Len+1];
			bAllocd = true;
		}
		strcpy(pBuf, pSrc);
	}
}

String &String::operator =(const char *pSrc)
{
	Release();

	if(pSrc == 0)
		pBuf = 0;
	else
	{
		int Len = (int)strlen(pSrc);
		if(Len < STRING_Prealloc)
			pBuf = szString;
		else
		{
			pBuf = new char [Len+1];
			bAllocd = true;
		}
		strcpy(pBuf, pSrc);
	}
	return *this;
}

String &String::operator =(const String &Src)
{
	return *this = Src.pBuf;
}

void String::Set(const char *pSrc, int Len)
{
	Release();
	if(pSrc == 0)
		pBuf = 0;
	else
	{
		if(Len < STRING_Prealloc)
			pBuf = szString;
		else
		{
			pBuf = new char [Len+1];
			bAllocd = true;
		}
		memcpy(pBuf, pSrc, Len+1);
	}
}


String &String::operator+=(const char *pSrc)
{
	if(pSrc == 0 || pSrc[0] == 0)
		return *this;

	if(pBuf == 0 || pBuf[0] == 0)
		return operator=(pSrc);

	int Len1 = (int)strlen(pBuf);
	int Len2 = (int)strlen(pSrc);

	int TotalLen = Len1 + Len2 + 1;
	if(TotalLen <= STRING_Prealloc)
	{
		assert(bAllocd == false);
		strcpy(pBuf + Len1, pSrc);
	}
	else
	{
		char *pOldString = pBuf;
		pBuf = new char[Len1 + Len2 + 1];
		strcpy(pBuf, pOldString);
		strcpy(pBuf + Len1, pSrc);
		if(bAllocd)
			delete [] pOldString;
		bAllocd = true;
	}
	return *this;
}

bool String::operator ==(char *pStr) const
{
	if(pBuf == 0)
		return (pStr == 0);
	if(pStr == 0)
		return false;

	return strcmp(pBuf, pStr) == 0;
}

bool String::operator ==(String &Str) const
{
	return *this == Str.pBuf;
}

int String::Length(void) const
{
	if(pBuf == 0) return 0;
	return (int)strlen(pBuf);
}
