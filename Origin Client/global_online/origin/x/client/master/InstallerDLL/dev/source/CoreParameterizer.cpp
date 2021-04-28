// telemetry
#include "TelemetryAPIDLL.h"

#include "stdafx.h"
#include "CoreParameterizer.h"
#include "platform.h"

namespace Core
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// class Parameterizer

	Parameterizer::Parameterizer() : mKeyToValueMap() {}

	Parameterizer::Parameterizer(const CString& sPackedParameterString): mKeyToValueMap()
	{
		Parse(sPackedParameterString); // ignore boolean return
	}

	bool Parameterizer::ValidateKey(const CString &sKey) const
	{
		// we can't have an '=' in a key... other characters should be ok...
		return (sKey.Find(L'=') == -1);
	}

	const CString VALUE_START( L"<eadvalue:" );
	const CString VALUE_END  ( L":eadvalue>" );
	const long nValueStartLength = VALUE_START.GetLength();
	const long nValueEndLength = VALUE_END.GetLength();

	bool Parameterizer::Parse(const CString& sPackedParameterString, bool bIgnoreCase)
	{
		if( sPackedParameterString.IsEmpty() )
		{
			return true;
		}

		int nLength = sPackedParameterString.GetLength();
		int nCurPos = sPackedParameterString.Find( L'?' );         // find the ? that begins the parameters

		if( nCurPos == -1 )
		{
			return false;
		}

		nCurPos++; // eat the '?'

		//	int nCurPos = 0;


		CString sKey;

		while( nCurPos >= 0 )
		{
			// Parse Key
			sKey = sPackedParameterString.Tokenize( L"=", nCurPos );

			if( !sKey.IsEmpty() )
			{
				if( bIgnoreCase )
				{
					sKey.MakeLower(); // case insensitive keys
				}

				CString sValue;

				// If the value is empty, the next char is an '&' or it has passed the end of the string
				if( nCurPos < nLength && sPackedParameterString.GetAt( nCurPos ) != L'&' )
				{
					// If the value begins with VALUE_START take everything until VALUE_END
					if( sPackedParameterString.Mid( nCurPos, nValueStartLength ) == VALUE_START )
					{
						nCurPos += nValueStartLength;	// Advance past the VALUE_START delimiter

						int aValueDelimEndPos = sPackedParameterString.Find( VALUE_END, nCurPos );	// Find the VALUE_END

						if( aValueDelimEndPos > 0 )	// If the VALUE_END was found
						{
							// Extract the value as everything between VALUE_START and VALUE_END
							sValue = sPackedParameterString.Mid( nCurPos, aValueDelimEndPos-nCurPos );
						}

						// Advance past the VALUE_END and past the '&' (if there is one)
						nCurPos = aValueDelimEndPos + nValueEndLength + 1;	// +1 skips the next '&'
					}
					else
					{
						// Take everything until the next '&' (or everything until the end of the string if no '&')
						sValue = sPackedParameterString.Tokenize(L"&", nCurPos);
					}
				}
				else
				{
					nCurPos++;
				}

				// Add the mapping
				mKeyToValueMap[sKey] = sValue;
			}
		}

		return true;
	}

	// NOTE: the packed param string is suitable to be added to another Parameterizer object as a string entry (since all string entries are encoded)
	bool Parameterizer::GetPackedParameterString(CString& sPackedString, CString sKeyPrefix) const
	{
		sPackedString = L"?";
		for (tKeyToValueMap::const_iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end();)
		{
			CString sKey = (*it).first;
			CString sValue = (*it).second;

			EncaseValueIfNecessary(sValue);		// adds delimiters around the value if it contains any characters that could interfere with parsing

			sPackedString += sKeyPrefix;
			sPackedString += sKey;    // The key
			sPackedString += L"=";
			sPackedString += sValue;   // The Value

			it++;

			if (it != mKeyToValueMap.end())     // if there are more parameters, add a '&'
				sPackedString += L"&";

		}

		return true;
	}

	bool Parameterizer::IsParameterPresent(CString sKey) const
	{
		ASSERT(ValidateKey(sKey));
		sKey.MakeLower(); // case insensitive keys

		tKeyToValueMap::const_iterator it = mKeyToValueMap.find(sKey);

		return it != mKeyToValueMap.end();
	}

	// helper func to assist parsing integers
	// given a key, gets its value and parses out <count> comma delimited integers
	// so, "0,1,2,3" -> [0,1,2,3]
	//
	// Note: intArray is resized to hold 'count' elements
	// returns true if count were read, and will assert if less than count elements were found.
	bool Parameterizer::ParseIntegers(const CString &sKey, const int count, CSimpleArray<int> &intArray) const
	{
		CString sValue;
		if( !GetString(sKey, sValue) )
			return false;

		intArray.RemoveAll();

		// parse out integers
		CString sInt;
		int i;
		int nPos = 0;
		for(i=0; i<count ; ++i)
		{
			if( nPos >= 0 )
			{
				sInt = sValue.Tokenize(L",", nPos); // nPos is set to -1 if token is not found...
				intArray[i] = _wtoi(sInt);
			}
			else
			{
				// corrupt entry?  Was expecting more integers...
				ASSERT(false);
				return false;
			}
		}

		return true;
	}


	// string values are urlencoded/decoded
	// this allows us to add a packed param string into another parameter object (used by notification callbacks)
	// it also allows us to move path info safely about (ex: "program files\Earth & Beyoned\Launcher.exe")
	// if it becomes a performance issue, we can fall back to having SetString/GetString and SetEncodedString/GetEncodedString methods
	bool Parameterizer::GetString(CString sKey, CString& sValue, bool bIgnoreCase) const
	{
		ASSERT(ValidateKey(sKey));

		if (bIgnoreCase)
			sKey.MakeLower();

		// Must use map::find.  Can't just use [] operator as (by nature of the map template) a new mapping is created if it does not already exist.
		tKeyToValueMap::const_iterator it = mKeyToValueMap.find(sKey); 
		if (it != mKeyToValueMap.end())
		{

			// url unescape the value
			sValue = (*it).second;
			if( UrlUnescapeInPlace(sValue.GetBuffer(), 0) != S_OK )
			{
				ASSERT(false);
				return false;
			}
			sValue.ReleaseBuffer();
			return true;
		}

		return false;
	}


	// Wrapper for ::Get, returns "" if no key existed
	CString Parameterizer::GetString(CString sKey) const
	{
		CString sValue;
		GetString(sKey, sValue);
		return sValue;
	}

	// string values are urlencoded/decoded
	// this allows us to add a packed param string into another parameter object (used by notification callbacks)
	// it also allows us to move path info safely about (ex: "program files\Earth & Beyoned\Launcher.exe")
	// if it becomes a performance issue, we can fall back to having SetString/GetString and SetEncodedString/GetEncodedString methods
	bool Parameterizer::SetString(CString sKey, const CString& sValue, bool bIgnoreCase)
	{
		ASSERT(ValidateKey(sKey));

		if (sValue == L"")
		{
			mKeyToValueMap[sKey] = L"";
			return true;
		}

		CString szEscapedBuffer;
		DWORD bufferSize = min(0x00040000, sValue.GetLength() * 2);

		HRESULT hRes = UrlEscape(sValue, szEscapedBuffer.GetBufferSetLength(bufferSize), &bufferSize, URL_ESCAPE_SEGMENT_ONLY | URL_ESCAPE_PERCENT);

		if ( hRes == E_POINTER )
		{
			szEscapedBuffer.ReleaseBufferSetLength(0);

			//buffer was too small to contain the result, bufferSize contains required size
			hRes = UrlEscape(sValue, szEscapedBuffer.GetBufferSetLength(bufferSize), &bufferSize, URL_ESCAPE_SEGMENT_ONLY | URL_ESCAPE_PERCENT);
		}

		if ( hRes == S_OK )
		{
			szEscapedBuffer.ReleaseBuffer(bufferSize);

			if ( bIgnoreCase )
				sKey.MakeLower();

			mKeyToValueMap[sKey] = szEscapedBuffer;
		}
		else
		{
			szEscapedBuffer.ReleaseBuffer(0);
		}

		return hRes == S_OK;
	}

	// remove a parameter if it exists, returns true if something was removed
	bool Parameterizer::UnsetString(CString sKey, bool bIgnoreCase)
	{
		if (bIgnoreCase)
			sKey.MakeLower();
		return mKeyToValueMap.erase(sKey) > 0 ? true : false; // erase returns # elements erased, ternary form to shutup stupid compiler warning
	}


	bool Parameterizer::GetColor(const CString& sKey, unsigned char& nR, unsigned char& nG, unsigned char& nB)
	{
		CSimpleArray<int> intArray;
		if( !ParseIntegers(sKey, 3, intArray) )
		{
			return false;
		}

		// Format must be r,g,b 
		nR = (unsigned char) intArray[0];
		nG = (unsigned char) intArray[1];
		nB = (unsigned char) intArray[2];

		return true;
	}

	bool Parameterizer::SetColor(const CString& sKey, unsigned char nR, unsigned char nG, unsigned char nB)
	{
		ASSERT(ValidateKey(sKey));

		CString sColorString;
		sColorString.Format(L"%u,%u,%u", nR, nG, nB);

		return SetString(sKey, sColorString);
	}


	bool Parameterizer::GetRect(const CString& sKey, CRect& area) const
	{
		CSimpleArray<int> intArray;
		if( !ParseIntegers(sKey, 4, intArray) )
		{
			return false;
		}

		// format must be L,T,R,B
		area.left   = intArray[0];
		area.top    = intArray[1];
		area.right  = intArray[2];
		area.bottom = intArray[3];

		return true;
	}

	// if key not found, returns CRect()
	CRect Parameterizer::GetRect(const CString& sKey) const
	{
		CRect result;
		GetRect(sKey, result);
		return result;
	}

	bool Parameterizer::SetRect(const CString& sKey, const CRect& area)
	{
		ASSERT(ValidateKey(sKey));

		CString sRectString;
		sRectString.Format(L"%ld,%ld,%ld,%ld", area.left, area.top, area.right, area.bottom);

		return SetString(sKey, sRectString);
	}

	bool Parameterizer::GetPoint(const CString& sKey, CPoint& point) const
	{
		CSimpleArray<int> intArray;
		if( !ParseIntegers(sKey, 2, intArray) )
		{
			return false;
		}

		// Format must be x,y
		point.x = intArray[0];
		point.y = intArray[1];

		return true;
	}

	// if key not found, returns CPoint()
	CPoint Parameterizer::GetPoint(const CString& sKey) const
	{
		CPoint result;
		GetPoint(sKey, result);
		return result;
	}

	bool Parameterizer::SetPoint(const CString& sKey, const CPoint& point)
	{
		ASSERT(ValidateKey(sKey));

		CString sPointString;
		sPointString.Format(L"%ld,%ld", point.x, point.y);

		return SetString(sKey, sPointString);
	}

	bool Parameterizer::GetLong(const CString& sKey, long& nValue) const
	{
		CString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.IsEmpty())
		{
			ASSERT(false);
			// bad number retrieval?
			return false;
		}

		nValue = _wtol(sNumberString);

		return true;
	}

	// returns 0 if no key was found
	long Parameterizer::GetLong(const CString& sKey) const
	{
		long result = 0;
		GetLong(sKey, result);
		return result;
	}

	bool Parameterizer::SetLong(const CString& sKey, const long& nValue)
	{
		ASSERT(ValidateKey(sKey));

		CString sNumberString;
		sNumberString.Format(L"%ld", nValue);

		return SetString(sKey, sNumberString);
	}

	bool Parameterizer::GetULong(const CString& sKey, unsigned long& nValue) const
	{
		CString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.IsEmpty())
		{
			// bad number retrieval?
			return false;
		}

		nValue = _wtol(sNumberString);

		return true;
	}

	// returns 0 if no key was found
	unsigned long Parameterizer::GetULong(const CString& sKey) const
	{
		unsigned long result = 0;
		GetULong(sKey, result);
		return result;
	}

	bool Parameterizer::SetULong(const CString& sKey, const unsigned long& nValue)
	{
		ASSERT(ValidateKey(sKey));

		CString sNumberString;
		sNumberString.Format(L"%lu", nValue);

		return SetString(sKey, sNumberString);
	}

	bool Parameterizer::Get64BitInt(const CString& sKey, int64_t& nValue) const
	{
		CString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.IsEmpty())
		{
			ASSERT(false);
			// bad number retrieval?
			return false;
		}

		nValue = _wtoi64(sNumberString);

		return true;
	}

	int64_t Parameterizer::Get64BitInt(const CString& sKey) const
	{
		int64_t result = 0;
		Get64BitInt(sKey, result);
		return result;
	}

	bool Parameterizer::Set64BitInt(const CString& sKey, int64_t nValue)
	{
		ASSERT(ValidateKey(sKey));

		CString sNumberString;
		sNumberString.Format(L"%I64d", nValue);

		return SetString(sKey, sNumberString);
	}



	// GetBool follows c-style bool rules (non-zero == true)
	bool Parameterizer::GetBool(const CString& sKey, bool& bValue) const
	{
		long num;
		if( !GetLong(sKey, num ) )
			return false;

		bValue = (num != 0);
		return true;
	}

	// GetBool follows c-style bool rules (non-zero == true)
	// will return false if no key was found
	bool Parameterizer::GetBool(const CString& sKey) const
	{
		bool bValue = false;
		GetBool(sKey, bValue);
		return bValue;
	}

	// SetBool will always serialize to either a 1 or a 0...
	bool Parameterizer::SetBool(const CString& sKey, const bool bValue)
	{
		ASSERT(ValidateKey(sKey));
		WCHAR *boolString = ( bValue ? L"1" : L"0" );
		return SetString(sKey, boolString );
	}

	bool Parameterizer::GetVoid(const CString& sKey, void** ppVoid) const
	{
		ASSERT(ppVoid);

		CString sVoidString;
		if (!GetString(sKey, sVoidString))
			return false;

		if (sVoidString.IsEmpty())
		{
			ASSERT(false);
			return false;
		}

		*ppVoid = (void*) _wtoi64(sVoidString);

		return true;
	}

	void* Parameterizer::GetVoid(const CString& sKey) const
	{
		void* pVoid = NULL;
		GetVoid(sKey, &pVoid);

		return pVoid;
	}

	bool Parameterizer::SetVoid(const CString& sKey, const void* pVoid)
	{
		CString sVoid;
		sVoid.Format(L"%I64d", (__int64) pVoid);
		return SetString(sKey, sVoid);
	}

	bool Parameterizer::GetFloat(const CString& sKey, float& nValue) const
	{
		CString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.IsEmpty())
		{
			ASSERT(false);
			// bad number retrieval?
			return false;
		}

		nValue = (float) _wtof(sNumberString);

		return true;
	}

	// if no key found, returns 0
	float Parameterizer::GetFloat(const CString& sKey) const
	{
		float result = 0;
		GetFloat(sKey, result);
		return result;
	}

	bool Parameterizer::SetFloat(const CString& sKey, const float& nValue)
	{
		ASSERT(ValidateKey(sKey));

		CString sNumberString;
		sNumberString.Format(L"%f", nValue);

		return SetString(sKey, sNumberString);
	}

	bool Parameterizer::Clear(CString sKey)
	{
		ASSERT(ValidateKey(sKey));

		sKey.MakeLower();

		tKeyToValueMap::iterator it = mKeyToValueMap.find(sKey);

		if (it != mKeyToValueMap.end())
		{
			mKeyToValueMap.erase(it);
			return true;
		}

		return false;
	}

	bool Parameterizer::ClearAll()
	{
		mKeyToValueMap.clear();
		return true;
	}

	int Parameterizer::ClearStartingWith(const CString& sKeyPrefix)
	{
		int iCnt = 0;
		for (tKeyToValueMap::iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end(); )
		{
			const CString &sKey = (*it).first;

			if (!sKey.Left(sKeyPrefix.GetLength()).CompareNoCase(sKeyPrefix))
			{
				it = mKeyToValueMap.erase(it);
				iCnt++;
			}
			else
				it++;
		}
		return iCnt;
	}

	int Parameterizer::GetNumParameters()
	{
		return (int) mKeyToValueMap.size();
	}


	void Parameterizer::GetAllParams(tKeyToValueMap &mapIn)
	{
		mapIn.clear();

		for (tKeyToValueMap::iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end(); it++)
		{
			const CString &sKey = (*it).first;
			CString sValue;
			GetString(sKey, sValue, false /*bIgnoreCase*/);
			// add key/value pair to mapIn
			mapIn[sKey] = sValue;
		}
	}
	void Parameterizer::GetPreferencesStartingWith(const CString& sKeyPrefix, tKeyToValueMap& mapIn)
	{
		mapIn.clear();

		for (tKeyToValueMap::iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end(); it++)
		{
			const CString &sKey = (*it).first;

			if (!sKey.Left(sKeyPrefix.GetLength()).CompareNoCase(sKeyPrefix))
			{
				CString sValue;
				GetString(sKey, sValue, false /*bIgnoreCase*/);
				// add key/value pair to mapIn
				mapIn[sKey] = sValue;
			}
		}
	}

	CString Parameterizer::GetAllMappingsDebugString()
	{
		CString sMappings(L"<table class='helpTable' ><tr><center><td><b>Help - Dump Parameterizer Mappings</b></td></center></tr>");
		for (tKeyToValueMap::iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end(); it++)
		{
			const CString &sKey = (*it).first;
			CString sValue;
			GetString(sKey, sValue);

			// gotta html escape
			sValue.Replace(L"&", L"&amp;");
			sValue.Replace(L"<", L"&lt;");
			sValue.Replace(L">", L"&gt;");

			sMappings.AppendFormat(L"<tr><td>%s</td><td>=</td><td>%s</td></tr>", sKey, sValue);
		}
		sMappings.Append(L"</table>");

		// gotta printf escape, since some strings have printf formatting characters in them
		sMappings.Replace(L"%", L"%%");
		sMappings.Replace(L"\\", L"\\\\");

		return sMappings;
	}

	bool Parameterizer::EncaseValueIfNecessary( CString& sValue ) const
	{
		// If the string contains any of the following: = & SPACE
		// encase it in value delimiters

		if( sValue.FindOneOf( L"=& " ) >= 0 )
		{
			sValue = VALUE_START + sValue + VALUE_END;
			return true;
		}

		return false;
	}
}