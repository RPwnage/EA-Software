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

#include "DirtySDK/dirtysock/dirtynames.h"
#define LobbyNameCmp DirtyUsernameCompare

/*!
\Module Lobby
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct HasherRef HasherRef;

typedef int32_t HasherCompareF(
        const unsigned char *key1, const unsigned char *key2);

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create new string lookup hash table
HasherRef *HasherCreate(int32_t initial, int32_t hashlen);

// destroy the tables
void HasherDestroy(HasherRef *ref);

// set key compare function for given hasher
void HasherSetStrCompareFunc(HasherRef *ref, HasherCompareF *compare);

// clear an existing hash table
void HasherClear(HasherRef *ref);

// enum hasher records
void *HasherFlush(HasherRef *ref);

// add more records to the table
int32_t HasherExpand(HasherRef *ref, int32_t expand);

// Return count of items in hash table
int32_t HasherCount(HasherRef *ref);

// Return count of items in hash table
int32_t HasherCount(HasherRef *ref);

// add new item to hash table
int32_t HashStrAdd(HasherRef *ref, const char *key, void *data);

// replace existing item in hash table
void *HashStrReplace(HasherRef *ref, const char *key, void *data);

// find existing item in table
void *HashStrFind(HasherRef *ref, const char *key);

// delete existing item from hash table
void *HashStrDel(HasherRef *ref, const char *key);

// add new item to hash table
int32_t HashNumAdd(HasherRef *ref, int32_t key, void *data);

// replace existing item in hash table
void *HashNumReplace(HasherRef *ref, int32_t key, void *data);

// find existing item in table
void *HashNumFind(HasherRef *ref, int32_t key);

// delete existing item from hash table
void *HashNumDel(HasherRef *ref, int32_t key);

// enumerate over the hash table records
void HasherEnumInit(HasherRef *ref);
void *HasherEnum(HasherRef *ref, void **key);

#ifdef __cplusplus
}
#endif

//@}

#endif // _hasher_h
