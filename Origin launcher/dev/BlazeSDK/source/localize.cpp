/*H********************************************************************************/
/*!
    \File localize.cpp

    \Description
        This module provides  a simple API to allow localization of some standard
        data types such as numbers, dates and times.

    \Notes
         While there are many other localization issues handled by the client, these
         are the minimum set required to allow cross-product game blaze statistics.

         Internally, data driven based on set of formatting rules per language.
         Exceptions are for a country basis. Currency too is per country.

    \Copyright
        (c) Electronic Arts. All Rights Reserved.

    \Version 1.0 11/12/2004 (tyrone@ea.com) First Version (Lobby project)
    \Version 1.0 03/09/2008 (divkovic@ea.com) First Version (Blaze project)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"

#include "BlazeSDK/shared/framework/locales.h"
#include "BlazeSDK/localize.h"

#include "EAStdC/EAString.h"

namespace Blaze
{

//TODO complete and fix table once info from localization folks come in.

//  iLang,cDecSymbol,cThouSep,eDateFormat,cDateSep,cTimeSep,bUse24Hour, bSignEnd,
//  strAM,strPM,bCurrSpace, bPercentInFront, bPercentSpace, currEnd

Localizer::LangMap Localizer::mLangMap[] =
{
    {  LANGUAGE_ENGLISH ,   '.', ',', _DF_MDY, '/', ':',  0, 0, "AM", "PM", 0,0,0,0},
    {  LANGUAGE_GERMAN ,    ',', '.', _DF_DMY, '.', ':',  1, 0, ""  ,  "",  1,0,1,1},
    {  LANGUAGE_FRENCH,     ',', ' ', _DF_DMY, '/', ':',  1, 0,  "", "",    1,0,1,1},

    {  LANGUAGE_FINNISH,    ',', ' ', _DF_DMY, '.', '.',  1, 0,  "", "",    1,0,1,1},
    {  LANGUAGE_DANISH,     ',', ' ', _DF_DMY, '.', '.',  1, 0,  "", "",    1,0,1,1},
    {  LANGUAGE_SWEDISH,    ',', ' ', _DF_YMD, '-', ':',  1, 0,  "", "",    1,0,1,1},

    {  LANGUAGE_NORWEGIAN,  ',', ' ', _DF_DMY, '.', '.',  1, 0,  "", "",    1,0,1,1},
    {  LANGUAGE_SPANISH,    ',', ' ', _DF_DMY, '/', ':',  1, 0,  "", "",    1,0,0,1},
    {  LANGUAGE_ITALIAN,    ',', '.', _DF_DMY, '/', ':',  1, 0,  "", "",    1,0,0,0},

    {  LANGUAGE_PORTUGUESE, ',', ' ', _DF_DMY, '/', ':',  1, 0,  "", "",    1,0,0,1},
    {  LANGUAGE_HUNGARIAN,  ',', ' ', _DF_YMD, '/', ':',  1, 0,  "", "",    1,0,0,1},
    {  LANGUAGE_RUSSIAN,    ',', ' ', _DF_DMY, '.', ':',  1, 0,  "", "",    1,0,0,1},

    {  LANGUAGE_CZECH,      ',', ' ', _DF_DMY, '.', ':',  1, 0,  "", "",    1,0,1,1},
    {  LANGUAGE_POLISH,     ',', ' ', _DF_DMY, '.', ':',  1, 0,  "", "",    1,0,0,1},
    {  LANGUAGE_DUTCH,      ',', '.', _DF_DMY, '-', ':',  1, 0,  "", "",    1,0,0,0},

    {  LANGUAGE_GREEK,      ',', '.', _DF_DMY, '/', ':',  1, 0,  "", "",    1,0,0,1},
    {  LANGUAGE_ITALIAN,    ',', '.', _DF_DMY, '/', ':',  1, 0,  "", "",    1,0,0,1},
    {  LANGUAGE_TURKISH,    ',', '.', _DF_DMY, '/', ':',  1, 0,  "", "",    1,1,0,1},
//keep default, zz as last...
    {  LANGUAGE_UNKNOWN ,   '.', ',', _DF_MDY, '/', ':',  1, 0, "AM", "PM", 0,0,0,0},
};
//see code below for country specific exceptions to language patterns
//-particularly, us,uk,ca and br.

// UCS-2 (unicode) map of country to currency symbol.
//Will be converted to utf-8 and returned. Euro is special cased...

//TODO --currency Entries for S.America, Scandanavia and East Europe..
Localizer::CurrMap Localizer::mCurrMap[] =
{
    {  COUNTRY_UNITED_STATES,       0x0024},     //dollar -- is just ascii..
    {  COUNTRY_UNITED_KINGDOM,      0x00A3},     //pound
       //NOTE:  euro is special cased, in code..
    {  COUNTRY_CANADA,              0x0024},     //dollar
    {  COUNTRY_KOREA_REPUBLIC_OF,   0x20A9},     //won
    {  COUNTRY_CHINA,               0x570E},     //yen-yuan
    {  COUNTRY_MEXICO,              0x20B1},     //peso

    {  COUNTRY_UNKNOWN,             0x0024}  //--keep this as last entry. Is  default of $
};


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function Localizer::LocalizePickCurrency

    \Description
        //Get currency symbol for country. Pick "$" if country does not match.

    \Input iCountry   - The country.
    \Input &rc - [out] -return code (warning) if country does not match of LOCALIZE_ERROR_COUNTRY.

    \Output  - The currency code chosen.

    \Version 1.0 11/11/2004 (tebert) First Version
*/
/********************************************************************************F*/
uint16_t Localizer::LocalizePickCurrency(int32_t iCountry, int32_t &rc) const
{
    int32_t i;
    rc = LOCALIZE_ERROR_NONE;
    //12 use Euro presently -
    if  (( iCountry == COUNTRY_FRANCE ) || (iCountry == COUNTRY_GERMANY)   || ( iCountry == COUNTRY_NETHERLANDS)
        ||(iCountry == COUNTRY_GREECE ) || (iCountry == COUNTRY_FINLAND)   || ( iCountry == COUNTRY_IRELAND)
        ||(iCountry == COUNTRY_ITALY  ) || (iCountry == COUNTRY_PORTUGAL)  || ( iCountry == COUNTRY_SPAIN)
        ||(iCountry == COUNTRY_AUSTRIA) || (iCountry == COUNTRY_LUXEMBOURG)|| ( iCountry == COUNTRY_BELGIUM)
        ||(iCountry == COUNTRY_HUNGARY) || (iCountry == COUNTRY_POLAND))
    {
        return(LOCALIZE_EURO_UCS);
    }
    for (i =0; ((mCurrMap[i].iCountry != iCountry)  && (mCurrMap[i].iCountry != COUNTRY_UNKNOWN )); i++)
        ;

    if  (mCurrMap[i].iCountry == COUNTRY_UNKNOWN )
        rc = LOCALIZE_ERROR_COUNTRY; //will use $ symbol..

    return (mCurrMap[i].uCurr);
}

