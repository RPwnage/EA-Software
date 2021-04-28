#include "CoreParameterizer.h"
#include "StringHelpers.h"
//#include "platform.h"
#include "WinDefs.h"

#include <QStringList>

//namespace Core
//{
	////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// class Parameterizer

	Parameterizer::Parameterizer() : mKeyToValueMap() {}

	Parameterizer::Parameterizer(const QString& sPackedParameterString): mKeyToValueMap()
	{
		Parse(sPackedParameterString); // ignore boolean return
	}

	bool Parameterizer::ValidateKey(const QString &sKey) const
	{
		// we can't have an '=' in a key... other characters should be ok...
		return (!sKey.contains('='));
	}

	const QString VALUE_START( "<eadvalue:" );
	const QString VALUE_END  ( ":eadvalue>" );
	const long nValueStartLength = VALUE_START.size();
	const long nValueEndLength = VALUE_END.size();

	bool Parameterizer::Parse(const QString& sPackedParameterString, bool bIgnoreCase)
	{
		if( sPackedParameterString.isEmpty() )
		{
			return true;
		}

		int nLength = sPackedParameterString.size();
		int nCurPos = sPackedParameterString.indexOf('?');         // find the ? that begins the parameters

		if( nCurPos == -1 )
		{
			return false;
		}

		nCurPos++; // eat the '?'

		//	int nCurPos = 0;

		while (nCurPos < nLength)
		{
			int delimiterIdx = sPackedParameterString.indexOf('=', nCurPos);
			if (delimiterIdx >= 0)
			{
				QString sKey(sPackedParameterString.mid(nCurPos, delimiterIdx - nCurPos));
				nCurPos = delimiterIdx + 1;
				if (!sKey.isEmpty())
				{
					if (bIgnoreCase)
					{
						sKey = sKey.toLower();
					}
					
					QString sValue;
					if (nCurPos < nLength && sPackedParameterString.at(nCurPos) != '&')
					{
						if (sPackedParameterString.mid(nCurPos, nValueStartLength) == VALUE_START)
						{
							nCurPos += nValueStartLength;	// Advance past the VALUE_START delimiter

							int aValueDelimEndPos = sPackedParameterString.indexOf( VALUE_END, nCurPos );	// Find the VALUE_END

							if( aValueDelimEndPos > 0 )	// If the VALUE_END was found
							{
								// Extract the value as everything between VALUE_START and VALUE_END
								sValue = sPackedParameterString.mid( nCurPos, aValueDelimEndPos-nCurPos );
							}

							// Advance past the VALUE_END and past the '&' (if there is one)
							nCurPos = aValueDelimEndPos + nValueEndLength + 1;	// +1 skips the next '&'
						}
						else
						{
							// Take everything until the next '&' (or everything until the end of the string if no '&')
							delimiterIdx = sPackedParameterString.indexOf('&', nCurPos);
							int n = delimiterIdx < 0 ? -1 : delimiterIdx - nCurPos;
							sValue = sPackedParameterString.mid(nCurPos, n);
							nCurPos = delimiterIdx < 0 ? nLength : delimiterIdx + 1;
						}
					}
					else
					{
						++nCurPos;
					}

					mKeyToValueMap[sKey] = sValue;
				}
			}
		}

		return true;
	}

	// NOTE: the packed param string is suitable to be added to another Parameterizer object as a string entry (since all string entries are encoded)
	bool Parameterizer::GetPackedParameterString(QString& sPackedString, QString sKeyPrefix) const
	{
		sPackedString = "?";
		for (tKeyToValueMap::const_iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end();)
		{
			QString sKey((*it).first);
			QString sValue((*it).second);

			EncaseValueIfNecessary(sValue);		// adds delimiters around the value if it contains any characters that could interfere with parsing

			sPackedString.append(QString("%1%2=%3").arg(sKeyPrefix).arg(sKey).arg(sValue));

			it++;

			if (it != mKeyToValueMap.end())     // if there are more parameters, add a '&'
				sPackedString.append("&");

		}

		return true;
	}

	bool Parameterizer::IsParameterPresent(QString sKey) const
	{
		CORE_ASSERT(ValidateKey(sKey));

		tKeyToValueMap::const_iterator it = mKeyToValueMap.find(sKey.toLower());

		return it != mKeyToValueMap.end();
	}

	// helper func to assist parsing integers
	// given a key, gets its value and parses out <count> comma delimited integers
	// so, "0,1,2,3" -> [0,1,2,3]
	//
	// Note: intArray is resized to hold 'count' elements
	// returns true if count were read, and will assert if less than count elements were found.
	bool Parameterizer::ParseIntegers(const QString &sKey, const int count, QList<int> &intArray) const
	{
		QString sValue;
		if( !GetString(sKey, sValue) )
			return false;

		intArray.clear();

		QStringList tokens(sValue.split(','));
		if (tokens.size() < count)
		{
			// corrupt entry?  Was expecting more integers...
			CORE_ASSERT(false);
			return false;
		}

		bool success = true;
		for (int i = 0; i < tokens.size(); ++i)
		{
			bool ok;
			intArray[i] = tokens[i].toInt(&ok);
			success &= ok;
		}

		return success;
	}


	// string values are urlencoded/decoded
	// this allows us to add a packed param string into another parameter object (used by notification callbacks)
	// it also allows us to move path info safely about (ex: "program files\Earth & Beyoned\Launcher.exe")
	// if it becomes a performance issue, we can fall back to having SetString/GetString and SetEncodedString/GetEncodedString methods
	bool Parameterizer::GetString(QString sKey, QString& sValue, bool bIgnoreCase) const
	{
		CORE_ASSERT(ValidateKey(sKey));

		if (bIgnoreCase)
		{
			sKey = sKey.toLower();
		}

		// Must use map::find.  Can't just use [] operator as (by nature of the map template) a new mapping is created if it does not already exist.
		tKeyToValueMap::const_iterator it = mKeyToValueMap.find(sKey); 
		if (it != mKeyToValueMap.end())
		{
			// url unescape the value
			int iBufferSize = ((*it).second.size()+1) * sizeof(wchar_t);
			wchar_t* buffer = new wchar_t[iBufferSize];
			memset(buffer, 0, iBufferSize);
			(*it).second.toWCharArray(buffer);

			if( UrlUnescapeInPlace(buffer, 0) != S_OK )
			{
				delete[] buffer;
				CORE_ASSERT(false);
				return false;
			}

			sValue = QString::fromWCharArray(buffer);
			delete[] buffer;
			return true;
		}

		return false;
	}


	// Wrapper for ::Get, returns "" if no key existed
	QString Parameterizer::GetString(QString sKey) const
	{
		QString sValue;
		GetString(sKey, sValue);
		return sValue;
	}

	// string values are urlencoded/decoded
	// this allows us to add a packed param string into another parameter object (used by notification callbacks)
	// it also allows us to move path info safely about (ex: "program files\Earth & Beyoned\Launcher.exe")
	// if it becomes a performance issue, we can fall back to having SetString/GetString and SetEncodedString/GetEncodedString methods
	bool Parameterizer::SetString(QString sKey, const QString& sValue, bool bIgnoreCase)
	{
		CORE_ASSERT(ValidateKey(sKey));

		if (sValue.isEmpty())
		{
			mKeyToValueMap[sKey].clear();
			return true;
		}

		QString value(sValue);
		if (StringHelpers::EscapeURL(value))
		{
			if (bIgnoreCase)
			{
				sKey = sKey.toLower();
			}

			mKeyToValueMap[sKey] = value;
			return true;
		}

		return false;
	}

	// remove a parameter if it exists, returns true if something was removed
	bool Parameterizer::UnsetString(QString sKey, bool bIgnoreCase)
	{
		if (bIgnoreCase)
			sKey = sKey.toLower();
		return mKeyToValueMap.erase(sKey) > 0 ? true : false; // erase returns # elements erased, ternary form to shutup stupid compiler warning
	}


	bool Parameterizer::GetColor(const QString& sKey, unsigned char& nR, unsigned char& nG, unsigned char& nB)
	{
		QList<int> intArray;
		if( !ParseIntegers(sKey, 3, intArray) )
		{
			return false;
		}

		// Format must be r,g,b 
		nR = intArray[0];
		nG = intArray[1];
		nB = intArray[2];

		return true;
	}

	bool Parameterizer::SetColor(const QString& sKey, unsigned char nR, unsigned char nG, unsigned char nB)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, QString("%1,%2,%3").arg(nR).arg(nG).arg(nB));
	}


	bool Parameterizer::GetRect(const QString& sKey, QRect& area) const
	{
		QList<int> intArray;
		if( !ParseIntegers(sKey, 4, intArray) )
		{
			return false;
		}

		// format must be L,T,R,B
		area.setLeft(intArray[0]);
		area.setTop(intArray[1]);
		area.setRight(intArray[2]);
		area.setBottom(intArray[3]);

		return true;
	}

	// if key not found, returns QRect()
	QRect Parameterizer::GetRect(const QString& sKey) const
	{
		QRect result;
		GetRect(sKey, result);
		return result;
	}

	bool Parameterizer::SetRect(const QString& sKey, const QRect& area)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, QString("%1,%2,%3,%4").arg(area.left()).arg(area.top()).arg(area.right()).arg(area.bottom()));
	}

	bool Parameterizer::GetPoint(const QString& sKey, QPoint& point) const
	{
		QList<int> intArray;
		if( !ParseIntegers(sKey, 2, intArray) )
		{
			return false;
		}

		// Format must be x,y
		point.setX(intArray[0]);
		point.setY(intArray[1]);

		return true;
	}

	// if key not found, returns CPoint()
	QPoint Parameterizer::GetPoint(const QString& sKey) const
	{
		QPoint result;
		GetPoint(sKey, result);
		return result;
	}

	bool Parameterizer::SetPoint(const QString& sKey, const QPoint& point)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, QString("%1,%2").arg(point.x()).arg(point.y()));
	}

	bool Parameterizer::GetLong(const QString& sKey, long& nValue) const
	{
		QString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.isEmpty())
		{
			CORE_ASSERT(false);
			// bad number retrieval?
			return false;
		}

		bool ok;
		nValue = sNumberString.toLong(&ok);

		return ok;
	}

	// returns 0 if no key was found
	long Parameterizer::GetLong(const QString& sKey) const
	{
		long result = 0;
		GetLong(sKey, result);
		return result;
	}

	bool Parameterizer::SetLong(const QString& sKey, const long& nValue)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, QString::number(nValue));
	}

	bool Parameterizer::GetULong(const QString& sKey, unsigned long& nValue) const
	{
		QString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.isEmpty())
		{
			// bad number retrieval?
			return false;
		}

		bool ok;
		nValue = sNumberString.toLong(&ok);

		return ok;
	}

	// returns 0 if no key was found
	unsigned long Parameterizer::GetULong(const QString& sKey) const
	{
		unsigned long result = 0;
		GetULong(sKey, result);
		return result;
	}

	bool Parameterizer::SetULong(const QString& sKey, const unsigned long& nValue)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, QString::number(nValue));
	}

	bool Parameterizer::Get64BitInt(const QString& sKey, int64_t& nValue) const
	{
		QString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.isEmpty())
		{
			CORE_ASSERT(false);
			// bad number retrieval?
			return false;
		}

		bool ok;
		nValue = sNumberString.toLongLong(&ok);

		return ok;
	}

	int64_t Parameterizer::Get64BitInt(const QString& sKey) const
	{
		int64_t result = 0;
		Get64BitInt(sKey, result);
		return result;
	}

	bool Parameterizer::Set64BitInt(const QString& sKey, int64_t nValue)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, QString::number(nValue));
	}



	// GetBool follows c-style bool rules (non-zero == true)
	bool Parameterizer::GetBool(const QString& sKey, bool& bValue) const
	{
		long num;
		if( !GetLong(sKey, num ) )
			return false;

		bValue = (num != 0);
		return true;
	}

	// GetBool follows c-style bool rules (non-zero == true)
	// will return false if no key was found
	bool Parameterizer::GetBool(const QString& sKey) const
	{
		bool bValue = false;
		GetBool(sKey, bValue);
		return bValue;
	}

	// SetBool will always serialize to either a 1 or a 0...
	bool Parameterizer::SetBool(const QString& sKey, const bool bValue)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, bValue ? "1" : "0" );
	}

	bool Parameterizer::GetVoid(const QString& sKey, void** ppVoid) const
	{
		CORE_ASSERT(ppVoid);

		QString sVoidString;
		if (!GetString(sKey, sVoidString))
			return false;

		if (sVoidString.isEmpty())
		{
			CORE_ASSERT(false);
			return false;
		}

		bool ok;
		*ppVoid = (void*)(sVoidString.toLongLong(&ok));

		return ok;
	}

	void* Parameterizer::GetVoid(const QString& sKey) const
	{
		void* pVoid = NULL;
		GetVoid(sKey, &pVoid);

		return pVoid;
	}

	bool Parameterizer::SetVoid(const QString& sKey, const void* pVoid)
	{
		return SetString(sKey, QString::number((qlonglong)pVoid));
	}

	bool Parameterizer::GetFloat(const QString& sKey, float& nValue) const
	{
		QString sNumberString;
		if (!GetString(sKey, sNumberString))
			return false;

		if (sNumberString.isEmpty())
		{
			CORE_ASSERT(false);
			// bad number retrieval?
			return false;
		}

		bool ok;
		nValue = sNumberString.toFloat(&ok);

		return ok;
	}

	// if no key found, returns 0
	float Parameterizer::GetFloat(const QString& sKey) const
	{
		float result = 0;
		GetFloat(sKey, result);
		return result;
	}

	bool Parameterizer::SetFloat(const QString& sKey, const float& nValue)
	{
		CORE_ASSERT(ValidateKey(sKey));

		return SetString(sKey, QString::number(nValue));
	}

	bool Parameterizer::Clear(QString sKey)
	{
		CORE_ASSERT(ValidateKey(sKey));

		tKeyToValueMap::iterator it = mKeyToValueMap.find(sKey.toLower());

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

	int Parameterizer::ClearStartingWith(const QString& sKeyPrefix)
	{
		int iCnt = 0;
		for (tKeyToValueMap::iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end(); )
		{
			const QString &sKey = (*it).first;

			if (!sKey.left(sKeyPrefix.size()).compare(sKeyPrefix, Qt::CaseInsensitive))
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
			const QString &sKey = (*it).first;
			QString sValue;
			GetString(sKey, sValue, false /*bIgnoreCase*/);
			// add key/value pair to mapIn
			mapIn[sKey] = sValue;
		}
	}
	void Parameterizer::GetPreferencesStartingWith(const QString& sKeyPrefix, tKeyToValueMap& mapIn)
	{
		mapIn.clear();

		for (tKeyToValueMap::iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end(); it++)
		{
			const QString &sKey = (*it).first;

			if (!sKey.left(sKeyPrefix.size()).compare(sKeyPrefix, Qt::CaseInsensitive))
			{
				QString sValue;
				GetString(sKey, sValue, false /*bIgnoreCase*/);
				// add key/value pair to mapIn
				mapIn[sKey] = sValue;
			}
		}
	}

	QString Parameterizer::GetAllMappingsDebugString()
	{
		QString sMappings("<table class='helpTable' ><tr><center><td><b>Help - Dump Parameterizer Mappings</b></td></center></tr>");
		for (tKeyToValueMap::iterator it = mKeyToValueMap.begin(); it != mKeyToValueMap.end(); it++)
		{
			const QString &sKey = (*it).first;
			QString sValue;
			GetString(sKey, sValue);

			// gotta html escape
			sValue.replace("&", "&amp;");
			sValue.replace("<", "&lt;");
			sValue.replace(">", "&gt;");

			sMappings.append(QString("<tr><td>%1</td><td>=</td><td>%2</td></tr>").arg(sKey).arg(sValue));
		}
		sMappings.append("</table>");

		// gotta printf escape, since some strings have printf formatting characters in them
		sMappings.replace("%", "%%");
		sMappings.replace("\\", "\\\\");

		return sMappings;
	}

	bool Parameterizer::EncaseValueIfNecessary( QString& sValue ) const
	{
		// If the string contains any of the following: = & SPACE
		// encase it in value delimiters

		if( sValue.indexOf( QRegExp("[=& ]") ) >= 0 )
		{
			sValue = VALUE_START + sValue + VALUE_END;
			return true;
		}

		return false;
	}
//}
