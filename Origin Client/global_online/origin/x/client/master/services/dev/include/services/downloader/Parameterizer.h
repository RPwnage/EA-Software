//
// Copyright 2005, Electronic Arts Inc
//
#ifndef PARAMETERIZER_H
#define PARAMETERIZER_H

//#include "services/downloader/stdafx.h"	//in services
#include <map>
//#include <atlcoll.h>
//#include <atltypes.h>

#include <QRect>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Downloader
{
	typedef std::map<QString, QString>	tKeyToValueMap;
	typedef tKeyToValueMap::iterator	tKeyToValueMapIter;

	//////////////////////////////////////////////////////////////////////////////////
	// class Parameterizer
	//
	// This class takes as input a URL string, and maps all parameters in it.
	// Parameters following a "?" will be mapped.
	// Format:  anything?KEY1=VALUE1&KEY2=VALUE2...KEYn=VALUEn
	// Keys are not case sensitive, values are.
	//
	class ORIGIN_PLUGIN_API Parameterizer
	{
	public:

		Parameterizer();
		Parameterizer(const QString& sPackedParameterString);     // clears out any mapped parameters and replaces with new mappings

		bool	Parse(const QString& sPackedParameterString, bool bIgnoreCase=true);       // clears out any mapped parameters and replaces with new mappings
		bool	GetPackedParameterString(QString& sPackedString, QString sKeyPrefix=QString()) const;   // builds a packed string from the existing mapping

		int		GetNumParameters();                         // Number of mapped parameters

		bool 	IsParameterPresent(const QString sKey) const;    // True if it's mapped
		bool 	Clear(QString sKey);                       // Clears a mapping
		bool 	ClearAll();                                // Clears All Mappings

		bool	GetString(QString sKey, QString& sValue, bool bIgnoreCase=true) const;  // Returns the value of a parameter if mapped
		QString GetString(QString sKey) const; // returns empty string if no key was found
		bool	SetString(QString sKey, const QString& sValue, bool bIgnoreCase=true);  // Sets a parameter to a value

		bool	UnsetString(QString sKey, bool bIgnoreCase=true); // remove a parameter if it exists, return true if something was removed

		// Utility Functions
		bool 	GetColor(const QString& sKey, unsigned char& nR, unsigned char& nG, unsigned char& nB);
		bool 	SetColor(const QString& sKey, unsigned char nR, unsigned char nG, unsigned char nB);

		bool	GetRect(const QString& sKey, QRect& area) const;
		QRect	GetRect(const QString& sKey) const; // returns default CRect() if no key was found
		bool	SetRect(const QString& sKey, const QRect& area);

		bool	GetPoint(const QString& sKey, QPoint& point) const;
		QPoint	GetPoint(const QString& sKey) const; // return default CPoint() if no key was found
		bool	SetPoint(const QString& sKey, const QPoint& point);

		bool 	GetLong(const QString& sKey, long& nValue) const;
		long 	GetLong(const QString& sKey) const; // returns 0 if no key was found
		bool 	SetLong(const QString& sKey, const long& nValue);

		bool 	GetULong(const QString& sKey, unsigned long& nValue) const;
		unsigned long 
				GetULong(const QString& sKey) const; // returns 0 if no key was found
		bool 	SetULong(const QString& sKey, const unsigned long& nValue);

		bool	Get64BitInt(const QString& sKey, qint64& nValue) const;
		qint64  Get64BitInt(const QString& sKey) const;	// return 0 if no key was found
		bool	Set64BitInt(const QString& sKey, qint64 nValue);

		bool	GetFloat(const QString& sKey, float& nValue) const;
		float	GetFloat(const QString& sKey) const; // returns 0 if no key was found
		bool	SetFloat(const QString& sKey, const float& nValue);

		bool 	GetBool(const QString& sKey, bool& bValue) const;
		bool 	GetBool(const QString& sKey) const; // returns false if no key was found
		bool 	SetBool(const QString& sKey, const bool bValue);

		bool	GetVoid(const QString& sKey, void** ppVoid) const;
		void*	GetVoid(const QString& sKey) const;
		bool	SetVoid(const QString& sKey, const void* pVoid);

		// gets all params (returns copy of mKeyToValueMap to mapIn)
		void	GetAllParams(tKeyToValueMap &mapIn);
		void	GetPreferencesStartingWith(const QString& sKeyPrefix, tKeyToValueMap& mapIn);

		QString GetAllMappingsDebugString();

	private:
		// helper func to assist parsing integers
		// given a key, gets its value and parses out <count> comma delimited integers
		// so, "0,1,2,3" -> [0,1,2,3]
		//
		// Note: intArray is resized to hold 'count' elements
		// returns true if count were read, and will assert if less than count elements were found.count)
		bool ParseIntegers(const QString &sKey, const int count, QList<int> &intArray ) const;
		bool EncaseValueIfNecessary(QString& sValue) const;

		bool ValidateKey(const QString& sKey) const;
		tKeyToValueMap mKeyToValueMap;
	};
} // namespace Downloader
} // namespace Origin

#endif
