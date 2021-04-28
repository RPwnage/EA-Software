/**************************************************************************************************/
/*! 
    \file localize.h
    
    
    \note
        This file is based on lobbylocalize.h from DirtySDK.
    
    \attention
        (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef LOCALIZE_H
#define LOCALIZE_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files ****************************************************************/
#include "BlazeSDK/shared/framework/locales.h"

/*** Defines **********************************************************************/
// the following data types are supported by localizer

namespace Blaze
{

enum LocalizeType
{
 LOCALIZE_TYPE_PERCENT =  ('~pct'),  //!< percentage: xx%
 LOCALIZE_TYPE_NUMBER  =  ('~num'),  //!< number 123456
 LOCALIZE_TYPE_SEPNUM  =  ('~sep'),  //!< separated number 123,456
 LOCALIZE_TYPE_TIME    =  ('~tim'),  //!< time 12:34a
 LOCALIZE_TYPE_DATE    =  ('~dat'),  //!< date 5/6/04
 LOCALIZE_TYPE_DATETIME=  ('~dtm'),  //!< date and time 5/6/05 12:34a
 LOCALIZE_TYPE_STRING  =  ('~str'),  //!< string (data unchanged)
 LOCALIZE_TYPE_RAW     =  ('~raw'),  //!< data unchanged
 LOCALIZE_TYPE_CURRENCY=  ('~cur'),  //!< $327.00
 LOCALIZE_TYPE_RANK    =  ('~rnk'),  //!< Rank (localize same as ~num)
 LOCALIZE_TYPE_RANK_PTS=  ('~pts'),  //!< Ranking points (localize same as ~num)
 LOCALIZE_TYPE_NAME    =  ('~nam')  //!< Rank entry name (localize same as ~str)
};

enum LocalizeError
{
// the following errors can be returned by the localizer
 LOCALIZE_ERROR_NONE         = (0),        //!< no localizing
 LOCALIZE_ERROR_LANG         = (-1),        //!< unknown language, 'en' will be used.
 LOCALIZE_ERROR_COUNTRY      = (-2),        //!< unknown country, 'us' will be used.
 LOCALIZE_ERROR_LANG_COUNTRY = (-3),        //!< unknown language,  and country..
 LOCALIZE_ERROR_TYPE         = (-4),        //!< unsupported localization type.
 LOCALIZE_ERROR_BUFFER       = (-5),        //!< output buffer too small
 LOCALIZE_ERROR_DATA         = (-6),        //!< input data is invalid
 LOCALIZE_ERROR_MAXSEC       = (-7),        //!< warning-overflow secs in a day, will truncate.
 LOCALIZE_ERROR_FATAL        = (-8)        //!< fatal error --did not init etc.
};

const int32_t LOCALIZE_AMPM_STR_LEN = 5;       // max char for am/pm string, if any. 
const char LOCALIZE_MINUS_SIGN      =  '-';    //default sign, in input.
const char LOCALIZE_DECIMAL_SIGN    =  '.';    //default dec. sign, in input.
const int32_t LOCALIZE_EURO_UCS     =  0x20AC; //euro currency symbol.

/*! ***********************************************************************************************/
/*!
    \class Localizer

    \brief Utilities for converting numbers into localized strings

    The Localizer class 

*************************************************************************************************/
class BLAZESDK_API Localizer
{
public:
    /*! ********************************************************************************/
    /*!
        \brief
            Constructor. Sets params for Localizer: language and country.

        \param[in] iCurrency The currency symbol the application wants the localizer
            to use (0=let localizer pick, based on country).
        \param[in] uLocality The iso3166 country code and language in 4 byte character-as-int32_t format.
                    See locale.h.

    */
    /********************************************************************************F*/
    Localizer(uint32_t uLocality, uint16_t iCurrency);

    /*! ****************************************************************************/
    /*! \brief Destructor
    ********************************************************************************/
    virtual ~Localizer() { };

    /*! *********************************************************************************/
    /*!
        \brief
            Localizes the data based on the language,country and data format. 
            The incoming data is always a string. This means that you need to convert your value to
            ASCII/UTF8 format before this function is called. The method then modifies the
            string to match the localization rules.
            This method can be repaced in custom Localizer implementation.

        \param[in] iType   What type/format are we localizing. 
        \param[in] pBuffer Localized output buffer--must be allocated by caller.
                   Strings will be terminated by this api, even if overflow occurs.
        \param[in] iBufLen length of above outputbuffer. Must allow for null terminator too.
        \param[in] pSource Input source string.

        \return The return value is a LOCALIZE_ERROR_* error code

         
    */
    /********************************************************************************F*/
    virtual LocalizeError LocalizeData(LocalizeType iType, char *pBuffer, size_t iBufLen, const char *pSource);

    /*! *********************************************************************************/
    /*!
        \brief
            Returns the current locale sttings for this localizer.

        \param[in] pBuf    Output buffer--must be allocated by caller.
        \param[in] uLength Length of above outputbuffer. Must allow for null terminator too.
         
    */
    /********************************************************************************F*/
    void GetLocaleString(char *pBuf, size_t uLength) const;

protected:
    enum DateFormat   //date format. --cryptic to fit map in one line.
    {   _DF_YMD=0,   // euro and iso standard. 
        _DF_DMY,     // brits.
        _DF_MDY      // just them 'mericans.
    };

    // Struct to hold language specific formatting rules or  behaviour.  
    struct LangMap
    {
        int32_t iLang;                  //language
    
        char cDecSymbol;                // Decimal Symbol for this lang/country.
        char cThouSep;                  // Thousands Separator for this lang/country.

        DateFormat eDateFormat;                 // how is date formatted, by default.
        char cDateSep;                          // day-month-year separator.
        char cTimeSep;                          // hour-minute-sec separator.
        int32_t  bUse24Hour;                    // use 24 hour clock?
        int32_t  bSignEnd;                      // does sign go in back?
        char strAM [LOCALIZE_AMPM_STR_LEN+1];   // if not 24 hour, AM string.
        char strPM [LOCALIZE_AMPM_STR_LEN+1];   // if not 24 hour, PM string.
        int32_t  bCurrSpace;                    // does currency have space between number and symbol?
        int32_t  bPercentInFront;               // does % age  go in front?
        int32_t  bPercentSpace;                 // does % age take a space between number and %?
        int32_t  bCurrEnd;                      // does currency sym go to end?
    };

    //Struct to hold currency symbol, for each country.
    struct CurrMap
    {
        int32_t iCountry;   // country code -iso as decimal.
        uint16_t uCurr;     // ucs2/unicode symbol.
    };
    
    static LangMap mLangMap[];
    static CurrMap mCurrMap[];

    uint16_t LocalizePickCurrency(int32_t iCountry, int32_t &rc) const;
    LocalizeError LocalizeFormatDate (const LangMap &rule, const char *pInBuf,  size_t iBufLen, char *pOutBuf, bool bTimeToo) const;
    LocalizeError LocalizeFormatTime(const LangMap &rule, const char *pInBuf, const tm& inDate, size_t iBufLen, char *pOutBuf) const;
    LocalizeError LocalizeFormatNum (const LangMap &rule, int32_t iType, const char * pInBuf,  size_t iBufLen, char *pOutBuf) const;
    int32_t Utf8EncodeFromUtf16CodePt(char *strUtf, uint16_t utf16sym) const;

    uint16_t mCountry;     //!< iso3166 country code
    uint16_t mLanguage;    //!< iso639 language code
    uint16_t mCurrency;    //!< currency symbol to use.ucs-2 /unicode symbol
    LangMap mLangRules;
};

} // Blaze
#endif // LOCALIZE_H
