/*H*************************************************************************************/
/*!
    \File    tagfield.h

    \Description
        This module provides routines to insert/extract tagged data into a free-form text buffer.
        Several data types are provided to allow the representation to be as natural as possible
        (i.e., the Binary routine could be used for everthing, but it is much more difficult to
        decode externally than the String routine).

    \Copyright
        Copyright (c) Electronic Arts 2000-2004. ALL RIGHTS RESERVED.

    \Version 1.0 01/23/2000 (gschaefer) First version
    \Version 1.1 04/03/2002 (gschaefer) Removed sscanf dependency
    \Version 1.2 01/29/2003 (gschaefer) Changed calling to signed char
    \Version 2.0 12/10/2004 (gschaefer) Redesigned "set" logic to avoid stack use
*/
/*************************************************************************************H*/

#ifndef _tagfield_h
#define _tagfield_h

/*!
\Moduledef TagField TagField
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

// clear the data buffer
#define TagFieldClear(record, reclen) (record)[0] = (0)

// save an entire structure as a single field
#define TagFieldSetStruct(record, reclen, name, bin, pat) TagFieldSetStructure(record, reclen, name, bin, -1, pat)

// get packed structure field contents
#define TagFieldGetStruct(data, buf, pat) TagFieldGetStructure(data, buf, -1, pat)

// get binary field contents using more efficient encoding
#define TagFieldGetBinary7(data, buffer, buflen) TagFieldGetBinary(data, buffer, buflen)

// old TagFieldSetCopy is now implemented via new TagFieldSetRaw function
#define TagFieldSetCopy(record, reclen, name, source) TagFieldSetRaw(record, reclen, name, TagFieldFind(source, name))

// unsigned version of TagFieldSetNumber
#define TagFieldSetUnsignedNumber(_pRecord, _iRecLen, _pName, _uValue) TagFieldSetNumber(_pRecord, _iRecLen, _pName, (signed)(_uValue))

// unsigned version of TagFieldGetNumber
#define TagFieldGetUnsignedNumber(_pData, _uDefVal) (unsigned)TagFieldGetNumber(_pData, (signed)(_uDefVal));


/*** Type Definitions ******************************************************************/

// Define the different types supported by TagFieldGet/SetStructure patterns
typedef enum
{
    TAGFIELD_PATTERN_INT8  = 'b',   // int8_t
    TAGFIELD_PATTERN_INT16 = 'w',   // int16_t
    TAGFIELD_PATTERN_INT32 = 'l',   // int32_t
    TAGFIELD_PATTERN_STR   = 's'    // string
} TagFieldPatternE;

// Pointer structure to allow enumeration of TagFieldSetStructure() encoded attrs
typedef struct TagFieldStructPtrT
{
    TagFieldPatternE eType;     // The type of the attribute
    int32_t iOffset;                // Offset from structure pointer where attr lives
} TagFieldStructPtrT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allow unit test to be called (code is in lobbytagtest.c)
DIRTYCODE_API int32_t TagFieldUnitTest(int32_t iVerbose);

// set the default field divider character
DIRTYCODE_API char TagFieldDivider(char iDivider);

// convert a tag buffer to use specified marker char
DIRTYCODE_API void TagFieldFormat(char *record, unsigned char marker);

// search the buffer and locate the named field
DIRTYCODE_API const char *TagFieldFind(const char *record, const char *name);

// search the buffer and locate the two-part named field (the names are catenated together)
DIRTYCODE_API const char *TagFieldFind2(const char *record, const char *name1, const char *name2);

// search the buffer and locate the indexed named field (eg NAME5)
DIRTYCODE_API const char *TagFieldFindIdx(const char *pRecord, const char *pName, int32_t iIdx);

// delete a field from the record
DIRTYCODE_API int32_t TagFieldDelete(char *record, const char *name);

// rename a field within the record
DIRTYCODE_API int32_t TagFieldRename(char *record, int32_t reclen, const char *oldname, const char *newname);

// duplicate an entire record
DIRTYCODE_API int32_t TagFieldDupl(char *record, int32_t reclen, const char *data);

// extract the first field name in record
DIRTYCODE_API int32_t TagFieldFirst(const char *record, char *nameptr, int32_t namelen);

// set a numeric field
DIRTYCODE_API int32_t TagFieldSetNumber(char *record, int32_t reclen, const char *name, int32_t value);

// set a 64-bit numeric field
DIRTYCODE_API int32_t TagFieldSetNumber64(char *record, int32_t reclen, const char *name, int64_t value);

