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

/*!
\Moduledef JsonParse JsonParse
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

//! alternate name for JsonSeekObjectEnd (DEPRECATED)
#define JsonGetListItemEnd(__pObject) JsonSeekObjectEnd(__pObject)

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// parse json, preparing lookup for fast dereference
DIRTYCODE_API int32_t JsonParse(uint16_t *pDst, int32_t iMax, const char *pSrc, int32_t iLen);

// locate a json element
DIRTYCODE_API const char *JsonFind(const uint16_t *pParse, const char *pName);

// locate a json element, starting from an offset, with an optional array index
DIRTYCODE_API const char *JsonFind2(const uint16_t *pParse, const char *pJson, const char *pName, int32_t iIndex);

// get a string element
DIRTYCODE_API int32_t JsonGetString(const char *pJson, char *pBuffer, int32_t iLength, const char *pDefault);

// get an integer element
DIRTYCODE_API int64_t JsonGetInteger(const char *pJson, int64_t iDefault);

// get a number (supporting non-integer) element $$TODO
DIRTYCODE_API float JsonGetNumber(const char *pJson, float fDefault);

// get a date in ISO_8601 format
DIRTYCODE_API uint32_t JsonGetDate(const char *pJson, uint32_t uDefault);

// get a boolean element
DIRTYCODE_API uint8_t JsonGetBoolean(const char *pJson, uint8_t bDefault);

// get an enum element
DIRTYCODE_API int32_t JsonGetEnum(const char *pJson, const char *pEnumArray[], int32_t iDefault);

// seek to the end of an object within the JSON text buffer
DIRTYCODE_API const char *JsonSeekObjectEnd(const char *pObject);

// seek to the start of the value of a key / value pair within the JSON text buffer
DIRTYCODE_API const char *JsonSeekValue(const char *pKey);


#ifdef __cplusplus
}
#endif

//@}

#endif // _jsonparse_h
