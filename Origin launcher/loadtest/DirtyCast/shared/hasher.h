/*H*************************************************************************************************/
/*!

    \File    hasher.h

    \Description
        This module implements some simple hash table functions. Both numeric
        and string based versions are provided. This module does not do key
        or data storage allocation-- it is up to the caller to keep the keys
        and data in memory that persists for the life of the hash table. The
        string hash functions do not consider case nor spaces when comparing.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2000-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/12/00 (GWS) First Version

*/
/*************************************************************************************************H*/

#ifndef _hasher_h
#define _hasher_h

/*!
\Moduledef Hasher Hasher
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct HasherRef HasherRef;

typedef int32_t (HasherCompareF)(const uint8_t *pKey1, const uint8_t *pKey2);

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create new string lookup hash table
HasherRef *HasherCreate(int32_t iInitial, int32_t iHashLen);

// destroy the tables
void HasherDestroy(HasherRef *pRef);

// set key compare function for given hasher
void HasherSetStrCompareFunc(HasherRef *pRef, HasherCompareF *pCompare);

// clear an existing hash table
void HasherClear(HasherRef *pRef);

// enum hasher records
void *HasherFlush(HasherRef *pRef);

// add more records to the table
int32_t HasherExpand(HasherRef *pRef, int32_t iExpand);

// Return count of items in hash table
int32_t HasherCount(HasherRef *pRef);

// add new item to hash table
int32_t HashStrAdd(HasherRef *pRef, const char *pKey, void *pData);

// replace existing item in hash table
void *HashStrReplace(HasherRef *pRef, const char *pKey, void *pData);

// find existing item in table
void *HashStrFind(HasherRef *pRef, const char *pKey);

// delete existing item from hash table
void *HashStrDel(HasherRef *pRef, const char *pKey);

// add new item to hash table
int32_t HashNumAdd(HasherRef *pRef, int32_t iKey, void *pData);

// replace existing item in hash table
void *HashNumReplace(HasherRef *pRef, int32_t iKey, void *pData);

// find existing item in table
void *HashNumFind(HasherRef *pRef, int32_t iKey);

// delete existing item from hash table
void *HashNumDel(HasherRef *pRef, int32_t iKey);

// enumerate over the hash table records
void HasherEnumInit(HasherRef *pRef);
void *HasherEnum(HasherRef *pRef, void **ppKey);

#ifdef __cplusplus
}
#endif

//@}

#endif // _hasher_h
