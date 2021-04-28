//StringHelpers.h
//
// purpose: Helper functions for certain string operations

#ifndef STRING_HELPERS_H
#define STRING_HELPERS_H

#include <stdint.h>

#ifdef WIN32
#ifdef AFX_PROJ
#include "stdafx.h"
#endif  // AFX_PROJ
#endif  // WIN32

//#include "CorePlatform.h"
#include <list>
#include <vector>
// QT <-> Windows specific helpers

#include <QString>
#include <QStringList>

//namespace Core
//{
	class CoreFile;
//}

namespace StringHelpers
{

	class TokenHandler
	{
	public:
		
		// called when a token is found by FindTokenInstances
		// aTokenStart and aTokenEnd can be used to decide the appropriate action
		// to take in replacing the key
		virtual void ProcessTokenKey( const QString& aTokenStart,
									  const QString& aTokenEnd,
									  QString& aKey,
									  QString& aReplacementValue ) = 0;
	};


/*#ifdef Q_WS_WIN
	// Some interum helpers to convert QStrings to CStrings
	// Note: May need to be updated with a better method or maybe we just leave it until all CStrings are eliminated
	inline CString QToC(const QString& szQString) { 
		return CString(szQString.utf16());
	}
	inline QString CToQ(const CString& szCString) { return QString::fromUtf16((LPCTSTR)szCString); }
#endif*/

	bool LoadQStringFromFile(const QString& sFilename, QString& string);
	bool LoadQStringFromFile(CoreFile& file, QString& string);

	bool SaveQStringToFile(const QString& sFilename, QString& string);
	bool SaveQStringToFile_UTF8(const QString& sFilename, QString& string);

	bool LoadQStringListFromFile(const QString& sFilename, QStringList& stringList);
	bool SaveQStringListToFile(const QString& sFilename, QStringList& stringList);

	// This encodes all characters into Unicode escape-sequences %uXXXX
	bool UnicodeEscapeQString(QString& sStringToEncode);
	bool UnicodeUnescapeQString(QString& sStringToDecode);

	bool EscapeJScriptArg(QString& sString);
	bool EscapeURL(QString& sString);

	bool IsAll8Bit(const QString sString);

	// Use XML safe encoding using &; format (examples: &amp;, &lt; etc.)
	void XMLSafeEncode(QString& sString);

	// This escapes 'problematic' characters for a javascript string literal.
	// Specifically, single & double quote are escaped to \" and \', and \ becomes "\\"
	QString EscapeForJavascriptStringLiteral(const QString &sStringToEscape);

	//Get a size in bytes formatted in the most appropriate unit
	QString SizeString( int64_t length );

	// replaces all instances of an ead token with sValue ie "<eadtoken:sTokenName>" is replaced with sValue
	//   returns the number of tokens replaced
	//int ReplaceToken(CString& sString, const CString& sTokenName, const CString& sValue);

	// returns a formatted windows error message given an error code (usually supplied by GetLastError).
	// you can lookup windows error codes here:
	// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/debug/base/system_error_codes.asp
	void GetWindowsErrorMessage(QString& szMessage, ulong dwErrorCode);
	///
	/// Obtains last error in human-readable form
	/// \param buffer to receive the human message
	///
	ulong GetWindowsErrorMessage(QString& szMessage);

	///
	/// helper to log last error
	///
	void logLastError(const QString& what);

	// finds all instances of a token (marked by a start string and end string) and calls
	// the ProcessTokenKey method on the handler allowing it to provide replacement text
	// for the key enclosed within the token
	void FindTokenInstances( QString& aString,
							 const QString& aTokenStart,
							 const QString& aTokenEnd,
							 TokenHandler *aHandler );

	// General 128-bit MD5 hasher that returns a hexadecimal string result.
	QString			GetMD5HashString( const char * pData, ulong dataLength ); 

	void AddHTTPGetParam(QString &strBaseURL, QString strParam, QString strValue);
	void RemoveHTTPGetParam(QString &strBaseURL, QString strParam);

	int FindNoCase(QString strString, QString strLookFor, int nStartAt = 0);

	void StripReservedCharactersFromFilename(QString& sFilename);		// note that this should NOT include a nested path since slashes would be stripped as well........ this can be a file or folder name.
	void StripReservedCharactersFromFilename(QString& sFilename);

	struct QStringCompareInsensitive
	{
		bool operator() (const QString& s1, const QString& s2) const
		{
			return s1.compare(s2, Qt::CaseInsensitive) < 0;
		}
	};

	// Replace all for generic string class
	unsigned int	ReplaceAll(std::wstring& szString, 
							const wchar_t* sFind,
							const wchar_t* sWith);
	unsigned int	ReplaceAll(std::wstring& szString, 
							wchar_t cFind,
							wchar_t cWith);
	unsigned int	RemoveAll(std::wstring& szString, 
							const wchar_t* sRemove);

	// Some XML helpers salvaged from the old XML classes
	void XMLEscape( QString & sInput );
	void XMLDeEscape( QString & sInput );

	// Convert a delimited string to a vector of CString
	// Given the string, the delimiter, and the vector.
	typedef std::vector< const QString > TokenList;
	void TokenizeToList(const QString aDelimitedList, const QString aDelimiter, TokenList &aResultList);
}

#endif