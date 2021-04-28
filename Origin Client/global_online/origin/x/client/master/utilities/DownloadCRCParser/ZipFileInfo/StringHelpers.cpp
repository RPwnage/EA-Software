// StringHelpers.cpp
// purpose: Helper functions for certain string operations

#include "WinDefs.h"
#include "StringHelpers.h"
#include "CoreFile.h"
#include <windows.h>

#include <QCryptographicHash>
#include <QRegExp>

#include <limits>

//using namespace Core;

bool StringHelpers::LoadQStringFromFile(const QString& sFilename, QString& string)
{
	CoreFile file(sFilename);
	if (!(file.exists() && file.open(QIODevice::ReadOnly)))
	{
#ifdef EADM
		EBILOGDEBUG << L"LoadQStringFromFile " << sFilename << L" failed\n";
#endif
		return false;
	}

	// Read the file into a buffer
	int64_t nFileSize = file.size();
	if (nFileSize == -1)
		return false;

	char* pBuf = new char[nFileSize];
	file.read(pBuf, nFileSize);

	// If we've read in a Unicode file, set the string (after the signature)
	if (IsTextUnicode(pBuf, nFileSize, NULL))
	{
		unsigned long nStringLength = (nFileSize-2) / sizeof(wchar_t);	// minus the signature size
		string = QString::fromWCharArray(reinterpret_cast<wchar_t*>(pBuf)+1, nStringLength);
	}
	else
	{
		// Otherwise do a Multibyte->Unicode conversion
		unsigned long nStringLength = 0;	// calculate string length

		wchar_t* pUnicodeBuf = new wchar_t[nFileSize];
		nStringLength = MultiByteToWideChar(CP_UTF8, 0, pBuf, nFileSize, pUnicodeBuf, nFileSize);
		string = QString::fromWCharArray(pUnicodeBuf, nStringLength);
		delete[] pUnicodeBuf;
	}

	delete[] pBuf;
	file.close();

	return true;
}

bool StringHelpers::LoadQStringFromFile(CoreFile& file, QString& string)
{
	// Read the file into a buffer
	int64_t nFileSize = file.size();
	if (nFileSize == -1)
		return false;

	char* pBuf = new char[nFileSize];
	file.read(pBuf, nFileSize);

	// If we've read in a Unicode file, set the string (after the signature)
	if (IsTextUnicode(pBuf, nFileSize, NULL))
	{
		unsigned long nStringLength = (nFileSize-2) / sizeof(wchar_t);	// minus the signature size
		string.fromWCharArray(reinterpret_cast<wchar_t*>(pBuf)+1, nStringLength);
	}
	else
	{
		// Otherwise do a Multibyte->Unicode conversion
		unsigned long nStringLength = 0;	// calculate string length

		wchar_t* pUnicodeBuf = new wchar_t[nFileSize];
		nStringLength = MultiByteToWideChar(CP_UTF8, 0, pBuf, nFileSize, pUnicodeBuf, nFileSize);
		string.fromWCharArray(pUnicodeBuf, nStringLength);
		delete[] pUnicodeBuf;
	}

	delete[] pBuf;

	return true;
}

bool StringHelpers::SaveQStringToFile(const QString& sFilename, QString& string)
{
	CoreFile file(sFilename);
	if (!file.open(QIODevice::ReadWrite))
	{
#ifdef EADM
		EBILOGDEBUG << L"SaveQStringToFile " << sFilename << L" failed\n";
#endif
		return false;
	}

	// Unicode UTF-16LittleEndian signature
	if (sizeof(wchar_t) == 2)
	{
		char sig[2];
		sig[0] = (char) 0xFF;
		sig[1] = (char) 0xFE;	
		file.write(sig, 2);
	}

	file.write(reinterpret_cast<const char*>(string.utf16()), string.size()*sizeof(wchar_t));
	file.close();

	return true;
}

