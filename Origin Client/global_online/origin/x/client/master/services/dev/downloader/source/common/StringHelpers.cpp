// StringHelpers.cpp
// purpose: Helper functions for certain string operations

#include "services/downloader/StringHelpers.h"
#include "services/debug/DebugService.h"
#include "services/downloader/File.h"
#include "services/log/LogService.h"

#include <QCryptographicHash>
#include <QRegExp>
#include <QTextCodec>

#include <algorithm>
#include <limits>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

namespace Origin
{
namespace Downloader
{

// determines whether the text buffer is encoded in utf-16 by checking for an appropriate BOM(signature)
bool StringHelpers::detectUnicode(unsigned char* buff, qint64 buffSize)
{
    if (buffSize < 2)
        return false;
    if (buff[0] == (unsigned char)0xff && buff[1] == (unsigned char)0xfe)
        return true;
    if (buff[0] == (unsigned char)0xfe && buff[1] == (unsigned char)0xff)
        return true;
    return false;
}

bool StringHelpers::LoadQStringFromFile(const QString& sFilename, QString& string)
{
	QFile file(sFilename);
    if (!(file.exists() && file.open(QIODevice::ReadOnly)))
    {
        ORIGIN_LOG_DEBUG << "LoadQStringFromFile " << sFilename << " failed\n";

		return false;
    }

	return LoadQStringFromFile(file, string);
}

bool StringHelpers::LoadQStringFromFile(QFile& file, QString& string)
{
    // Read the file into a buffer
    qint64 nFileSize = file.size();
    if (nFileSize == -1)
        return false;

    char* pBuf = new char[nFileSize];
    file.read(pBuf, nFileSize);

    // If we've read in a Unicode file, set the string
    if (detectUnicode((unsigned char*)pBuf, nFileSize))
    {
        unsigned long nStringLength = nFileSize / sizeof(quint16);
        string = QString::fromUtf16(reinterpret_cast<ushort*>(pBuf), nStringLength);
    }
    else
        string = QString::fromUtf8(pBuf, nFileSize);
    delete[] pBuf;
    file.close();

    return true;
}

bool StringHelpers::SaveQStringToFile(const QString& sFilename, QString& string)
{
    // Create the proper directory if it does not yet exist.
    QDir path(QFileInfo(sFilename).absolutePath());
    if (!path.exists())
    {
        path.mkpath(path.path());
    }
    
    // Open the file.
    QFile file(sFilename);
    if (!file.open(QIODevice::ReadWrite))
    {
        ORIGIN_LOG_DEBUG << "SaveQStringToFile " << sFilename << " failed\n";

        return false;
    }

    // Unicode UTF-16LittleEndian signature
    unsigned char sig[2];
    sig[0] = (unsigned char) 0xFF;
    sig[1] = (unsigned char) 0xFE;
    file.write((const char*)sig, 2);

    file.write(reinterpret_cast<const char*>(string.utf16()), string.size()*sizeof(QChar));
    file.close();

    return true;
}

// Purpose: Get a human readable size from a byte count
QString StringHelpers::SizeString( qint64 length )
{
	QString result;
	double d = static_cast<double>(length);
	if ( length >= (1024 * 1024 * 1024) )
	{
		result = QString("%1 GB").arg(d / 1024 / 1024 / 1024, 0, 'f', 2);
	}
	else if ( length >= (1024 * 1024) )
	{
		result = QString("%1 MB").arg(d / 1024 / 1024, 0, 'f', 2);
	}
	else if ( length >= 1024 )
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
		}
	}

	sString = sTemp;
	return true;
}

// Converts a 4 character string that represents a hexadecimal number to its decimal value
inline qint64 StringToHex(QString& sHex)
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

// This decodes all characters from Unicode escape-sequences %uXXXX
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
            int nValue = StringToHex(sValue);

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
//   (c) (copyright)
//	 tm (Trademark)
//	 R (Reserved)
void StringHelpers::StripReservedCharactersFromFilename(QString& sFilename)
{
    #define COPYRIGHT_UTF8 (ushort)0x00A9
    #define TRADEMARK_UTF8 (ushort)0x2122
    #define RESERVED_UTF8 (ushort)0x00AE
    static QRegExp reservedChars(QString::fromLatin1("[<>:\\\"/\\\\\\|\\?\\*") + QChar(COPYRIGHT_UTF8) + QChar(TRADEMARK_UTF8) + QChar(RESERVED_UTF8) + QString::fromLatin1("]"));
    //static QRegExp reservedChars(QString::fromLatin1("[<>:\"/\\\\|?*") + QChar(COPYRIGHT_UTF8) + QChar(TRADEMARK_UTF8) + QChar(RESERVED_UTF8) + QString::fromLatin1("]"));
    sFilename.remove(reservedChars);
}

