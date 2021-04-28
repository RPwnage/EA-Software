//
// Copyright 2005, Electronic Arts Inc
//
#ifndef COREPARAMETERIZER_H
#define COREPARAMETERIZER_H

#include <map>
#include <atlcoll.h>
#include <atltypes.h>

namespace Core
{
	typedef std::map<CString, CString>	tKeyToValueMap;
	typedef tKeyToValueMap::iterator	tKeyToValueMapIter;

	//////////////////////////////////////////////////////////////////////////////////
	// class Parameterizer
	//
	// This class takes as input a URL string, and maps all parameters in it.
	// Parameters following a "?" will be mapped.
	// Format:  anything?KEY1=VALUE1&KEY2=VALUE2...KEYn=VALUEn
	// Keys are not case sensitive, values are.
	//
	class Parameterizer
	{
	public:

		Parameterizer();
		Parameterizer(const CString& sPackedParameterString);     // clears out any mapped parameters and replaces with new mappings

		bool	Parse(const CString& sPackedParameterString, bool bIgnoreCase=true);       // clears out any mapped parameters and replaces with new mappings
		bool	GetPackedParameterString(CString& sPackedString, CString sKeyPrefix=CString("")) const;   // builds a packed string from the existing mapping

		int		GetNumParameters();                         // Number of mapped parameters

		bool 	IsParameterPresent(const CString sKey) const;    // True if it's mapped
		bool 	Clear(CString sKey);                       // Clears a mapping
		bool 	ClearAll();                                // Clears All Mappings
		int		ClearStartingWith(const CString& sKeyPrefix);

		bool	GetString(CString sKey, CString& sValue, bool bIgnoreCase=true) const;  // Returns the value of a parameter if mapped
		CString GetString(CString sKey) const; // returns empty string if no key was found
		bool	SetString(CString sKey, const CString& sValue, bool bIgnoreCase=true);  // Sets a parameter to a value

		bool	UnsetString(CString sKey, bool bIgnoreCase=true); // remove a parameter if it exists, return true if something was removed

		// Utility Functions
		bool 	GetColor(const CString& sKey, unsigned char& nR, unsigned char& nG, unsigned char& nB);
		bool 	SetColor(const CString& sKey, unsigned char nR, unsigned char nG, unsigned char nB);

		bool	GetRect(const CString& sKey, CRect& area) const;
		CRect	GetRect(const CString& sKey) const; // returns default CRect() if no key was found
		bool	SetRect(const CString& sKey, const CRect& area);

		bool	GetPoint(const CString& sKey, CPoint& point) const;
		CPoint	GetPoint(const CString& sKey) const; // return default CPoint() if no key was found
		bool	SetPoint(const CString& sKey, const CPoint& point);

		bool 	GetLong(const CString& sKey, long& nValue) const;
		long 	GetLong(const CString& sKey) const; // returns 0 if no key was found
		bool 	SetLong(const CString& sKey, const long& nValue);

		bool 	GetULong(const CString& sKey, unsigned long& nValue) const;
		unsigned long 	
			GetULong(const CString& sKey) const; // returns 0 if no key was found
		bool 	SetULong(const CString& sKey, const unsigned long& nValue);

		bool	Get64BitInt(const CString& sKey, __int64& nValue) const;
		__int64 Get64BitInt(const CString& sKey) const;	// return 0 if no key was found
		bool	Set64BitInt(const CString& sKey, __int64 nValue);

		bool	GetFloat(const CString& sKey, float& nValue) const;
		float	GetFloat(const CString& sKey) const; // returns 0 if no key was found
		bool	SetFloat(const CString& sKey, const float& nValue);

		bool 	GetBool(const CString& sKey, bool& bValue) const;
		bool 	GetBool(const CString& sKey) const; // returns false if no key was found
		bool 	SetBool(const CString& sKey, const bool bValue);

		bool	GetVoid(const CString& sKey, void** ppVoid) const;
		void*	GetVoid(const CString& sKey) const;
		bool	SetVoid(const CString& sKey, const void* pVoid);

		// gets all params (returns copy of mKeyToValueMap to mapIn)
		void	GetAllParams(tKeyToValueMap &mapIn); 
		void	GetPreferencesStartingWith(const CString& sKeyPrefix, tKeyToValueMap& mapIn);

		CString GetAllMappingsDebugString(); 

	private:
		// helper func to assist parsing integers
		// given a key, gets its value and parses out <count> comma delimited integers
		// so, "0,1,2,3" -> [0,1,2,3]
		//
		// Note: intArray is resized to hold 'count' elements
		// returns true if count were read, and will assert if less than count elements were found.count)
		bool ParseIntegers(const CString &sKey, const int count, CSimpleArray<int> &intArray ) const;
		bool EncaseValueIfNecessary(CString& sValue) const;

		bool ValidateKey(const CString& sKey) const;
		tKeyToValueMap mKeyToValueMap;
	};
}
#endif
