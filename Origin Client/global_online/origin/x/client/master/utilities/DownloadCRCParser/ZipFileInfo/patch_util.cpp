#include "patch_util.h"
#ifdef CORE
#include "Logger.h"
#endif

// APP TODO - need this class?

#ifdef WIN32
#include <tchar.h>
#endif
#include "assert.h"

#define LOCAL_MIN(x,y)(x) <(y) ?(x) :(y)
#define LOCAL_MAX(x,y)(x) >(y) ?(x) :(y)

char *Util::StringNDup( const char *src, size_t len )
{
	if (len == 0)
		return 0;

	assert(src);

	char *dst = (char *)malloc( len + 1 );
	strncpy_s( dst, len, src, len );
	dst[len] = 0;
	return dst;
}


wchar_t *Util::StringNDup( const wchar_t *src, size_t len )
{
	if (len == 0)
		return 0;

	assert(src);

	wchar_t *dst = (wchar_t *)malloc( (len + 1)*sizeof(wchar_t));
	wcsncpy_s( dst, len+1, src, len );
	dst[len] = 0;
	return dst;
}


bool Util::IsEqual( const std::string& left, const std::string& right, bool case_sensitive )
{
	if (case_sensitive)
		return ( left.compare(right) == 0 );

	return ( _stricmp(left.c_str(), right.c_str()) == 0 ); 
}

bool Util::IsEqual( const std::wstring& left, const std::wstring& right, bool case_sensitive )
{
	if (case_sensitive)
		return ( left.compare(right) == 0 );

	return ( _wcsicmp(left.c_str(), right.c_str()) == 0 ); 
}

std::string Util::Mid(const std::string& str, size_t nFirst, size_t nChars)
{
	if(nFirst >= str.length())								//If position is out of range,
		return std::string();									//   Return an empty string
	const size_t nCount(LOCAL_MIN(str.length()-nFirst, nChars));	//Simply find out the proper #.
	return std::string(str, nFirst, nCount);					//   and return it.
}

std::string Util::Left(const std::string& str, size_t nChars)
{
	if(nChars > str.length())
		return str;
	return Mid(str, 0, nChars);
}

std::string Util::SplitToken(std::string& str, const char* pToken)
{
	if (pToken == 0)
		return str;

	const size_t nLength = strlen(pToken);
	if (nLength == 0)
		return str;

	const size_t nIndex = str.find(pToken, 0, nLength);
	if (nIndex == -1)
		return std::string();

	const std::string sLeft(str, 0, nIndex);
	str.erase(0, nIndex + nLength);

	return sLeft;
}

void Util::Sprintf(std::string& str, const char* pFormat, ...)
{
	va_list arguments;
	va_start(arguments, pFormat);
	SprintfVaList(str, pFormat, arguments);
	va_end(arguments);   
}

void Util::SprintfVaList(std::string& str, const char* pFormat, va_list arguments)
{
	// From unofficial C89 extension documentation:
	// The vsnprintf The number of characters written into the array,
	// not counting the terminating null character, or a negative value
	// if count or more characters are requested to be generated.
	// An error can occur while converting a value for output.

	// From the C99 standard:
	// The vsnprintf function returns the number of characters that would have
	// been written had n been sufficiently large, not counting the terminating
	// null character, or a negative value if an encoding error occurred.
	// Thus, the null-terminated output has been completely written if and only
	// if the returned value is nonnegative and less than n.

	const int kStackBufferSize(256);
	char      pStackBuffer[kStackBufferSize];
	int       nResult;

	nResult = _vsnprintf_s(pStackBuffer, kStackBufferSize, kStackBufferSize, pFormat, arguments);

	if(nResult >= (int)kStackBufferSize){
		// In this case we definitely have C99 vsnprintf behaviour.
		str.resize(nResult + 1); // '+1' because 'nResult' doesn't include the terminating 0.
		nResult = _vsnprintf_s(const_cast<char*>(str.data()), str.size(), str.size(), pFormat, arguments);
		str.resize(nResult);
	}
	else if(nResult < 0){
		// In this case we either have C89 extension behaviour or 
		// have a C99 encoding error. We assume C89.
		size_t nFormatSize = strlen(pFormat);
		size_t n           = LOCAL_MAX(nFormatSize + nFormatSize / 4, str.size() * 2);

		for(; (nResult < 0) && (n < 1000000); n *= 2){
			str.resize(n);
			nResult = _vsnprintf_s(const_cast<char*>(str.data()), str.size(), str.size(), pFormat, arguments);
		}
		if(nResult >= 0)
			str.resize(nResult);
	}
	else{
		str.assign(pStackBuffer, nResult);
	}
}