// enable the following for a quick-n-dirty unit test for StringHelpers::StripReservedCharactersFromFilename
#define ENABLE_UNITTEST_STRIPRESERVEDCHARACTERSFROMFILENAME 0

#if ENABLE_UNITTEST_STRIPRESERVEDCHARACTERSFROMFILENAME

class TestStripReservedCharactersFromFilename
{
public:

    TestStripReservedCharactersFromFilename()
    {
        wchar_t filenames[13][16] = {
            L"Filename",
            L"Filename<",
            L"Filename>",
            L"Filename:",
            L"Filename\"",
            L"Filename/",
            L"Filename\\",
            L"Filename|",
            L"Filename?",
            L"Filename*",
            L"Filename\u00A9",
            L"Filename\u2122",
            L"Filename\u00AE"
        };
        const size_t filenamesCount = sizeof(filenames)/sizeof(filenames[0]);
        const QString verification("Filename");
        for (int i = 0; i < filenamesCount; ++i)
        {
            QString testName(QString::fromWCharArray(filenames[i]));
            StringHelpers::StripReservedCharactersFromFilename(testName);
            if (testName != verification)
            {
                ORIGIN_LOG_DEBUG << "mismatch: " << QString(QString::fromWCharArray(filenames[i])) << " => " << QString(testName);
            }
        }
    }
};

TestStripReservedCharactersFromFilename testStripping;

#endif

/// \fn StringHelpers::stripChars(const QString& qstr)
/// \brief Strip characters that might lead to confusion like the ones that the unicode converter left as '?' characters.
///
/// \param qstr The string to operate on
///
QString StringHelpers::stripChars(const QString& qstr)
{
    std::string str = qstr.toLocal8Bit().constData();
    QString myRes;
    for (int i = 0; i < (int)str.length(); i++)
    {
        char c = str.at(i);
        if (isAscii(c) && c != '?')
            myRes += c;
    }

    // Remove duplicate spaces
    myRes.replace(QRegExp("[ ]+"), " ");

    return myRes;
}

/// \fn StringHelpers::stripMultiByte(const QString& bfr)
/// \brief Strips multibyte characters from passed parameter returns modified string.
///
/// \param bfr The string to operate on.
///
QString StringHelpers::stripMultiByte(const QString& bfr)
{
	// Obtain the codec for locale then convert from Unicode

	QString res = QTextCodec::codecForName("latin1")->fromUnicode(bfr);

	// Remove garbage, control chars and unknowns
	res = stripChars(res);
	return res;
}

/// \fn StringHelpers::containsHtmlTag(const QString& text) 
/// \brief returns true if the text contains any html tags and false otherwise
///
/// Note: This is not an HTML parser or validator. 
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
bool StringHelpers::containsHtmlTag(const QString& text) 
{       
    static QRegExp htmlUnpairedTagRegExp("<(br|basefont|hr|input|source|frame|param|area|meta|col|link|option|base|img|wbr)\\b.*>", Qt::CaseInsensitive);
    static QRegExp htmlPairedTagRegExp("<(a|abbr|acronym|address|applet|article|aside|audio|b|bdi|bdo|big|blockquote|body|button|canvas|caption|center|cite|code|colgroup|command|datalist|dd|del|details|dfn|dialog|dir|div|dl|dt|em|embed|fieldset|figcaption|figure|font|footer|form|frameset|head|header|hgroup|h1|h2|h3|h4|h5|h6|html|i|iframe|ins|kbd|keygen|label|legend|li|map|mark|menu|meter|nav|noframes|noscript|object|ol|optgroup|output|p|pre|progress|q|rp|rt|ruby|s|samp|script|section|select|small|span|strike|strong|style|sub|summary|sup|table|tbody|td|textarea|tfoot|th|thead|time|title|tr|track|tt|u|ul|var|video)\\b.*>.*</\\1>", Qt::CaseInsensitive);
    
    return text.contains(htmlPairedTagRegExp) || text.contains(htmlUnpairedTagRegExp);
}

} // namespace Downloader
} // namespace Origin