bool StringHelpers::SaveQStringToFile_UTF8(const QString& sFilename, QString& string)
{
	CoreFile file(sFilename);
	if (!file.open(QIODevice::ReadWrite))
	{
#ifdef EADM
		EBILOGDEBUG << L"SaveQStringToFile " << sFilename << L" failed\n";
#endif
		return false;
	}

	QByteArray pUTF8Buffer(string.toUtf8());
	if (pUTF8Buffer.isEmpty())
		return false;

	// Unicode UTF-8 signature
	if (sizeof(wchar_t) == 2)
	{
		char sig[3];
		sig[0] = (char) 0xEF;
		sig[1] = (char) 0xBB;	
		sig[2] = (char) 0xBF;	
		file.write(sig, 3);
	}

	file.write(pUTF8Buffer.constData(), pUTF8Buffer.size());
	file.close();

	return true;
}

const QString ksStringListFileHeader("EALstringlist-ver1");
bool StringHelpers::SaveQStringListToFile(const QString& sFilename, QStringList& stringList)
{
	CoreFile file(sFilename);
	if (!file.open(QIODevice::ReadWrite))
	{
#ifdef EADM
		EBILOGDEBUG << L"SaveQStringListToFile " << sFilename << L" failed\n";
#endif
		return false;
	}

	// write header
	file.write(reinterpret_cast<const char*>(ksStringListFileHeader.utf16()), ksStringListFileHeader.size() * sizeof(wchar_t));

	// write num strings
	unsigned long nNumStrings = static_cast<unsigned long>(stringList.size());
	file.write(reinterpret_cast<char*>(&nNumStrings), sizeof(unsigned long));

	for (QStringList::iterator it = stringList.begin(); it != stringList.end(); it++)
	{
		QString& sString = *it;
		unsigned long nLength = sString.size();

		// Write length
		file.write(reinterpret_cast<char*>(&nLength), sizeof(unsigned long));

		// Write string data
		file.write(reinterpret_cast<const char*>(sString.utf16()), sString.size()*sizeof(wchar_t));
	}

	file.close();

	return true;
}

// Code snippet to test the stringlist writer/reader
/*{
	std::list<CString> testList;
	std::list<CString> testList2;
	testList.push_back(CString(L"test1"));
	testList.push_back(CString(L"test2"));
	testList.push_back(CString(L"test 3"));
	testList.push_back(CString(L"test 4"));
	testList.push_back(CString(L"??????????????: ?? ????? ??? ??????? ?"));

	CString sTestFilename = CString(L"c:/temp/testlist.ealsl");
	StringHelpers::SaveCoreStringListToFile(sTestFilename, testList);
	StringHelpers::LoadCoreStringListFromFile(sTestFilename, testList2);

	std::list<CString>::iterator it = testList.begin();
	std::list<CString>::iterator it2 = testList2.begin();
	for (; it != testList.end();)
	{
		CString sString1 = *it;
		CString sString2 = *it2;
		if (sString1 == sString2)
		{
			// good
			CoreLogDebug(L"String \"%s\" read fine", sString1);
		}
		else
		{
			CORE_ASSERT(false);
		}
		it++;
		it2++;
	}
}*/

// Purpose: Minimalistic removal of nested quotes etc for arguments passed to JScript
bool StringHelpers::EscapeJScriptArg(QString& sString)
{
	QString sEncodedChar;
	QString sTemp;
	int	nStrLen = sString.size();

	for ( int i = 0; i < nStrLen; i++ )
	{
		QChar c = sString.at( i );
		if ( c.unicode() > 0x00FF || (!c.isLetterOrNumber() && c != ' '))
		{
			sEncodedChar.sprintf("%%u%04x", c.unicode());
			sTemp += sEncodedChar;		
		}
		else
		{
			sTemp.append(c);
		}

	} // for ( int i = 0...
	sString = sTemp;
	return true;
}

