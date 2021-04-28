//StringHelpers.h
//
// purpose: Helper functions for certain string operations

#ifndef STRING_HELPERS_H
#define STRING_HELPERS_H

#include <list>
#include <vector>
// QT <-> Windows specific helpers

#include <QString>
#include <QStringList>
#include <QFile>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{

class File;

namespace StringHelpers
{
	
	ORIGIN_PLUGIN_API bool detectUnicode(unsigned char* buff, qint64 buffSize);

	ORIGIN_PLUGIN_API bool LoadQStringFromFile(const QString& sFilename, QString& string);
	ORIGIN_PLUGIN_API bool LoadQStringFromFile(QFile& file, QString& string);

	ORIGIN_PLUGIN_API bool SaveQStringToFile(const QString& sFilename, QString& string);

	ORIGIN_PLUGIN_API bool LoadQStringListFromFile(const QString& sFilename, QStringList& stringList);
	
	// This decodes all characters from Unicode escape-sequences %uXXXX
	ORIGIN_PLUGIN_API bool UnicodeUnescapeQString(QString& sStringToDecode);

	ORIGIN_PLUGIN_API bool EscapeURL(QString& sString);

	//Get a size in bytes formatted in the most appropriate unit
	ORIGIN_PLUGIN_API QString SizeString( qint64 length );

	ORIGIN_PLUGIN_API void StripReservedCharactersFromFilename(QString& sFilename);		// note that this should NOT include a nested path since slashes would be stripped as well........ this can be a file or folder name.
	ORIGIN_PLUGIN_API void StripReservedCharactersFromFilename(QString& sFilename);

	// useful values
	static const uchar MAX_CHAR = 126;
	static const uchar MIN_CHAR = 32;

	///
	/// returns true if the char is not valid ASCII and false otherwise
	inline bool isAscii(const char chr)
	{
		return chr <= MAX_CHAR && chr >= MIN_CHAR;
	}

	/// \brief Strips characters that might lead to confusion like the ones that the unicode
	/// converter left as '?' characters.
	///
	/// \param qstr The string to operate on.
	///
	ORIGIN_PLUGIN_API QString stripChars(const QString& qstr);
	
	/// \brief Strips multibyte characters from passed parameters and returns the modified string.
	///
	/// \param bfr The string to operate on.
	///
	ORIGIN_PLUGIN_API QString stripMultiByte(const QString& bfr);

	struct QStringCompareInsensitive
	{
		bool operator() (const QString& s1, const QString& s2) const
		{
			return s1.compare(s2, Qt::CaseInsensitive) < 0;
		}
	};

    /// \brief returns true if the text contains any html tags and false otherwise
    ///
    /// Note: This is not an HTML parser or validator. 
    /// 
    /// For paired HTML tags, we match if we find an open and a closing tag
    /// For example, the following text strings will return TRUE.
    /// <a href='link.html'>This is a hyperlink</a>
    /// <head>a bunch of stuff</head>
    /// <b>bold</b>
    /// <i>italic</i>
    /// The following text strings will return FALSE.
    /// <i>no match</b>
    /// <head>
    ///
    /// For unpaired tags, we match if we find any properly formatted tag.
    /// For example, the following text strings will return TRUE
    /// <br>
    /// <br/>
    /// <img src='test.png'>
    /// The following strings will return FALSE
    /// <br123>
    /// <img
    ///
    /// \param text The string to test
    ///
    ORIGIN_PLUGIN_API bool containsHtmlTag(const QString& text);

}

} // namespace Downloader
} // namespace Origin

#endif
