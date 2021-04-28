///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Locale.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Locale.h>
#include <EAPatchClient/Debug.h>
#include <EAPatchClient/URL.h>
#include <EAStdC/EAString.h>


namespace EA
{
namespace Patch
{


/////////////////////////////////////////////////////////////////////////////
// SetDefaultLocale
/////////////////////////////////////////////////////////////////////////////

String gsLocale;
String gsSystemLocale;


EAPATCHCLIENT_API void SetDefaultLocale(const char8_t* pLocale)
{
    gsLocale = pLocale;
}


EAPATCHCLIENT_API const char8_t* GetDefaultLocale()
{
    return gsLocale.c_str();
}


EAPATCHCLIENT_API const char8_t* GetSystemLocale()
{
    if(gsSystemLocale.empty())
    {
        // To do.
        EAPATCH_FAIL_MESSAGE("GetSystemLocale not yet implemented.");
    }

    return gsSystemLocale.c_str();
}



/////////////////////////////////////////////////////////////////////////////
// LocaleMatch
/////////////////////////////////////////////////////////////////////////////


// LocaleMatch
//
bool LocaleMatch(const char8_t* pLocale1, const char8_t* pLocale2)
{
    using namespace EA::StdC;

    EAPATCH_ASSERT(pLocale1 && pLocale2);
    
    char8_t  locale1Language[3];
    char8_t  locale1Country[3];
    char8_t  locale2Language[3];
    char8_t  locale2Country[3];
    
    const char8_t* pSource = pLocale1;
    char8_t*       pDest   = locale1Language;

    // To consider: We can make this function more efficient by delaying the parsing of 
    // the country part of the two strings until after we know the language part matches.

    // Read locale1Language
    *pDest = *pSource++;
    if(*pDest++ != '*')
        *pDest++ = *pSource++;
    *pDest = 0;
    EAPATCH_ASSERT(*pSource == '_');
    pSource++;

    // Read locale1Country
    pDest = locale1Country;
    *pDest = *pSource++;
    if(*pDest++ != '*')
        *pDest++ = *pSource++;
    *pDest = 0;

    // Read locale2Language
    pSource = pLocale2;
    pDest   = locale2Language;

    *pDest = *pSource++;
    if(*pDest++ != '*')
        *pDest++ = *pSource++;
    *pDest = 0;
    EAPATCH_ASSERT(*pSource == '_');
    pSource++;

    // Read locale2Country
    pDest = locale2Country;
    *pDest = *pSource++;
    if(*pDest++ != '*')
        *pDest++ = *pSource++;
    *pDest = 0;

    // Compare language and locales. 
    if((locale1Language[0] == '*') || (locale2Language[0] == '*') || (Stricmp(locale1Language, locale2Language) == 0)) 
    {
        if((locale1Country[0] == '*') || (locale2Country[0] == '*') || (Stricmp(locale1Country, locale2Country) == 0)) 
            return true;
    }

    return false;
}


// LocaleMatch
//
// To do: We need to have a function that finds the best locale match between
// a match string and a locale set. For example, we may have two URLs to choose
// from, one that' specific for French and one that's a catch-all for all other 
// languages. We want to pick the French one if running in French instead of 
// picking the catch-all one, though the catch-all does match.
//
bool LocaleMatch(const StringArray& localeArray1, const StringArray& localeArray2)
{
    if(!localeArray1.empty() && !localeArray2.empty()) // If either array is empty then it's assumed to match all locales.
    {
        // To consider: We can improve the performance of this n^2 comparison by pre-parsing the 
        // language and country portions of each string. As it stands now, each call to LocaleMatch
        // within the second loop below re-parses the language and country information.

        for(size_t i = 0, iEnd = localeArray1.size(); i < iEnd; i++)
        {
            for(size_t j = 0, jEnd = localeArray2.size(); j < jEnd; j++)
            {
                const char8_t* pLocale1 = localeArray1[i].c_str();
                const char8_t* pLocale2 = localeArray2[i].c_str();

                if(LocaleMatch(pLocale1, pLocale2))
                    return true;
            }
        }

        return false;
    }

    return true; 
}

bool LocaleMatch(const StringArray& localeArray1, const char8_t* pLocale2)
{
    if(!localeArray1.empty()) // If the array is empty then it's assumed to match all locales.
    {
        // To consider: apply optimizations discussed above in the other LocaleMatch functions.
        for(size_t i = 0, iEnd = localeArray1.size(); i < iEnd; i++)
        {
            const char8_t* pLocale1 = localeArray1[i].c_str();

            if(LocaleMatch(pLocale1, pLocale2))
                return true;
        }

        return false;
    }

    return true; 
}




/////////////////////////////////////////////////////////////////////////////
// LocalizedString
/////////////////////////////////////////////////////////////////////////////

// StricmpFunctor
// This allows us to have an eastl map of string objects which are looked up
// in a case-insensitive way with char8_t* C string pointers. 
template <typename StringA, typename StringB>
struct StricmpFunctor : public eastl::binary_function<StringA, StringB, bool>
{
    bool operator()(const StringA& a, const StringB& b) const
        { return (a.comparei(b) < 0); } // Return true if a is less than b.