// Purpose: Get a human readable size from a byte count
QString StringHelpers::SizeString( int64_t length )
{
	QString result;
	double d = static_cast<double>(length);
	if ( length > (1024 * 1024 * 1024) )
	{
		result = QString("%1 GB").arg(d / 1024 / 1024 / 1024, 0, 'f', 2);
	}
	else if ( length > (1024 * 1024) )
	{
		result = QString("%1 MB").arg(d / 1024 / 1024, 0, 'f', 2);
	}
	else if ( length > 1024 )
	{
		result = QString("%1 KB").arg(d / 1024, 0, 'f', 2);
	}
	else if ( length > 0 )
	{
		result = QString("%1 Bytes").arg(length);
	}

	return ( result );
}

// Purpose: Encodes a string so it can be safely passed in a URL (i.e. without affecting URL behavior).
// URL encoding characters can be found here: http://www.blooberry.com/indexdot/html\\topics/urlencoding.htm
bool StringHelpers::EscapeURL(QString& sString)
{
	QString sEncodedChar;
	QString sTemp;
	int	nStrLen = sString.size();

	for ( int i = 0; i < nStrLen; i++ )
	{
		QChar c = sString.at( i );

		if ( c == ' '  || c == '<'  || c == '>' || c == '#' || 
			 c == '%'  || c == '{'  || c == '}' || c == '|' || 
			 c == '\\' || c == '^'  || c == '~' || c == '[' || 
			 c == ']'  || c == '\'' || c == ';' || c == '/' || 
			 c == '?'  || c == ':'  || c == '@' || c == '=' || 
			 c == '&'  || c == '$'  || c == '+' )
		{
			sEncodedChar.sprintf("%%%02x", c.unicode());
			sTemp.append(sEncodedChar);			
		}
		else
		{
			sTemp.append(c);
		} // if ( c ==...

	} // for ( int i = 0...

	sString = sTemp;

	return true;
}

bool StringHelpers::IsAll8Bit(const QString sString)
{
	int	nStrLen = sString.size();
	for ( int i = 0; i < nStrLen; i++ )
		if (sString.at(i).unicode() > 0x00FF)
			return false;
	return true;
}

void StringHelpers::XMLSafeEncode(QString& sString)
{
	if(sString.contains(QRegExp("[&><'\"]")))
	{
		int i = 0;
		while (i < sString.size())
		{
			const QChar c = sString.at(i);
            switch (c.toLatin1())
			{
				case '&':
					sString.replace(i, 1, "&amp;");
					i += 5;
					break;
				case '>':
					sString.replace(i, 1, "&gt;");
					i += 4;
					break;
				case '<':
					sString.replace(i, 1, "&lt;");
					i += 4;
					break;
				case '\'':
					sString.replace(i, 1, "&apos;");
					i += 6;
					break;
				case '"':
					sString.replace(i, 1, "&quot;");
					i += 6;
					break;
				default:
					++i;
			}
		}
	}
}

inline long StringToHex(QString& sHex)
{
	long nValue = 0;
	for (int i = 0; i < 4; i++)
	{
		long nCharValue = 0;
        char c = sHex.at(i).toLatin1();
		if (isdigit(c))
			nCharValue = c - '0';
		else
			nCharValue = c - 'a' + 10;

		nValue = nValue << 4;
		nValue |= nCharValue;
	}

	return nValue;
}

// This encodes all characters into Unicode escape-sequences %uXXXX
bool StringHelpers::UnicodeUnescapeQString(QString& sStringToDecode)
{
	QString sProcessed;

	bool bProcessMore = true;
	int nIndex = 0;
	while (bProcessMore)
	{
		nIndex = sStringToDecode.indexOf("%u");
		if (nIndex >= 0)
		{
			sProcessed += sStringToDecode.left(nIndex);
			QString sValue(sStringToDecode.mid(nIndex + 2, 4));
			long nValue = StringToHex(sValue);

			QChar sChar(nValue);
			sProcessed.append(sChar);

			sStringToDecode = sStringToDecode.right(sStringToDecode.size() - (nIndex + 6));
		}
		else
			bProcessMore = false;
	}

	sProcessed.append(sStringToDecode.right(sStringToDecode.size() - nIndex));

	sStringToDecode = sProcessed;

	return true;
}