/*F********************************************************************************/
/*!
    \Function Localizer::LocalizeFormatTime

    \Description
        Format time according to (language) rules given.

    \Input &rule   - reference to rule to apply
    \Input *pInBuf  - input buffer -if  input is from string, nullptr if not.
    \Input inDate  -  tm date struct, if input is raw date.
    \Input *pOutBuf - [out] output buffer

    \Output         - LOCALIZE_ERROR_*

    \Version 1.0 11/11/2004 (tebert) First Version
*/
/********************************************************************************F*/
LocalizeError Localizer::LocalizeFormatTime(const LangMap &rule, const char *pInBuf, const tm& inDate, size_t iBufLen, char *pOutBuf) const
{
#define MAX_SEC 60*60*24
    LocalizeError iResult = LOCALIZE_ERROR_NONE;
    char strAmPm[LOCALIZE_AMPM_STR_LEN + 1];
    struct tm date;
    int32_t iSecs;

    char cSep = rule.cTimeSep;

    if (pInBuf != nullptr)
    {
        iSecs = EA::StdC::AtoI32( pInBuf);
        if ((iSecs  <0) || (iSecs >= MAX_SEC))
        { //more than days worth, set error but carry on.
            iResult = LOCALIZE_ERROR_MAXSEC; // >day, but carry on
        }
        date.tm_sec  = iSecs % 60;
        iSecs /= 60;
        date.tm_min  = iSecs % 60;
        iSecs /= 60;
        date.tm_hour  = iSecs;
    }
    else
    {
        date = inDate;
    }

    if (rule.bUse24Hour == 0)
    {
        int32_t iHour = date.tm_hour;
        // We're in the PM if the hours is 12 or greater
        if (iHour >= 12)
        {
            blaze_strnzcpy (strAmPm, "PM", sizeof(strAmPm));
        }
        else
        {
            blaze_strnzcpy (strAmPm, "AM", sizeof(strAmPm));
        }
        // Adjust for 24 hour mode
        if (iHour > 12)
        {
            iHour -= 12;
        }
        // don't forget to compensate for 0:XX (midnight) in 24 hour mode
        else if (iHour == 0)
        {
            iHour = 12;
        }
        blaze_snzprintf(pOutBuf, iBufLen, "%02d%c%02d%c%02d %s", iHour, cSep, date.tm_min,
            cSep, date.tm_sec, strAmPm);
    }
    else
    {
        //use 24 hour clock..
        blaze_snzprintf(pOutBuf, iBufLen, "%02d%c%02d%c%02d", date.tm_hour, cSep, date.tm_min, cSep, date.tm_sec);
    }

    if (strlen (pOutBuf) >= (unsigned)iBufLen)
    {
        pOutBuf[iBufLen-1] = '\0'; //terminate, due to truncation..
        iResult = LOCALIZE_ERROR_BUFFER;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function LocalizeFormatDate

    \Description
        Format date (and time) according to (language) rules given.

    \Input rule     - reference to rule to apply
    \Input *pInBuf  - input buffer
    \Input iBufLen  - size of output buffer
    \Input *pOutBuf - [out] output buffer
    \Input bTimeToo - should format time too?

    \Output         - LOCALIZE_ERROR_*

    \Version 1.0 11/11/2004 (tebert) First Version
*/
/********************************************************************************F*/
LocalizeError Localizer::LocalizeFormatDate (const LangMap &rule, const char *pInBuf,  size_t iBufLen, char *pOutBuf, bool bTimeToo) const
{
    LocalizeError iResult = LOCALIZE_ERROR_NONE;
    struct tm tmDate;
    char strDate[32];
    char strTime[32];
    time_t uEpoch;
    char cDSep = rule.cDateSep;

    uEpoch = (time_t)EA::StdC::AtoI32( pInBuf);
    tmDate = *localtime(&uEpoch);

    if (rule.eDateFormat == _DF_YMD)
    {
        blaze_snzprintf(strDate, sizeof(strDate), "%04d%c%02d%c%02d",
            tmDate.tm_year+1900, cDSep, tmDate.tm_mon+1, cDSep, tmDate.tm_mday);
    }
    else if (rule.eDateFormat == _DF_MDY)
    {
        blaze_snzprintf(strDate, sizeof(strDate), "%02d%c%02d%c%04d",
            tmDate.tm_mon+1, cDSep, tmDate.tm_mday, cDSep, tmDate.tm_year+1900);
    }
    else if (rule.eDateFormat == _DF_DMY)
    {
        blaze_snzprintf(strDate, sizeof(strDate), "%02d%c%02d%c%04d",
            tmDate.tm_mday, cDSep, tmDate.tm_mon+1,cDSep, tmDate.tm_year+1900);
    }
    else
    {
        iResult = LOCALIZE_ERROR_DATA; //use a default..
        blaze_snzprintf(strDate, sizeof(strDate), "%02d%c%02d%c%04d",
            tmDate.tm_mday, cDSep, tmDate.tm_mon+1,cDSep, tmDate.tm_year+1900);
    }

    if (bTimeToo)
    {
        iResult = LocalizeFormatTime (rule, nullptr, tmDate, sizeof (strTime), strTime);
        blaze_snzprintf(pOutBuf, iBufLen, "%s%c%s", strDate, ' ', strTime);
    }
    else
    {
        blaze_strnzcpy (pOutBuf, strDate, iBufLen);
    }
    if ((strlen (pOutBuf) >= (unsigned)iBufLen) && (iResult == 0))
    {
        pOutBuf[iBufLen-1] = 0; //terminate, due to truncation..
        iResult = LOCALIZE_ERROR_BUFFER;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function Localizer::LocalizeFormatNum

    \Description
        Format numerics according to language rules given.

    \Input rule     - reference to rule to apply
    \Input iType    - type
    \Input *pInBuf  - input buffer
    \Input iBufLen  - size of output buffer
    \Input *pOutBuf - [out] output buffer

    \Output         - LOCALIZE_ERROR_*

    \Version 1.0 11/11/2004 (tebert) First Version
*/
/********************************************************************************F*/
LocalizeError Localizer::LocalizeFormatNum (const LangMap &rule, int32_t iType, const char * pInBuf,  size_t iBufLen, char *pOutBuf) const
{
    LocalizeError iResult = LOCALIZE_ERROR_NONE;
    int32_t iDecPos = -1,  iMinPos = -1;  //decimal and minus position
    bool bOverflow = false;
    int32_t iInPos, iOutPos;
    int32_t iInStrLen = (int32_t)strlen(pInBuf);

    //get the standard decimal and minus position if any, from input stream.
    for ( iInPos = 0; iInPos < iInStrLen; iInPos++)
    {
        if ((iDecPos ==-1 ) && (pInBuf[iInPos] == LOCALIZE_DECIMAL_SIGN))
            iDecPos = iInPos;

        if ((iMinPos ==-1 ) && (pInBuf[iInPos] == LOCALIZE_MINUS_SIGN))
            iMinPos = iInPos;
    }
    if (iDecPos == -1)
    {
        iDecPos = iInStrLen;  // no decimal, assumes its at end.
    }

    // traverse input, copy to output, adding format info as we go..
    for (iInPos = 0, iOutPos = 0; iInPos < iInStrLen; iInPos++)
    {
        if (iOutPos >= (int32_t)(iBufLen-1))
        {
            bOverflow = true;
            break;
        }
        if (iInPos == iDecPos)
        {
            pOutBuf[iOutPos++] = rule.cDecSymbol;
        }
        else if ((iInPos == iMinPos) && (rule.bSignEnd))
        {
            ; //skip the minus -- it moves to end
        }
        else
        {
            if (iType == LOCALIZE_TYPE_SEPNUM)
            {
                // add thousands seperator, every 3 digits, if needed.
                if (((iDecPos - iInPos) > 1) && (((iDecPos - iInPos) % 3) == 0)&& (iOutPos != 0))
                {
                    pOutBuf[iOutPos++] = rule.cThouSep; //insert thousand separator.
                }
            }
            pOutBuf[iOutPos++] = pInBuf[iInPos];    //copy original digit
        }
    }
    if ( (iMinPos != -1) && (rule.bSignEnd) && !bOverflow )
    {
        // add minus to end..
        pOutBuf[iOutPos++] = LOCALIZE_MINUS_SIGN;
        if (iOutPos >= (int32_t)iBufLen)
        {
            iOutPos--;
            bOverflow = true;
        }
    }
    pOutBuf[iOutPos] = '\0';

    // did it overflow output buffer?
    iResult = (bOverflow) ? LOCALIZE_ERROR_BUFFER : iResult;
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function Localizer::Localazer

    \Description
        Constructor. Set params for  Localizer, like language and country.

    \Input iCurrency -the currency symbol the application wants the localizer
        to use (0=let localizer pick, based on country).
    \Input uLocality -the iso3166 country code and language in 4 byte character-as-int32_t format.
                See locale.h.
    \Output -  none

*/
/********************************************************************************F*/
Localizer::Localizer(uint32_t uLocality, uint16_t iCurrency)
{
    int32_t i;
    int32_t iResult = 0;

    if (uLocality == 0)
    {
        uLocality = LOCALE_DEFAULT;
    }

    mCountry = LocaleTokenGetCountry(uLocality);
    mLanguage = LocaleTokenGetLanguage(uLocality);

    // if currency symbol not set, we pick based on country.
    mCurrency = (iCurrency == 0) ?
        LocalizePickCurrency(mCountry, iResult) : iCurrency;

    // find rules for country...
    for (i = 0; ((mLangMap[i].iLang != mLanguage)  && (mLangMap[i].iLang != LANGUAGE_UNKNOWN)); i++)
        ;

    // if not found, will use unknown
    mLangRules = mLangMap[i];

    // Now look for few exceptions. In general, language sets the default, except
    // for a few, very specific country exemptions..
    if ((mCountry == COUNTRY_UNITED_KINGDOM )
                    && (mLanguage == LANGUAGE_ENGLISH))
        mLangRules.eDateFormat = _DF_DMY;

    if ((mCountry == COUNTRY_MEXICO) && (mLanguage == LANGUAGE_SPANISH))
    {
        mLangRules.cDecSymbol = '.';
        mLangRules.cThouSep = ',';

        mLangRules.bCurrEnd = 0;
        mLangRules.bCurrSpace = 0;
    }

    //TODO -- add more country excemptions, as info from localization team comes in.
}

/*F********************************************************************************/
/*!
    \Function Utf8EncodeFromUtf16CodePt

    \Description
        Converts UTF16 character into a signle encoded UTF16 char (which is
        acutally an array of bytes - string)
        WARNING: The rutine works only for UTF16 chars in range 0x0000 - 0xFFFF

    \Input strUtf -  Destination buffer
    \Input utf16sym - UTF16 symbol 0x0000 - 0xFFFF
*/
/********************************************************************************F*/
int32_t Localizer::Utf8EncodeFromUtf16CodePt(char *strUtf, uint16_t utf16sym) const
{
    int32_t len = 0;
    if (utf16sym <= 0x7f)
    {
        len = 1;
        strUtf[0] = (char) utf16sym;

    }
    else if (utf16sym > 0x007f && utf16sym <= 0x07ff)
    {
        len = 2;
        strUtf[0] = (char)( 0xc0 | ( ( utf16sym & 0x07c0 ) >> 11 ) );
        strUtf[1] = (char)( 0x80 | ( ( utf16sym & 0x003f )       ) );
    }
    else if (utf16sym > 0x07ff)
    {
        len = 3;
        strUtf[0] = (char)( 0xe0 | ( ( utf16sym & 0xf000 ) >> 12 ) );
        strUtf[1] = (char)( 0x80 | ( ( utf16sym & 0x0fc0 ) >> 6  ) );
        strUtf[2] = (char)( 0x80 | ( ( utf16sym & 0x003f )       ) );
    }

    return len;
}

/*F********************************************************************************/
/*!
    \Function LocalizeData

    \Description
        Localize the data based on the language,country  and data format.

    \Input iType -  What type/format are we localizing. A LOCALIZE_TYPE_* selector
    \Input pBuffer -Localized output buffer--must be allocated by caller.
            Strings will be terminated by this api, even if overflow occurs.
    \Input iBufLen -length of above outputbuffer. Must allow for null terminator too.
    \Input pSource -Input source string.

    \Output -The return value is a LOCALIZE_ERROR_* error code

    \Notes
         The incoming data is always a string. This means that something like a number is
         already in ASCII/UTF8 format before this function is called.

     \Version 1.0 11/11/2004 (Tyrone Ebert) First Version
*/
/********************************************************************************F*/
LocalizeError Localizer::LocalizeData(LocalizeType iType, char *pBuffer, size_t iBufLen, const char *pSource)
{
    LocalizeError iResult = LOCALIZE_ERROR_NONE;

    if ((iBufLen <= 0) || (pBuffer == nullptr) || (pSource == nullptr))
        return(LOCALIZE_ERROR_DATA);

    switch(iType)
    {
    case  LOCALIZE_TYPE_NUMBER:
    case  LOCALIZE_TYPE_SEPNUM:
    case  LOCALIZE_TYPE_RANK:
    case  LOCALIZE_TYPE_RANK_PTS:
        iResult =LocalizeFormatNum (mLangRules, iType, pSource, iBufLen, pBuffer);
        break;
    case LOCALIZE_TYPE_TIME:
        struct tm DummyDate;
        memset(&DummyDate, 0, sizeof(DummyDate));
        iResult =LocalizeFormatTime (mLangRules, pSource, DummyDate, iBufLen, pBuffer);
        break;
    case LOCALIZE_TYPE_DATETIME:
        iResult =LocalizeFormatDate (mLangRules, pSource, iBufLen, pBuffer, 1);
        break;
    case LOCALIZE_TYPE_DATE:
        iResult =LocalizeFormatDate (mLangRules, pSource, iBufLen, pBuffer, 0); //time2=0
        break;
    case LOCALIZE_TYPE_PERCENT:

        if ( !strcmp(pSource,"-") || !strcmp(pSource,"") )
        {
            blaze_snzprintf (pBuffer, iBufLen, "%s", pSource);
        }
        // NOTE: literal "%" requires "%%", and buffer overflow check done at end..
        else
        {
            char strNum[32];
            iResult = LocalizeFormatNum(mLangRules, LOCALIZE_TYPE_NUMBER, pSource, sizeof(strNum), strNum);
            if (iResult != LOCALIZE_ERROR_NONE)
                break;

            if (mLangRules.bPercentInFront)
            { //french, italian, spanish  etc.
                if (mLangRules.bPercentSpace)
                    blaze_snzprintf(pBuffer, iBufLen, "%% %s", strNum);
                else
                    blaze_snzprintf(pBuffer, iBufLen, "%%%s", strNum);
            }
            else if (mLangRules.bPercentSpace)
            {
                blaze_snzprintf(pBuffer, iBufLen, "%s %%", strNum);
            }
            else
            { //default, go to back, no space..
                blaze_snzprintf (pBuffer, iBufLen, "%s%%", strNum);
            }
        }

        break;
    case LOCALIZE_TYPE_CURRENCY:
        {// convert UCS symbol to the utf-8 and add currency string... (is one char for $ )
            char strUtf[32];
            char strSpec[32];
            int32_t i = Utf8EncodeFromUtf16CodePt(strUtf, mCurrency);
            if (i < (int32_t) sizeof(strUtf))
                strUtf[i] = 0;//terminate  the utfstring
            if (mLangRules.bCurrSpace)
            {
                blaze_snzprintf(strSpec, sizeof(strSpec), "%%s %%s");
            }
            else
            {
                blaze_snzprintf(strSpec, sizeof(strSpec), "%%s%%s");
            }
            if ( mLangRules.bCurrEnd)
            {// $ goes at end...
                blaze_snzprintf (pBuffer, iBufLen, strSpec, pSource, strUtf);
            }
            else
            {// at front
                if (pSource[0] =='-' )
                { // convert  $-10.00 to -$10.00
                    pBuffer[0] = '-';
                    blaze_snzprintf (pBuffer+1, iBufLen-1, strSpec, strUtf, pSource+1);
                }
                else //positive, in front, currency
                {
                    blaze_snzprintf (pBuffer, iBufLen, strSpec, strUtf, pSource);
                }
            }
        }
        break;
        //RAW is for Ranker to shortcut localizer, but, if get it, same as string.
    case LOCALIZE_TYPE_STRING:
    case LOCALIZE_TYPE_RAW:
    case LOCALIZE_TYPE_NAME:
        strncpy(pBuffer, pSource, iBufLen);
        break;

    default:
        iResult = LOCALIZE_ERROR_TYPE;
        strncpy(pBuffer, pSource, iBufLen);
    }
    // did we overflow?
    if ( (iResult == 0) && (pBuffer) && (strlen(pBuffer) >= iBufLen))
    {
        pBuffer[iBufLen-1] = 0; //terminate
        iResult = LOCALIZE_ERROR_BUFFER;
    }
    return(iResult);
}

//! Debug function to dump our locale as a languageCountry string
void Localizer::GetLocaleString(char *pBuf, size_t uLength) const
{
   uint32_t uLoc = (uint32_t) LocaleTokenCreate(mLanguage, mCountry);
   blaze_snzprintf (pBuf, uLength, "%c%c%c%c",  LocaleTokenPrintCharArray(uLoc));
}

} // Blaze