// set a flags field
DIRTYCODE_API int32_t TagFieldSetFlags(char *record, int32_t reclen, const char *name, int32_t value);

// set an ip-address field
DIRTYCODE_API int32_t TagFieldSetAddress(char *record, int32_t reclen, const char *name, uint32_t addr);

// set a token field
DIRTYCODE_API int32_t TagFieldSetToken(char *record, int32_t reclen, const char *name, int32_t token);

// set a string field
DIRTYCODE_API int32_t TagFieldSetString(char *record, int32_t reclen, const char *name, const char *value);

// set a binary field
DIRTYCODE_API int32_t TagFieldSetBinary(char *record, int32_t reclen, const char *name, const void *value, int32_t vallen);

// set a binary field using more efficient encoding
DIRTYCODE_API int32_t TagFieldSetBinary7(char *record, int32_t reclen, const char *name, const void *value, int32_t vallen);

// save an entire structure as a single field
DIRTYCODE_API int32_t TagFieldSetStructure(char *record, int32_t reclen, const char *name, const void *_bin, int32_t len, const char *pat);

// set a date from epoch time using std C time.h lib
DIRTYCODE_API int32_t TagFieldSetEpoch(char *record, int32_t reclen, const char *name, uint32_t epoch);

// set a date field from discrete date components
DIRTYCODE_API int32_t TagFieldSetDate(char *record, int32_t reclen, const char *name,
                    int32_t year, int32_t month, int32_t day, int32_t hour, int32_t minute, int32_t second, int32_t gmt);

// set a floating point number
DIRTYCODE_API int32_t TagFieldSetFloat(char *pRecord, int32_t iReclen, const char *pName, float fValue);

// store raw field contents in a record
DIRTYCODE_API int32_t TagFieldSetRaw(char *pRecord, int32_t iReclen, const char *pName, const char *pData_);

// merge two records together (does not merge bulk data)
DIRTYCODE_API int32_t TagFieldMerge(char *record, int32_t reclen, const char *merge);

// create a tagfield record using printf style descriptor
DIRTYCODE_API int32_t TagFieldPrintf(char *pRecord, int32_t iLength, const char *pFormat, ...);

// get a numeric field
DIRTYCODE_API int32_t TagFieldGetNumber(const char *data, int32_t defval);

// get a 64-bit numeric field
DIRTYCODE_API int64_t TagFieldGetNumber64(const char *data, int64_t defval);

// get a flags field
DIRTYCODE_API int32_t TagFieldGetFlags(const char *data, int32_t defval);

// get an ip address field
DIRTYCODE_API uint32_t TagFieldGetAddress(const char *data, uint32_t defval);

// get a token
DIRTYCODE_API int32_t TagFieldGetToken(const char *data, int32_t deftok);

// get item from delimeter separated list
DIRTYCODE_API int32_t TagFieldGetDelim(const char *data, char *buffer, int32_t buflen, const char *defval, int32_t index, int32_t delim);

// get string field contents
DIRTYCODE_API int32_t TagFieldGetString(const char *data, char *buffer, int32_t buflen, const char *defval);

// get binary field contents
DIRTYCODE_API int32_t TagFieldGetBinary(const char *data, void *buffer, int32_t buflen);

// get packed structure field contents
DIRTYCODE_API int32_t TagFieldGetStructure(const char *data, void *_buf, int32_t len, const char *pat);

// get date in epoch time using std C time.h lib
DIRTYCODE_API uint32_t TagFieldGetEpoch(const char *data, uint32_t defval);

// get date
DIRTYCODE_API int32_t TagFieldGetDate(const char *data, int32_t *pyear, int32_t *pmonth, int32_t *pday, int32_t *phour, int32_t *pminute, int32_t *psecond, int32_t gmt);

// get floating point number
DIRTYCODE_API float TagFieldGetFloat(const char *pData, float fDefval);

// get the raw, undecoded value of a field
DIRTYCODE_API int32_t TagFieldGetRaw(const char *pData, char *pBuffer, int32_t iBuflen, const char *pDefval);

// find next tag field in given record
DIRTYCODE_API const char *TagFieldFindNext(const char *pRecord, char *pName, int32_t iNameSize);

// Populate pPtrs with indexes into a structure for each attr defined by pPat
DIRTYCODE_API int32_t TagFieldGetStructureOffsets(const char *pPat, TagFieldStructPtrT *pPtrs, int32_t iNumPtrs);

#ifdef __cplusplus
}
#endif

//@}

#endif // _tagfield_h