// This encodes all characters into Unicode escape-sequences %uXXXX
bool StringHelpers::UnicodeEscapeQString(QString& sStringToEncode)
{
	QString sTemp;

	QString sEncodedChar;

	int nStrLen = sStringToEncode.size();
	for (int i = 0; i < nStrLen; i++)
	{
		QChar c = sStringToEncode.at(i);
		// for some reason isalnum asserts... so we'll just look for an alpha numeric manually  *rolling eyes*
		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9'))
		{
			sTemp.append(c);
		}
		else
		{
			sEncodedChar.sprintf("%%u%04x", c.unicode());
			sTemp.append(sEncodedChar);
		}

	}

	sStringToEncode = sTemp;
	return true;
}

// This escapes 'problematic' characters for a javascript string literal.
// Specifically, single & double quote are escaped to \" and \'
//  Also, \ -> "\\" 
QString StringHelpers::EscapeForJavascriptStringLiteral(const QString &sStringToEscape)
{
	QString sEscaped;

	// assert if this string has already had its quotes escaped
	CORE_ASSERT( sStringToEscape.indexOf("\\'") < 0 && sStringToEscape.indexOf("\\\"") < 0 ); // already escaped!

	int nStrLen = sStringToEscape.size();
	QChar c;
	for (int i = 0; i < nStrLen; i++)
	{
		c = sStringToEscape.at(i);
		if( c == '\'' || c == '"' || c == '\\')
		{
			sEscaped.append('\\');
		}
		sEscaped.append(c);
	}

	return sEscaped;
}

template <typename T>
bool formatMsg(ulong dwErrorCode, T& t)
{
	void* lpMsgBuf;
	if (FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrorCode,
		// MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), // AMerican English
		(wchar_t*) &lpMsgBuf,
		0,
		NULL ))
	{
		t = (wchar_t*)lpMsgBuf;
		LocalFree(lpMsgBuf);
		return true;
	}
	return false;
}
// replaces all instances of an ead token with sValue ie "<eadtoken:sTokenName>" is replaced with sValue
//   returns the number of tokens replaced
/*int StringHelpers::ReplaceToken(CString& sString, const CString& sTokenName, const CString& sValue)
{
	CString sToken;
	sToken.Format(L"<eadtoken:%s>", sTokenName);
	return sString.Replace(sToken, sValue);
}*/

// returns a formatted windows error message given an error code (usually supplied by GetLastError).
// you can lookup system error codes here:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/debug/base/system_error_codes.asp
void StringHelpers::GetWindowsErrorMessage(QString& szMessage, ulong dwErrorCode)
{
	std::wstring msg;
	if (formatMsg(dwErrorCode, msg))
	{
		szMessage = QString::fromStdWString(msg);
	}
	else
	{
		szMessage = QString("Error Code: %1").arg(dwErrorCode);
	}
}

void StringHelpers::logLastError(const QString& what)
{
	QString szMessage;
	ulong error = StringHelpers::GetWindowsErrorMessage(szMessage);
#ifdef EADM
	EBILOGERROR << what << L" Error [" << szMessage << L" : " << error << L"]";
#endif
}

ulong StringHelpers::GetWindowsErrorMessage(QString &szMessage)
{
	ulong dwErrorCode = ::GetLastError();
	std::wstring msg;
	if (formatMsg(dwErrorCode, msg))
	{
		szMessage = QString::fromStdWString(msg);
	}
	else
	{   // Format message failed (unknown error?)
		szMessage = QString("Error Code: %1").arg(dwErrorCode);
	}

	return dwErrorCode;
}

