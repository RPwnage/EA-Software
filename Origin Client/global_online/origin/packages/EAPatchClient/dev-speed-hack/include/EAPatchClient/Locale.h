///////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Locale.h
//
// Copyright (c) Electronic Arts. All Rights Reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAPATCHCLIENT_LOCALE_H
#define EAPATCHCLIENT_LOCALE_H


#include <EAPatchClient/Config.h>
#include <EAPatchClient/Base.h>
#include <EASTL/fixed_map.h>
#include <eathread/eathread_futex.h>


#ifdef _MSC_VER
    #pragma warning(push)           // We have to be careful about disabling this warning. Sometimes the warning is meaningful; sometimes it isn't.
    #pragma warning(disable: 4251)  // class (some template) needs to have dll-interface to be used by clients.
    #pragma warning(disable: 4275)  // non dll-interface class used as base for dll-interface class.
#endif


namespace EA
{
namespace Patch
{

///////////////////////////////////////////////////////////////////////////////
// Locale string format
//
// The format of locales is LL_CC (2 digit language _ two digit country),
// with letters being case-insensitive. We use '_' instead of '-' because '-' is
// often a reserved character in encoding systems.
///////////////////////////////////////////////////////////////////////////////

/// kLocaleAny
///
/// This is a wildcard locale match string which means any language in any location. 
///
#define kLocaleAny "*_*"


/// SetDefaultLocale
///
/// Sets the default locale for EAPatchClient. The input string address
/// is copied to a local buffer and doesn't need to be persistant beyond
/// the call to SetDefaultLocale. If called with NULL then the default 
/// locale is set to the current application locale. Beware that some
/// platforms don't have locales and so calling with NULL can have no effect.
/// This must not be called while this library is in active use. Typically
/// you would call it before using this library.
///
EAPATCHCLIENT_API void SetDefaultLocale(const char8_t* pLocale);


/// GetDefaultLocale
///
/// Returns the locale set by SetDefaultLocale, else NULL.
/// If you want the system locale, then you should call GetSystemLocale.
///
EAPATCHCLIENT_API const char8_t* GetDefaultLocale();


/// GetSystemLocale
///
/// Returns the locale set by SetDefaultLocale, else NULL.
///
EAPATCHCLIENT_API const char8_t* GetSystemLocale();



/// LocaleMatch
///
/// Returns true if pLocale1 'matches' pLocale2.
/// Matches are case insensitive.
/// Locales may not be empty or NULL, they must specify a valid locale pattern or 
/// else the return value is not defined.
/// If either component of either locale is a * instead of two letters, then it 
/// matches anything in the other locale's equivalent component.
///
/// Example usage and results:
///     LocaleMatch("en_us", "EN_US")      true
///     LocaleMatch("en_us", "dv_dv")      false
///     LocaleMatch("en_us", "en_*")       true
///     LocaleMatch("mx_dv", "en_*")       false
///     LocaleMatch("*_us", "dv_*")        true        // This is an unusual case which in practice is unlikely to be encountered.
///
bool LocaleMatch(const char8_t* pLocale1, const char8_t* pLocale2);

/// LocaleMatch
///
/// Acts the same as LocaleMatch(char8_t*, char8_t*) except that anything from either array can match
/// anything from the other array. This is potentially an n^2 operation, though in practice
/// at least one of the two arrays will have few elements, and even in the worst case 
/// there are ~20x20 = 400 comparisons, and this is unlikely to be a time-sensitive function.
/// If either array is empty then it's assumed to match all locales, and the return value is true.
///
bool LocaleMatch(const StringArray& localeArray1, const StringArray& localeArray2);

/// LocaleMatch
///
/// Hybrid version.
///
bool LocaleMatch(const StringArray& localeArray1, const char8_t* pLocale2);




///////////////////////////////////////////////////////////////////////////////
/// LocalizedString
///
/// Implements an interface to a localized string lookup.
/// The locale string format is user-defined but standard "en-us"-style 
/// locale names are recommended, though sometimes that needs to be 
/// "en_us" because - chars are disallowed by some encoding systems.
/// This class essentially wraps an stl map class.
/// Instances of this class are not internally thread-safe, and thread safety
/// must be managed at a higher level.
///
class EAPATCHCLIENT_API LocalizedString
{
public:
    LocalizedString();
    LocalizedString(const LocalizedString&);
   ~LocalizedString();

    LocalizedString& operator=(const LocalizedString&);

    /// If pLocale is NULL, the default EAPatch locale (GetDefaultLocale) is used.
    /// If subsequently the default locale is NULL then this function returns NULL.
    /// Lookups are case-insensitive, as otherwise surely somebody will forget to 
    /// make the case correct somewhere. 
    /// Returns NULL if there is no entry for the specified pLocale.
    /// The returned string may be empty if an empty (zero-length) string is associated 
    /// with the specified pLocale.
    /// Wildcards may be present in pLocale or the looked up LocaleStringMap. First 
    /// a perfect stricmp match is done with pLocale, and if that fails then a wildcard
    /// match is attempted.
    /// The returned string is a pointer into this class' data, and thus is good only 
    /// as long as the given class instance is unmodified.
    const char8_t* GetString(const char8_t* pLocale) const;

    /// Adds a string to the table.
    /// If pLocale is NULL, the default EAPatch locale (GetDefaultLocale) is used.
    /// If subsequently the default locale is NULL then this function has no effect.
    /// Adding a NULL pString removes the entry from the table.
    /// pString may be empty, which adds an empty string and does not remove the entry.
    /// There may be only one string associated with the given locale; calling this
    /// function a second time for a given pLocale overwrites the previous string.
    void AddString(const char8_t* pLocale, const char8_t* pString);

    /// Returns the count of entries in the table.
    size_t Count() const;

    //// Removes all entries from the table.
    void Clear();

    // Treats the strings as URLs and encodes them (e.g. replaces " " with "%20")
    void URLEncodeStrings();

    /// LocaleStringMap
    typedef eastl::fixed_map<StringSmall, String, 16, true, eastl::less<StringSmall>, CoreAllocator> LocaleStringMap;

    /// Returns the first found en_* value. If none are found then returns any value. If empty then returns a default value.
    const char8_t* GetEnglishValue() const;

    /// Returns the internal map itself. Used for serialization/deserialization.
    const LocaleStringMap& GetLocaleStringMap() const;
    LocaleStringMap& GetLocaleStringMap();

protected:
    LocaleStringMap mLocaleStringMap;
};

bool operator==(const LocalizedString&, const LocalizedString&);


} // namespace Patch
} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard



 

