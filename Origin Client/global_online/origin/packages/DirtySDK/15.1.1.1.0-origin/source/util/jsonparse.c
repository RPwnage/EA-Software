/*H*************************************************************************************/
/*!
    \File jsonparse.c

    \Description
        Simple JSON parser.

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Notes
        Written by Greg Schaefer outside of EA for a personal project, but donated
        back for a good cause.

    \Version 12/11/2012 (jbrookes) Added to DirtySDK, added some new functionality
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/util/jsonparse.h"

/*** Defines ***************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Variables *************************************************************************/

//! decode table to classify characters as whitespace, number, letter, bracket, brace, or invalid
static const uint8_t _strDecode[256] =
{
    0,' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',    // 00..0f
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',    // 10..1f
  ' ',  0,'"',  0,  0,  0,  0,  0,  0,  0,  0,  0,',','1',  0,  0,    // 20..2f
  '1','1','1','1','1','1','1','1','1','1',':',  0,  0,  0,  0,  0,    // 30..3f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 40..4f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'[',  0,']',  0,  0,    // 50..5f
  ' ','a','a','a','a','a','a','a','a','a','a','a','a','a','a','a',    // 60..6f
  'a','a','a','a','a','a','a','a','a','a','a','{',0  ,'}',  0,  0,    // 70..7f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 80..8f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 90..9f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // a0..af
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // b0..bf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // c0..cf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // d0..df
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // e0..ef
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0     // f0..ff
};

//! characters that need to be escaped
static const uint8_t _strEscape[256] =
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 00..0f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 10..1f
    0,  0,'"',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'/',    // 20..2f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 30..3f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 40..4f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,'\\', 0,  0,  0,    // 50..5f
    0,  0,  8,  0,  0,  0, 12,  0,  0,  0,  0,  0,  0,  0, 10,  0,    // 60..6f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 70..7f
    0,  0, 13,  0,  9,'u',  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 80..8f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // 90..9f
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // a0..af
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // b0..bf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // c0..cf
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // d0..df
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    // e0..ef
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0     // f0..ff
};