// finds all instances of a token (marked by a start string and end string) and calls
// the ProcessTokenKey method on the handler allowing it to provide replacement text
// for the key enclosed within the token
void StringHelpers::FindTokenInstances( QString& aString,
										const QString& aTokenStart,
										const QString& aTokenEnd,
										TokenHandler *aHandler )
{
	int aCurPos = 0;
	QString aResult;

	while( aCurPos >= 0 )
	{
		int aStringStart = aString.indexOf( aTokenStart, aCurPos );

		if( aStringStart >= 0 )
		{
			// copy over everything between the current position and the token
			aResult.append(aString.mid( aCurPos, aStringStart - aCurPos ));

			// skip the token start delimiter
			aStringStart += aTokenStart.size();  

			int aStringEnd = aString.indexOf( aTokenEnd, aStringStart );
			CORE_ASSERT( aStringEnd >= 0 );

			QString aKey = aString.mid( aStringStart, aStringEnd - aStringStart );

			// Skip the token end delimiter
			aStringEnd += aTokenEnd.size();
			
			QString aReplacement;

			aHandler->ProcessTokenKey( aTokenStart, aTokenEnd, aKey, aReplacement );

			aResult.append(aReplacement);

			aCurPos = aStringEnd;
		}
		else
		{
			// No more tokens found, copy over the remainder of the string
			aResult.append(aString.mid( aCurPos ));
			break;
		}
	}

	aString = aResult;
}

/* ===========================================================================================
	Purpose: Hashes binary data with MD5 encryption and returns the result in the form of a 
			 32 character hexadecimal string.

	Parameters:	pData	   - Pointer to data (memory block of bytes).
				dataLength - The size (in bytes) of the data.

	Returns: o A 32 character hexadecimal string - success
			 o An empty string				     - failure

	Notes: Collisions may occur when hashing data > 16 bytes (128 bits).
   =========================================================================================== */
QString StringHelpers::GetMD5HashString( const char * pData, ulong dataLength )
{
	QCryptographicHash hash(QCryptographicHash::Md5);

	// stupid windows #defines max
	/*int len = std::numeric_limits<int>::max() > dataLength ? dataLength : std::numeric_limits<int>::max();
	while (dataLength > 0)
	{
		hash.addData(pData, len);
		pData += len;
		dataLength -= len;
	}*/

	hash.addData(pData, dataLength);

	return QString(hash.result());
}

void StringHelpers::AddHTTPGetParam(QString &strBaseURL, QString strParam, QString strValue)
{
	if (strBaseURL.indexOf('?') < 0)
		strBaseURL.append('?');
	else
		strBaseURL.append('&');

	if (!strParam.isEmpty())
	{
		strBaseURL.append(QString("%1=%2").arg(strParam).arg(strValue));
	}
}

void StringHelpers::RemoveHTTPGetParam(QString &strBaseURL, QString strParam)
{
	// This function doesn't differentiate between what's before the ?
	// and what's afterward, so make sure strParam is unique.
	
	// See if we can find the parameter
	int nTokenStart = strBaseURL.indexOf(strParam + "=");
	
	// If not, just return
	if (nTokenStart < 0)
		return;

	// Find where this token ends
	int nTokenEnd = nTokenStart + strParam.size() + 1;
	for (nTokenEnd; nTokenEnd < strBaseURL.size(); nTokenEnd++)
	{
		if (strBaseURL[nTokenEnd] == '&' || strBaseURL[nTokenEnd] == '?' || strBaseURL[nTokenEnd] == 0)
			break;
	}

	// If we hit the end of the string, remove the previous delimiter
	if (strBaseURL[nTokenEnd] == 0)
	{
		// But only if there is one
		if (nTokenStart != 0)
			nTokenStart--;
	}
	else 
		// Otherwise, remove the next delimiter
		// (because the previous delimiter may be a ? and we don't want to remove that)
		nTokenEnd++;

	// Get the full token
	QString strToken = strBaseURL.mid(nTokenStart, nTokenEnd - nTokenStart);

	// Cut it out of the string
	strBaseURL.remove(strToken);

}