    bool operator()(const StringB& b, const StringA& a) const
        { return (a.comparei(b) > 0); } // Return true if b is less than a.
};



LocalizedString::LocalizedString()
  : mLocaleStringMap()
{
}


LocalizedString::LocalizedString(const LocalizedString& b)
{
    mLocaleStringMap = b.mLocaleStringMap;
}


LocalizedString::~LocalizedString()
{
}


LocalizedString& LocalizedString::operator=(const LocalizedString& b)
{
    mLocaleStringMap = b.mLocaleStringMap;
    return *this;
}


bool operator==(const LocalizedString& a, const LocalizedString& b)
{
    return a.GetLocaleStringMap() == b.GetLocaleStringMap();
}


const char8_t* LocalizedString::GetString(const char8_t* pLocale) const
{
    if(!pLocale || !*pLocale)
        pLocale = EA::Patch::GetDefaultLocale();

    if(pLocale)
    {
        // First see if there is a perfect match.
        LocaleStringMap::const_iterator it = mLocaleStringMap.find_as(pLocale, StricmpFunctor<StringSmall, const char8_t*>());

        if(it != mLocaleStringMap.end())
        {
            const String& s = it->second;
            return s.c_str();
        }

        // Now see if there is a wildcard match.
        for(it = mLocaleStringMap.begin(); it != mLocaleStringMap.end(); ++it)
        {
            const LocaleStringMap::value_type& value = *it;

            if(LocaleMatch(value.first.c_str(), pLocale))
            {
                const String& s = it->second;
                return s.c_str();
            }
        }
    }

    return NULL;
}



void LocalizedString::AddString(const char8_t* pLocale, const char8_t* pString)
{
    if(!pLocale)
        pLocale = EA::Patch::GetDefaultLocale();

    LocaleStringMap::iterator it;

    if(pLocale)
    {
        if(pString)
        {
            // The following will overwrite any existing version.
            const StringSmall sLocale(pLocale);

            mLocaleStringMap[sLocale] = pString;
        }
        else
        {
            // Erase the string if present.
            it = mLocaleStringMap.find_as(pLocale, StricmpFunctor<StringSmall, const char8_t*>());

            if(it != mLocaleStringMap.end())
                mLocaleStringMap.erase(it);
        }
    }
}


size_t LocalizedString::Count() const
{
    return (size_t)mLocaleStringMap.size();
}


void LocalizedString::Clear()
{
    mLocaleStringMap.clear();
}


void LocalizedString::URLEncodeStrings()
{
    for(LocaleStringMap::iterator it = mLocaleStringMap.begin(); it != mLocaleStringMap.end(); ++it)
    {
        LocaleStringMap::value_type& value = *it;
        String& sValue = value.second;

        // To do: Implement a standalone StringLooksLikeURL function.
        if((EA::StdC::Stristr(sValue.c_str(), "http://") == sValue.c_str()) ||
           (EA::StdC::Stristr(sValue.c_str(), "https://") == sValue.c_str()))
        {
            URL::EncodeSpacesInURL(sValue.c_str(), sValue);
        }
    }
}


const LocalizedString::LocaleStringMap& LocalizedString::GetLocaleStringMap() const
{
    return mLocaleStringMap;
}


const char8_t* LocalizedString::GetEnglishValue() const
{
    const char8_t* pValue = GetString("en_us");

    if(!pValue)
        pValue = GetString("en_*");

    if(!pValue && !mLocaleStringMap.empty())
        pValue = mLocaleStringMap.begin()->second.c_str();

    if(!pValue)
        pValue = "";

    return pValue;
}


LocalizedString::LocaleStringMap& LocalizedString::GetLocaleStringMap()
{
    return mLocaleStringMap;
}




} // namespace Patch
} // namespace EA