//! hex->int decode table
static const uint8_t _HexDecode[256] =
{
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,128,128,128,128,128,128,
    128, 10, 11, 12, 13, 14, 15,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128, 10, 11, 12, 13, 14, 15,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,
    128,128,128,128,128,128,128,128,128,128,128,128,128,128,128,128
};


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _JsonGetQuoted

    \Description
        Get text from a quoted string

    \Input *pDst    - [out] storage for text
    \Input iLim     - size of output buffer
    \Input *pSrc    - pointer to quoted string
    \Input iSkip    - length check

    \Output
        int32_t     - returns string length

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
static int32_t _JsonGetQuoted(uint8_t *pDst, int32_t iLim, const uint8_t *pSrc, int32_t iSkip)
{
    int32_t iLen = 0;
    uint8_t c;

    // make room for terminator
    --iLim;

    // make sure its a quoted string
    if (*pSrc == '"')
    {
        for (++pSrc; (iLen < iLim) && (c = *pSrc) != '"'; ++pSrc)
        {
            // handle escape sequence
            if ((c == '\\') && (pSrc[1] > ' '))
            {
                c = _strEscape[*++pSrc];
                if ((c == 'u') && (pSrc[1] != '"') && (pSrc[2] != '"') && (pSrc[3] != '"') && (pSrc[4] != '"'))
                {
                    c = (_HexDecode[pSrc[3]]<<4)|(_HexDecode[pSrc[4]]<<0), pSrc += 4;
                }
            }
            // conditional save (to use as length check)
            *pDst = c;
            pDst += iSkip;
            ++iLen;
        }
        *pDst = 0;
    }

    return(iLen);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function JsonParse

    \Description
        Pre-parse a JSON buffer for fast parsing.  Must be called on a buffer before
        any Find() or Get() functions are used.

    \Input *pDst    - [out] storage for lookup buffer, two elements per object
    \Input iMax     - number of elements in output buffer
    \Input *_pSrc   - pointer to JSON buffer
    \Input iLen     - length of JSON buffer

    \Output
        int32_t     - zero=table too small, else size of table in bytes

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonParse(uint16_t *pDst, int32_t iMax, const char *_pSrc, int32_t iLen)
{
    uint8_t c;
    int32_t iArray = 0;
    int32_t iObject = 0;
    uint16_t *pArray[16];
    uint16_t *pObject[16];
    const uint8_t *pSrc = (const uint8_t *)_pSrc;
    const uint8_t *pOff = pSrc;
    const uint8_t *pEnd = pSrc+iLen;
    uint16_t *pBeg = pDst;
    uint16_t *pMax = pDst+iMax-2;

    // make sure parser table of sufficient size
    if (iMax < (int32_t)(sizeof(*pSrc)+sizeof(*pDst)*2))
    {
        return(0);
    }

    // cheat and copy json data pointer to start
    memcpy(pDst, &pSrc, sizeof(pSrc));
    pDst += sizeof(pSrc)/sizeof(*pDst);

    // find the tokens
    for (; (pSrc != pEnd) && (pDst != pMax); ++pSrc)
    {
        c = _strDecode[*pSrc];

        // handle end of decoding
        if (c == 0)
        {
            break;
        }
        // handle white space
        if (c == ' ')
        {
            continue;
        }
        // save start of token
        pDst[0] = pSrc-pOff;
        pDst[1] = 1;

        // handle number
        if (c == '1')
        {
            // skip past number
            for (++pSrc; (pSrc != pEnd) && (*pSrc >= '0') && (*pSrc <= '9'); ++pSrc)
                ;
            // handle decimal ending
            if ((pSrc != pEnd) && (*pSrc == '.'))
            {
                for (++pSrc; (pSrc != pEnd) && (*pSrc >= '0') && (*pSrc <= '9'); ++pSrc)
                    ;
            }
            // handle exponent
            if ((pSrc != pEnd) && ((*pSrc|32) == 'e'))
            {
                // gobble extension after exponent
                ++pSrc;
                if ((pSrc != pEnd) && ((*pSrc == '+') || (*pSrc == '-')))
                {
                    ++pSrc;
                }
                for (; (pSrc != pEnd) && (*pSrc >= '0') && (*pSrc <= '9'); ++pSrc)
                    ;
            }
            --pSrc;
            pDst[1] = (pSrc-pOff)-pDst[0]+1;
        }
        // handle string
        else if (c == '"')
        {
            // walk the string
            for (++pSrc; (pSrc != pEnd) && (*pSrc != '"'); ++pSrc)
            {
                if ((*pSrc == '\\') && (pSrc[1] > ' '))
                {
                    ++pSrc;
                }
            }
            pDst[1] = (pSrc-pOff)-pDst[0]+1;
        }
        // handle token
        else if (c == 'a')
        {
            for (++pSrc; (pSrc != pEnd) && (_strDecode[*pSrc] == 'a'); ++pSrc)
                ;
            --pSrc;
            pDst[1] = (pSrc-pOff)-pDst[0]+1;
        }
        // handle object open
        else if ((c == '{') && (iObject < (int32_t)(sizeof(pObject)/sizeof(pObject[0]))))
        {
            pObject[iObject++] = pDst+1;
        }
        // handle object close
        else if ((c == '}') && (iObject > 0))
        {
            --iObject;
            *pObject[iObject] =  pDst-pObject[iObject]+1;
        }
        // handle array open
        else if ((c == '[') && (iArray < (int32_t)(sizeof(pArray)/sizeof(pArray[0]))))
        {
            pArray[iArray++] = pDst+1;
        }
        // handle array close
        else if ((c == ']') && (iArray > 0))
        {
            --iArray;
            *pArray[iArray] = pDst-pArray[iArray]+1;
        }

        // save the record
        pDst += 2;
    }

    // close any remaining objects
    while (iObject-- > 0)
    {
        *pObject[iObject] = pDst-pObject[iObject]+1;
    }
    while (iArray-- > 0)
    {
        *pArray[iArray] = pDst-pArray[iArray]+1;
    }

    // terminate and return size in bytes
    *pDst++ = 0;
    *pDst++ = 0;
    return((int32_t)(((uint8_t *)pDst)-((uint8_t *)pBeg)));
}

/*F*************************************************************************************/
/*!
    \Function JsonFind

    \Description
        Locate a JSON element

    \Input *pParse      - pre-parsed lookup from JsonParse
    \Input *pName       - name of element to locate

    \Output
        const char *    - pointer to element, or NULL if not found

    \Notes
        A name can include a period character '.' which indicates an object reference,
        or an open bracket '[' which indicates an array reference.  A bracket must be
        the last character in the name.

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
const char *JsonFind(const uint16_t *pParse, const char *pName)
{
    return(JsonFind2(pParse, NULL, pName, 0));
}

/*F*************************************************************************************/
/*!
    \Function JsonFind2

    \Description
        Locate a JSON element, starting from an offset, with an optional array index

    \Input *pParse      - pre-parsed lookup from JsonParse
    \Input *_pJsonOff   - offset into JSON buffer to begin search
    \Input *pName       - name of element to locate
    \Input iIndex       - [optional] index of array element to return

    \Output
        const uint8_t * - pointer to element, or NULL if not found

    \Notes
        This version of Find is useful for parsing arrays of scalars or objects, or
        for starting a parse at an offset within the JSON document.

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
const char *JsonFind2(const uint16_t *pParse, const char *_pJsonOff, const char *pName, int32_t iIndex)
{
    const uint8_t *pJsonOff = (const uint8_t *)_pJsonOff;
    const uint8_t *pJson;
    const char  *pLast;
    int32_t iLen;
    uint8_t c;

    // get json data pointer
    // note vc bug-- thinks &pJson is const whereas it points to something const
    memcpy((void *)&pJson, pParse, sizeof(pJson));
    pParse += sizeof(pJson)/sizeof(*pParse);

    // if json has outer object, then skip it
    if (pJson[pParse[0]] == '{')
    {
        pParse += 2;
    }

    // if they gave us an offset, move the parse forward to match
    if (pJsonOff != NULL)
    {
        int32_t iOffset = (int32_t)(pJsonOff - pJson);
        for ( ; (pParse[0] < iOffset) && (pParse[0] != 0); pParse += 2)
            ;
    }

    // parse the find string
    while ((*pName != 0) && (pParse[1] > 0))
    {
        // handle traversing into an object
        if (*pName == '.')
        {
            // if not an object, then its an error
            if (pJson[pParse[0]] != '{')
            {
                break;
            }
            // move into object and keep parsing
            pParse += 2;
            ++pName;
            continue;
        }
        // handle traversing into an array
        else if (*pName == '[')
        {
            // if not an array, then its an error
            if (pJson[pParse[0]] != '[')
            {
                break;
            }
            // move into object
            pParse += 2;
            // skip to nth element
            for (c = ' '; iIndex > 0; iIndex -= 1)
            {
                c = pJson[pParse[0]];
                if (c == ']')
                {
                    break;
                }
                else if (c == '{')
                {
                    pParse += pParse[1]; // skip to end of object
                    pParse += 2; // skip trailing brace
                }
                else
                {
                    pParse += 2; // skip object
                }
                // skip trailing comma?
                if (pJson[pParse[0]] == ',')
                {
                    pParse += 2;
                }
            }
            // if we didn't end on a close bracket we've found our element
            if (pJson[pParse[0]] != ']') // must re-check because we could end exactly on the close bracket
            {
                pName = "\0";
            }
            break;
        }
        // check to see if object name matches
        else if (((*pName|32) >= 'a') && ((*pName|32) <= 'z'))
        {
            // figure out length
            for (pLast = pName; (*pLast != 0) && (*pLast != '.') && (*pLast != '['); ++pLast)
                ;
            iLen = (int32_t)(pLast-pName);

            // search for this name
            for (; pParse[1] > 0; pParse += 2)
            {
                c = pJson[pParse[0]];

                // skip arrays & objects
                if ((c == '[') || (c == '{'))
                {
                    pParse += pParse[1];
                    continue;
                }

                // end of array/object is terminator
                if ((c == ']') || (c == '}'))
                {
                    return(NULL);
                }

                // look for a label
                if ((c == '"') && (pJson[pParse[2]] == ':') && (iLen+2 == pParse[1]))
                {
                    if (memcmp(pJson+pParse[0]+1, pName, iLen) == 0)
                    {
                        pParse += 4;
                        pName = pLast;
                        break;
                    }
                }
            }
        }
        else
        {
            // invalid pattern
            return(NULL);
        }
    }

    // point to element
    return(*pName ? NULL : (const char *)(pJson+pParse[0]));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetString

    \Description
        Extract a JSON string element.  Pass pBuffer=NULL for length check.

    \Input *pJson       - JSON buffer
    \Input *pBuffer     - [out] storage for string (may be NULL for length query)
    \Input iLength      - length of output buffer
    \Input *pDefault    - default value to use, if string could not be extracted

    \Output
        int32_t         - length of string, or negative on error

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonGetString(const char *pJson, char *pBuffer, int32_t iLength, const char *pDefault)
{
    int32_t iLen;
    uint8_t c;

    // handle default value
    if (pJson == NULL)
    {
        // error if no default string
        if (pDefault == NULL)
        {
            return(-1);
        }
        // handle length-only query
        if (pBuffer == NULL)
        {
            return((int32_t)strlen(pDefault));
        }
        // copy the string
        for (iLen = 1; (iLen < iLength) && (*pDefault != 0); ++iLen)
        {
            *pBuffer++ = *pDefault++;
        }
        *pBuffer = 0;
        return(iLen-1);
    }

    // handle length query
    if (pBuffer == NULL)
    {
        return(_JsonGetQuoted(&c, 999999, (const uint8_t *)pJson, 0));
    }

    // make sure return buffer has terminator length
    if (iLength < 1)
    {
        return(-1);
    }

    // copy the string
    return(_JsonGetQuoted((uint8_t *)pBuffer, iLength, (const uint8_t *)pJson, 1));
}

/*F*************************************************************************************/
/*!
    \Function JsonGetInteger

    \Description
        Extract a JSON integer number element

    \Input *pJson       - JSON buffer
    \Input iDefault     - default value to use, if number could not be extracted

    \Output
        int64_t         - integer, or default value

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int64_t JsonGetInteger(const char *pJson, int64_t iDefault)
{
    int64_t iSign = 1;
    int64_t iValue;

    // handle default
    if (pJson == NULL)
    {
        return(iDefault);
    }

    // handle negative
    if (*pJson == '-')
    {
        ++pJson;
        iSign = -1;
    }

    // parse the value
    for (iValue = 0; (*pJson >= '0') && (*pJson <= '9'); ++pJson)
    {
        iValue = (iValue * 10) + (*pJson  & 15);
    }

    // return with sign
    return(iSign*iValue);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetNumber

    \Description
        Extract a JSON number element

    \Input *pJson       - JSON buffer
    \Input fDefault     - default value to use, if number could not be extracted

    \Output
        float           - number, or default value

    \Todo
        Implement

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
float JsonGetNumber(const char *pJson, float fDefault)
{
    return(fDefault);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetDate

    \Description
        Extract a JSON date element

    \Input *pJson       - JSON buffer
    \Input uDefault     - default date value to use

    \Output
        uint32_t        - extracted date, or default

    \Notes
        Date is assumed to be in ISO_8601 format

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
uint32_t JsonGetDate(const char *pJson, uint32_t uDefault)
{
    uint32_t uDate = uDefault;
    char strValue[128];

    JsonGetString(pJson, strValue, sizeof(strValue), "");
    if ((uDate = (uint32_t)ds_strtotime2(strValue, TIMETOSTRING_CONVERSION_ISO_8601)) == 0)
    {
        uDate = uDefault;
    }
    return(uDate);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetBoolean

    \Description
        Extract a JSON boolean

    \Input *pJson       - JSON buffer
    \Input bDefault     - default boolean value to use

    \Output
        uint8_t         - extracted bool, or default

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
uint8_t JsonGetBoolean(const char *pJson, uint8_t bDefault)
{
    uint8_t bValue = bDefault;

    // handle default
    if (pJson == NULL)
    {
        return(bDefault);
    }
    // check for true
    if (strncmp((const char *)pJson, "true", 4) == 0)
    {
        bValue = TRUE;
    }
    else if (strncmp((const char *)pJson, "false", 4) == 0)
    {
        bValue = FALSE;
    }
    // return result
    return(bValue);
}

/*F*************************************************************************************/
/*!
    \Function JsonGetEnum

    \Description
        Extract a JSON enumerated value.  The source type is assumed to be a string,
        and pEnumArray contains a NULL-terminated list of strings that may be converted
        to an enum value.

    \Input *pJson       - JSON buffer
    \Input *pEnumArray  - NULL-terminated list of enum strings
    \Input iDefault     - default enum value to use if no match is found

    \Output
        int32_t         - enum value; or default

    \Version 12/13/2012 (jbrookes)
*/
/*************************************************************************************F*/
int32_t JsonGetEnum(const char *pJson, const char *pEnumArray[], int32_t iDefault)
{
    int32_t iEnum = iDefault;
    char strValue[128];

    JsonGetString(pJson, strValue, sizeof(strValue), "");
    for (iEnum = 0; pEnumArray[iEnum] != NULL; iEnum += 1)
    {
        if (!strcmp(pEnumArray[iEnum], strValue))
        {
            return(iEnum);
        }
    }
    return(iDefault);
}

/*F*************************************************************************************************/
/*!
    \Function   JsonGetListItemEnd

    \Description
        Find the end of a single user entry in a JSON list

    \Input *pList - pointer to the beginning raw json user entry

    \Output
        const char*     - pointer to the end of the entry, NULL for error

    \Version 04/17/2013 (amakoukji)
*/
/*************************************************************************************************F*/
const char* JsonGetListItemEnd(const char *pList)
{
    // Extremely simply parser to find the ',' or ']' that denotes
    // the end of the friend list entry in JSON
    const char *pCurrent = pList;
    char c = *pList;
    uint32_t uDepth = 0;

    while (c != '\0')
    {
        c = *pCurrent;

        switch (c)
        {
            case '{':
            {
                ++uDepth;
            }
            break;

            case '}':
            {
                --uDepth;
            }
            break;

            case ',':
            {
                if(uDepth == 0)
                {
                    return(pCurrent);
                }
            }
            break;

            case ']':
            {
                if(uDepth == 0)
                {
                    return(pCurrent);
                }
            }
            break;
        }

        ++pCurrent;
    }

    // error
    return NULL;
}

/*F*************************************************************************************************/
/*!
    \Function JsonSeekValue

    \Description
        Given a pointer to the beginning of a simple JSON key value pair, seeks the 
        start of the value
        
    \Input *key               - module ref

    \Output int32_t           - pointer to the start of the value, NULL for error
        
    \Version 11/28/2013 (amakoukji)
*/
/*************************************************************************************************F*/
const char* JsonSeekValue(const char *key)
{
    // we assume we are pointing to the start of the key to a simple JSON object
    const char *pTemp = key;

    if (key == NULL)
    {
        return(NULL);
    }

    // Seek to the ':'
    while ( *pTemp != ':')
    {
        if (*pTemp == '\0' || *pTemp == '\n')
        {
            return(NULL);
        }
        ++pTemp;
    }
    ++pTemp;

    // Seek to non-whitespace
    while ( *pTemp == ' ' || *pTemp == '\t')
    {
        if (*pTemp == '\0' || *pTemp == '\n')
        {
            return(NULL);
        }
        ++pTemp;
    }

    return(pTemp);
}