void Util::ConvertBinaryDataToASCIIArray(const uint8_t* pBinaryData, uint32_t nBinaryDataLength, char* pASCIIArray){
	const uint8_t* pEnd = pBinaryData + nBinaryDataLength;

	while(pBinaryData < pEnd){
		*pASCIIArray = (char)('0' + ((*pBinaryData & 0xF0) >> 4)); //Convert the high byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; //Convert the ':' to 'A', for example.
		pASCIIArray++;
		*pASCIIArray = (char)('0' + (*pBinaryData & 0x0F));        //Convert the low byte to a number between 1 and 15.
		if(*pASCIIArray > '9')
			*pASCIIArray += 7; //Convert the ':' to 'A', for example.
		pASCIIArray++;
		pBinaryData++;
	}

	*pASCIIArray = '0';
}

bool Util::ConvertASCIIArrayToBinaryData(const char* pASCIIArray, uint32_t nASCIIArrayLength, uint8_t* pBinaryData){
	const char* pEnd = pASCIIArray + nASCIIArrayLength;
	char        chTemp;
	bool        bReturnValue(true);

	while(pASCIIArray < pEnd){
		*pBinaryData = 0;

		for(int j=4; j>=0; j-=4){
			chTemp = *pASCIIArray;

			if(chTemp < '0'){ //Do some bounds checking.
				chTemp = '0';
				bReturnValue = false;
			}
			else if(chTemp > 'F'){ //Do some bounds checking.
				if(chTemp >= 'a' && chTemp <= 'f')
					chTemp -= 39; //Convert 'a' to ':'.
				else{
					chTemp = '0';
					bReturnValue = false;
				}
			}
			else if(chTemp > '9' && chTemp < 'A'){ //Do some bounds checking.
				chTemp = '0';
				bReturnValue = false;
			}
			else if(chTemp >= 'A')
				chTemp -= 7;

			*pBinaryData = (uint8_t)(*pBinaryData + ((chTemp - '0') << j));
			pASCIIArray++;
		}
		pBinaryData++;
	}
	return bReturnValue;
}

// return true if string matches TRUE/true/T/t or 1
bool Util::GetBool(const std::string& str)
{
	if (( _stricmp( str.c_str(), "true" ) == 0 ) ||
		( _stricmp( str.c_str(), "t") == 0) ||
		( atoi(str.c_str()) == 1 ))
	{
		return true;
	}

	return false;
}

int64_t Util::GetSint64(const std::string& str)
{
	return _strtoi64(str.c_str(), 0, 0);
}

uint64_t Util::GetUint64(const std::string& str)
{
	return _strtoui64(str.c_str(), 0, 0);
}

std::string Util::GetSint64String(int64_t num)
{
	return ""; // APP TODO POSTFESL
	/*
	char buf[32];
	sprintf(buf, "%"PRId64, num);
	return buf;
	*/
}

std::string Util::GetUint64String(uint64_t num)
{
	/*
	char buf[32];
	sprintf(buf, "%"PRId64, num);
	return buf;
	*/
	return ""; // APP TODO POSTFESL
}