int StringHelpers::FindNoCase(QString strString, QString strLookFor, int nStartAt)
{
	return strString.indexOf(strLookFor, nStartAt, Qt::CaseInsensitive);
}

// Returns number of matched instances replaced
unsigned int StringHelpers::ReplaceAll(std::wstring& szString, 
									   const wchar_t* sFind,
									   const wchar_t* sWith)
{
	unsigned int uReplCnt_ret = 0;

	std::wstring::size_type uFindLen = wcslen(sFind);
	std::wstring::size_type uWithLen = (sWith ? wcslen(sWith) : 0);

	std::wstring::size_type uPos=0;
	while ((uPos = szString.find(sFind, uPos)) != std::wstring::npos)
	{
		szString.erase(uPos, uFindLen);
		if (sWith)
		{
			szString.insert(uPos, sWith);
			uPos += uWithLen;
		}

		uReplCnt_ret++;
	}

	return uReplCnt_ret;
}

unsigned int StringHelpers::ReplaceAll(std::wstring& szString, 
									   wchar_t cFind,
									   wchar_t cWith)
{
	unsigned int uReplCnt_ret = 0;

	std::wstring::size_type uPos=0;
	while ((uPos = szString.find(cFind, uPos)) != std::wstring::npos)
	{
		szString.replace(uPos, 1, 1, cWith);

		uReplCnt_ret++;
	}

	return uReplCnt_ret;
}

unsigned int StringHelpers::RemoveAll(std::wstring& szString, 
									   const wchar_t* sRemove)
{
	unsigned int uRemoveCnt_ret = 0;

	std::wstring::size_type uRemoveLen = wcslen(sRemove);

	std::wstring::size_type uPos=0;
	while ((uPos = szString.find(sRemove, uPos)) != std::wstring::npos)
	{
		szString.erase(uPos, uRemoveLen);

		uRemoveCnt_ret++;
	}

	return uRemoveCnt_ret;
}

// According to MSDN, the following characters are reserved and should not be used in filenames:
// http://msdn.microsoft.com/en-us/library/aa365247.aspx
// here we'll strip all of these characters out
//  
//   < (less than)
//   > (greater than)
//   : (colon)
//   " (double quote)
//   / (forward slash)
//   \ (backslash)
//   | (vertical bar or pipe)
//   ? (question mark)
//   * (asterisk)
// We also want to strip out special characters that some games might not be able to handle in their paths
//   © (copyright)
//	 ™ (Trademark)
//	 ® (Reserved)
void StringHelpers::StripReservedCharactersFromFilename(QString& sFilename)
{
	sFilename.remove(QRegExp("[<>:\"/\\\\|?*©™®]"));
}

/* ===========================================================================================
Purpose: Encodes XML characters into XML escape tokens (i.e. DecodeStrings->EncodeStrings).

Parameters:	(In/Out) sInput - String to encode.
=========================================================================================== */
void StringHelpers::XMLEscape( QString & sInput )
{
	XMLSafeEncode(sInput);
}

/* ===========================================================================================
Purpose: Decodes XML escape tokens into normal XML characters (i.e. EncodeStrings->
			DecodeStrings).

Parameters:	(In/Out) sInput - String to decode.
=========================================================================================== */
void StringHelpers::XMLDeEscape( QString & sInput )
{
	sInput.replace("&amp;", "&");
	sInput.replace("&lt;", "<");
	sInput.replace("&gt;", ">");
	sInput.replace("&apos;", "'");
	sInput.replace("&quot;", "\"");
}

// Convert a delimited string to a vector of CString
// Given the string, the delimiter, and the vector.
void StringHelpers::TokenizeToList(const QString aDelimitedList, const QString aDelimiter, TokenList &aResultList)
{
	aResultList.clear();
	QStringList tokens(aDelimitedList.split(aDelimiter, QString::SkipEmptyParts));
	for (QStringList::const_iterator i = tokens.constBegin(); i != tokens.constEnd(); ++i)
	{
		aResultList.push_back(*i);
	}
}
