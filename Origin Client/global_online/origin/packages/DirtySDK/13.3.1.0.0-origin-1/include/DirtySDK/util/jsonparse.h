/*H*************************************************************************************/
/*!
    \File jsonparse.h

    \Description
        A simple JSON parser.

    \Notes
        Written by Greg Schaefer outside of EA for a personal project, but donated
        back for a good cause.

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Version 12/11/2012 (jbrookes) Added to DirtySDK, added some new functionality
*/
/*************************************************************************************H*/

#ifndef _jsonparse_h
#define _jsonparse_h

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// parse json, preparing lookup for fast dereference
int32_t JsonParse(uint16_t *pDst, int32_t iMax, const char *pSrc, int32_t iLen);

// locate a json element
const char *JsonFind(const uint16_t *pParse, const char *pName);

// locate a json element, starting from an offset, with an optional array index
const char *JsonFind2(const uint16_t *pParse, const char *pJson, const char *pName, int32_t iIndex);

// get a string element
int32_t JsonGetString(const char *pJson, char *pBuffer, int32_t iLength, const char *pDefault);

// get an integer element
int64_t JsonGetInteger(const char *pJson, int64_t iDefault);

// get a number (supporting non-integer) element $$TODO
float JsonGetNumber(const char *pJson, float fDefault);

// get a date in ISO_8601 format
uint32_t JsonGetDate(const char *pJson, uint32_t uDefault);

// get a boolean element
uint8_t JsonGetBoolean(const char *pJson, uint8_t bDefault);

// get an enum element
int32_t JsonGetEnum(const char *pJson, const char *pEnumArray[], int32_t iDefault);


#ifdef __cplusplus
}
#endif

//@}

#endif // _jsonparse_h
