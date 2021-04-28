//
//	Header file for FESL utility methods.
//
//	Copyright 2005 Electronic Arts Inc.  All rights reserved.
//

#ifndef PATCH_UTIL_H
#define PATCH_UTIL_H

#include <stdint.h>
//#include "platform.h"
#include <vector>
#include <string>
#include <windows.h>

template< typename value_t >
struct util_array : public std::vector< value_t >
{
// Helper template for c-style array usage
// c-style array interface
	value_t *	get_pointer(void) const { return (this->empty() ? 0 : (value_t *)&(*this)[0]);	}
	int			get_count(void) const	{ return (int)this->size();							}
};

class Util
{
public:
	///////////////////////////////////////////////////////////////////////////
	// StrDup
	// 
	// duplicate string by length versus termination
	// 
	// PRE: if len > 0 src must be valid
	//
	static char *StringNDup( const char *src, size_t len );

	static wchar_t *StringNDup( const wchar_t *src, size_t len );

	static bool IsEqual( const std::string& left, const std::string& right, bool case_sensitive = true );

	static bool IsEqual( const std::wstring& left, const std::wstring& right, bool case_sensitive = true );

	///////////////////////////////////////////////////////////////////////////////
	// Mid
	//
	static std::string Mid(const std::string& str, size_t nFirst, size_t nChars=-1);

	///////////////////////////////////////////////////////////////////////////////
	// Left
	//
	static std::string Left(const std::string& str, size_t nChars);

	///////////////////////////////////////////////////////////////////////////////
	// SplitToken
	//
	// SplitToken acts somewhat like strtok:
	// It searches a string for a token, and if found
	// modifies the string to whatever is after the found token.
	// The return value is whatever was to the left of the token.
	// The function is therefore destructive.
	//    ex.  String = "12345"
	//    returnString = String.SplitToken("34");
	//    Now String == "5"
	//    and returnString == "12"
	//
	static std::string SplitToken(std::string& str, const char* pToken);

	static void MakeLower(std::string& str)
	{
		_strlwr_s( const_cast<char*>( str.c_str() ), str.length()+1 );
	}

	///////////////////////////////////////////////////////////////////////////////
	// LTrim
	//
	// Removes leading spaces from string.
	//
	static void LTrim(std::string& str)
	{
		str.erase(0, str.find_first_not_of(" \t"));
	}


	///////////////////////////////////////////////////////////////////////////////
	// RTrim
	//
	static void RTrim(std::string& str)
	{
		str.erase(str.find_last_not_of(" \t")+1);
	}


	///////////////////////////////////////////////////////////////////////////////
	// Trim
	//
	static void Trim(std::string& str)
	{
		LTrim(str);
		RTrim(str);
	}

	///////////////////////////////////////////////////////////////////////////////
	// Sprintf
	//
	// A full format specification is as follows:
	//    %[flags][width][.precision][{h | l | ll | L}]type
	// You can lookup the meaning of each in a C language reference under 'printf'.
	// Flags consists of at most one of any of the following characters:
	//    '-' '+' ' ' '#' '0'
	// Width and precision are in the format of:
	//    #.# where '#' is any digit or the '*' char.
	// Type is any of the following:
	//    d, i, o, u, x, X, D, O, U, f, e, E, g, G, c, s, C, S, p, n
	//
	// To use these format specifiers safely with sized types such as uint32_t
	// and char, you need to use Framework-defined sized printf format specifiers.
	// These are defined in the C99 standard (but replicated here) in section 7.8.1.
	// So to portably print the following string, instead of doing it like this:
	//    Sprintf("%hd %d, %ll", nUint16, nSint32, nSint64);
	// would would need to do this:
	//    Sprintf("%"PRIu16" %"PRId32", %"PRId64"", nUint16, nSint32, nSint64);
	// This isn't pretty, but it is the standard solution. You can make this 
	// easier to read at debug time by creating a temporary string pointer to 
	// hold the format. It may also help to put the intended format in a comment
	// above the Sprintf statement.
	//
	static void Sprintf(std::string& str, const char* pFormat, ...);

	///////////////////////////////////////////////////////////////////////////////
	// SprintfVaList
	//
	static void SprintfVaList(std::string& str, const char* pFormat, va_list arguments);

	///////////////////////////////////////////////////////////////////////////////
	// ConvertBinaryDataToASCIIArray
	//
	// Since every binary byte converts to exactly 2 ascii bytes, the ASCII
	// array  must have space for at least twice the amount of bytes
	// as 'nBinaryDataLength' + 1.
	//
	static void ConvertBinaryDataToASCIIArray(const uint8_t* pBinaryData, uint32_t nBinaryDataLength, char* pASCIIArray);

	///////////////////////////////////////////////////////////////////////////////
	// ConvertASCIIArrayToBinaryData
	//
	// We have a boolean return value because it is possible that the ascii data is
	// corrupt. We check for this corruption and return false if so, while converting
	// all corrupt bytes to valid ones.
//
	static bool ConvertASCIIArrayToBinaryData(const char* pASCIIArray, uint32_t nASCIIArrayLength, uint8_t* pBinaryData);

	// Get bool from string
	static bool GetBool(const std::string& str);

	// Get int64 from string
	static int64_t GetSint64(const std::string& str);
	static uint64_t GetUint64(const std::string& str);

	// Get string from int64
	static std::string GetSint64String(int64_t num);	
	static std::string GetUint64String(uint64_t num);

	// Get int32 from string
	static int32_t GetSint32(const std::string& str);
	static uint32_t GetUint32(const std::string& str);

	// Get string from int32
	static std::string GetSint32String(int32_t num);
	static std::string GetUint32String(uint32_t num);

	// Return the wide-char representation of the given UTF8 string.
	static std::wstring UTF8ToWChar(const std::string UTF8Str);
	static std::wstring UTF8ToWChar(const char *UTF8Str = NULL);
	
	// Return a string that is the utf8 equivalent of the given unicode string
	static std::string WCharToUTF8(const std::wstring WCharStr);
	static std::string WCharToUTF8(const wchar_t *WCharStr = NULL);

	// Enumerate keys and save to string vector
	typedef std::vector<std::string> RegKeys;
	static void EnumRegKeys( const HKEY PreDefKey, const char *SubKey, RegKeys& keys );
};

#endif