int32_t Util::GetSint32(const std::string& str)
{
	return strtol(str.c_str(), 0, 0);
}

uint32_t Util::GetUint32(const std::string& str)
{
	return strtoul(str.c_str(), 0, 0);
}

std::string Util::GetSint32String(int32_t num)
{
	char buf[32];
	sprintf_s(buf, sizeof(buf), "%d", num);
	return buf;
}

std::string Util::GetUint32String(uint32_t num)
{
	char buf[32];
	sprintf_s(buf, sizeof(buf), "%d", num);
	return buf;
}



std::wstring Util::UTF8ToWChar(const std::string UTF8Str)
{
	return UTF8ToWChar(UTF8Str.c_str());
}


// Convert a string to wchar.
// Parameters
// UTF8Str - a null terminated UTF8 string.
std::wstring Util::UTF8ToWChar(const char *UTF8Str)
{
	std::wstring rval(L"");

	if ( UTF8Str && strlen(UTF8Str) > 0 )
	{
		wchar_t	*buffer = NULL;

		int size = MultiByteToWideChar(CP_UTF8,0,UTF8Str,-1,buffer,0);  

		if (size > 0)
		{
			buffer = new wchar_t[size];

			if (buffer)
			{
				if (MultiByteToWideChar(CP_UTF8, 0, UTF8Str,-1, buffer, size) > 0)
					rval.assign(buffer);

				delete [] buffer;
			}
		}
	}
	return rval;
}



std::string Util::WCharToUTF8(const std::wstring WCharStr)
{
	return WCharToUTF8(WCharStr.c_str());
}


// Convert a wchar string into UTF8.
// Parameters
// WCharStr - a null Unicode string.
std::string Util::WCharToUTF8(const wchar_t *WCharStr)
{
	std::string rval("");

	if ( WCharStr && wcslen(WCharStr) > 0 )
	{
		char	*buffer = NULL;

		// Determine how much space is needed
		int size = WideCharToMultiByte(CP_UTF8, 0,WCharStr,-1,buffer,0, NULL, NULL); 


		if (size > 0)
		{
			buffer = new char[size];

			if (buffer)
			{
				if (WideCharToMultiByte(CP_UTF8, 0, WCharStr,-1, buffer, size, NULL, NULL) > 0)
					rval.assign(buffer);

				delete [] buffer;
			}
		}
	}
	return rval;
}


void Util::EnumRegKeys( const HKEY PreDefKey, const char *SubKey, RegKeys& keys )
{
	HKEY hKey;

	// Get the handle to the registry key where the Key is located
#ifdef CORE
	if (!RegOpenKeyExA(PreDefKey, SubKey, 0L, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKey)== ERROR_SUCCESS)
		EBILOGWARNING << L"Util::EnumRegKeys unable to open registry key for enumeration";
#else
	RegOpenKeyExA(PreDefKey, SubKey, 0L, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &hKey);
#endif

	DWORD dwError;
	DWORD dwIndex = 0;
	const DWORD MAX_KEY_SIZE = 16383; // magic number from msdn
	char subDomainPartition[MAX_KEY_SIZE];
	do 
	{
		DWORD subDomainPartitionLength = MAX_KEY_SIZE;
		dwError = RegEnumKeyExA(hKey, dwIndex, subDomainPartition, &subDomainPartitionLength, 0, 0, 0, 0 );
		if (dwError != ERROR_SUCCESS)
			break;

		subDomainPartition[subDomainPartitionLength] = 0; // terminate the buffer
		keys.push_back(subDomainPartition);

		++dwIndex;
	} while (true);

#ifdef CORE
	if (dwError != ERROR_NO_MORE_ITEMS)
		EBILOGERROR << L"Util::EnumRegKeys encountered an error while enumerating an open registry key.";
#endif

	// Close the key
	if (hKey!= INVALID_HANDLE_VALUE ) 
		RegCloseKey(hKey);

}
