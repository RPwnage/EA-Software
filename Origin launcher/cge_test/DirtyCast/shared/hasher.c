/*H*************************************************************************************************/
/*!
    \File    hasher.c

    \Description
        This module implements some simple hash table functions. Both numeric
        and string based versions are provided. This module does not do key
        or data storage allocation-- it is up to the caller to keep the keys
        and data in memory that persists for the life of the hash table. The
        string hash functions do not consider case nor spaces when comparing.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2000-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynames.h"
#include "hasher.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! individual hash record
typedef struct HasherT
{
    struct HasherT *pNext;       //!< link to next item
    void *pData;                 //!< pointer to corresponding data

    //! hash key
    union {
        int32_t iVal;            //!< numeric key value
        const char *pPtr;        //!< key string value
    } Key;
} HasherT;

//! enumeration state
typedef struct HashEnumT
{
    int32_t iIdx;                //!< current index for enumation
    HasherT *pCurr;              //!< pointer to current record
} HashEnumT;

//! module state
struct HasherRef
{ 
    int32_t iMemGroup;           //!< module mem group id
    void *pMemGroupUserData;     //!< user data associated with mem group

    int32_t iItems;              //!< total records in hash table
    int32_t iFlush;              //!< index for efficient table flush
    HashEnumT Iter;              //!< info for table enumeration

    int32_t iHashLen;            //!< length of hash table
    HasherT **ppHashBuf;         //!< table index

    HasherT *pList;              //!< linked list of hash storage blocks
    HasherT *pAvail;             //!< linked list of available records

    HasherCompareF *pCompare;    //!< key compare function
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    HasherCreate

    \Description
        Create new hash table

    \Input iInitial     - initial number of records
    \Input iHashLen     - size of hash table

    \Output HasherRef * - reference pointer (needed for all other functions)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
HasherRef *HasherCreate(int32_t iInitial, int32_t iHashLen)
{
    HasherRef *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp entry parms
    if (iInitial < 1)
        iInitial = 1;
    if (iHashLen < 16)
        iHashLen = 16;

    // allocate module state
    pRef = (HasherRef *)DirtyMemAlloc(sizeof(*pRef), HASHER_MEMID, iMemGroup, pMemGroupUserData);
    ds_memclr(pRef, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;

    // allocate hash table
    pRef->iHashLen = iHashLen;
    pRef->ppHashBuf = (HasherT **)DirtyMemAlloc(pRef->iHashLen*sizeof(pRef->ppHashBuf[0]), HASHER_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    ds_memclr(pRef->ppHashBuf, pRef->iHashLen*sizeof(pRef->ppHashBuf[0]));

    // set default string compare function
    pRef->pCompare = (HasherCompareF *)DirtyUsernameCompare;

    // allocate initial records
    if (iInitial > 0)
    {
        HasherExpand(pRef, iInitial);
    }

    // return the state
    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherDestroy

    \Description
        Destroy the hash table

    \Input *pRef    - reference pointer

    \Output None.

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void HasherDestroy(HasherRef *pRef)
{
    // destroy item tables
    while (pRef->pList != NULL)
    {
        HasherT *pRoot = pRef->pList;
        pRef->pList = pRoot->pNext;
        DirtyMemFree(pRoot, HASHER_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }

    // destroy hash table
    DirtyMemFree(pRef->ppHashBuf, HASHER_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    // destroy state
    DirtyMemFree(pRef, HASHER_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherSetStrCompareFunc

    \Description
        Override the default string compare function for the given hash table.

    \Input *pRef     - reference pointer
    \Input *pCompare - pointer to new compare function

    \Output None.

    \Version    1.0        04/30/03 (DBO) First Version
*/
/*************************************************************************************************F*/
void HasherSetStrCompareFunc(HasherRef *pRef, HasherCompareF *pCompare)
{
    pRef->pCompare = pCompare;
}


/*F*************************************************************************************************/
/*!
    \Function    HasherClear

    \Description
        Clear an existing hash table

    \Input *pRef     - reference pointer

    \Output None.

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void HasherClear(HasherRef *pRef)
{
    int32_t iIdx;
    HasherT *pItem;

    // reset the item count
    pRef->iItems = 0;

    // remove everything from hash table
    for (iIdx = 0; iIdx < pRef->iHashLen; ++iIdx)
    {
        // remove from hash table
        while ((pItem = pRef->ppHashBuf[iIdx]) != NULL)
        {
            pRef->ppHashBuf[iIdx] = pItem->pNext;
            // put into free table
            pItem->pNext = pRef->pAvail;
            pRef->pAvail = pItem;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    HasherExpand

    \Description
        Add additional records to a hash table

    \Input *pRef     - reference pointer
    \Input iExpand   - number of records to add

    \Output int32_t  - number of free records

    \Notes
        Not normally called by user (table is auto-expanded)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HasherExpand(HasherRef *pRef, int32_t iExpand)
{
    HasherT *pTable;
    HasherT *pItem;

    // put into range
    if (iExpand < 1)
    {
        iExpand = 1;
    }
    // add extra header record
    iExpand += 1;

    // allocate a new table
    pTable = (HasherT *)DirtyMemAlloc(iExpand*sizeof(*pTable), HASHER_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    if (pTable == NULL)
    {
        return(-1);
    }

    // add first record to master list
    pTable->pNext = pRef->pList;
    pRef->pList = pTable;

    // add remaining records to available list
    for (pItem = pTable+1; pItem != pTable+iExpand; ++pItem)
    {
        pItem->pNext = pRef->pAvail;
        pRef->pAvail = pItem;
    }

    // return the count
    return(iExpand-1);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherFlush

    \Description
        Flush the hash records one at a time

    \Input *pRef    - reference pointer

    \Output void *  - record pointer (NULL=no more records, table cleared)

    \Notes
        Similar to clear, but allows caller to get back records

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HasherFlush(HasherRef *pRef)
{
    HasherT *pItem;

    // all done if nothing in table
    if (pRef->iItems == 0)
    {
        return(NULL);
    }

    // find the next item
    for (; pRef->ppHashBuf[pRef->iFlush] == NULL; pRef->iFlush = (pRef->iFlush+1)%pRef->iHashLen)
        ;

    // remove item from hash
    pItem = pRef->ppHashBuf[pRef->iFlush];
    pRef->ppHashBuf[pRef->iFlush] = pItem->pNext;
    pItem->pNext = pRef->pAvail;
    pRef->pAvail = pItem;
    pRef->iItems -= 1;

    // return data pointer
    return(pItem->pData);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherCount

    \Description
        Return count of items in hash table

    \Input *pRef    - reference pointer

    \Output Number of items in hash table

    \Version    1.0        07/29/03 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HasherCount(HasherRef *pRef)
{
    return(pRef->iItems);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrHash

    \Description
        Calculate hash key for string

    \Input *pRef    - reference pointer
    \Input *pKey    - null terminated string

    \Output int32_t - hash value

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t HashStrHash(HasherRef *pRef, const char *pKey)
{
    int32_t iHash;

    // build the hash
    for (iHash = 0; *pKey != 0; ++pKey)
    {
        if (*pKey > ' ')
        {
            iHash = 131*iHash + (*pKey & 0x5f);
        }
    }

    // make sure its not negative
    iHash &= 0x7fffffff;

    // return the hash
    return(iHash % pRef->iHashLen);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrAdd

    \Description
        Add new record to hash table

    \Input *pRef    - reference pointer
    \Input *pKey    - record key
    \Input *pData   - record data

    \Output int32_t - negative=failed, positive=success

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HashStrAdd(HasherRef *pRef, const char *pKey, void *pData)
{
    HasherT *pItem;
    int32_t iHash = HashStrHash(pRef, pKey);

    // see if we need to expand
    if ((pRef->pAvail == NULL) && (HasherExpand(pRef, (pRef->iItems*3+10)/2) <= 0))
    {
        return(-1);
    }

    // get a free record
    pItem = pRef->pAvail;
    pRef->pAvail = pItem->pNext;

    // populate the record
    pItem->Key.pPtr = pKey;
    pItem->pData = pData;

    // put into hash table
    pItem->pNext = pRef->ppHashBuf[iHash];
    pRef->ppHashBuf[iHash] = pItem;

    // count the item
    pRef->iItems += 1;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrReplace

    \Description
        Replace an existing record in the hash table

    \Input *pRef    - reference pointer
    \Input *pKey    - record key
    \Input *pData   - record data

    \Output void *  - old record pointer (NULL=no match)

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void *HashStrReplace(HasherRef *pRef, const char *pKey, void *pData)
{
    void *pOld;
    HasherT *pItem;
    int32_t iHash = HashStrHash(pRef, pKey);

    for (pItem = pRef->ppHashBuf[iHash]; pItem != NULL; pItem = pItem->pNext)
    {
        if (pRef->pCompare((const uint8_t *)pItem->Key.pPtr, (const uint8_t *)pKey) == 0)
        {
            pOld = pItem->pData;
            pItem->pData = pData;
            return(pOld);
        }
    }

    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrFind

    \Description
        Find existing item within hash table

    \Input *pRef    - reference pointer
    \Input *pKey    - record key

    \Output void *  - record pointer (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashStrFind(HasherRef *pRef, const char *pKey)
{
    HasherT *pItem;
    int32_t iHash = HashStrHash(pRef, pKey);

    // search for match
    for (pItem = pRef->ppHashBuf[iHash]; pItem != NULL; pItem = pItem->pNext)
    {
        if (pRef->pCompare((const uint8_t *)pItem->Key.pPtr, (const uint8_t *)pKey) == 0)
        {
            return(pItem->pData);
        }
    }

    // no match
    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrDel

    \Description
        Delete existing record from hash table

    \Input *pRef    - reference pointer
    \Input *pKey    - item to delete

    \Output void *  - pointer to record data (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashStrDel(HasherRef *pRef, const char *pKey)
{
    HasherT *pItem, **ppLink;
    int32_t iHash = HashStrHash(pRef, pKey);

    // search for match
    for (ppLink = &pRef->ppHashBuf[iHash];; ppLink = &(*ppLink)->pNext)
    {
        pItem = *ppLink;
        // see if we hit end of list
        if (pItem == NULL)
        {
            return(NULL);
        }
        // check for match
        if (pRef->pCompare((const uint8_t *)(*ppLink)->Key.pPtr, (const uint8_t *)pKey) == 0)
        {
            break;
        }
    }

    // remove item from hash
    *ppLink = pItem->pNext;
    if (pRef->Iter.pCurr == pItem)
    {
        pRef->Iter.pCurr = pItem->pNext;
    }

    // add to available list
    pItem->pNext = pRef->pAvail;
    pRef->pAvail = pItem;

    // count the delete
    pRef->iItems -= 1;
    return(pItem->pData);
}

/*F*************************************************************************************************/
/*!
    \Function    HashNumAdd

    \Description
        Add new record to hash table

    \Input *pRef    - reference pointer
    \Input iKey     - record key
    \Input *pData   - record data

    \Output int32_t - negative=failed, positive=success

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HashNumAdd(HasherRef *pRef, int32_t iKey, void *pData)
{
    HasherT *pItem;
    int32_t iHash = (iKey >= 0 ? iKey : -iKey) % pRef->iHashLen;

    // see if we need to expand
    if ((pRef->pAvail == NULL) && (HasherExpand(pRef, (pRef->iItems*3+10)/2) <= 0))
    {
        return(-1);
    }

    // get a free record
    pItem = pRef->pAvail;
    pRef->pAvail = pItem->pNext;

    // populate the record
    pItem->Key.iVal = iKey;
    pItem->pData = pData;

    // put into hash table
    pItem->pNext = pRef->ppHashBuf[iHash];
    pRef->ppHashBuf[iHash] = pItem;

    // count the item
    pRef->iItems += 1;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    HashNumReplace

    \Description
        Replace an existing record in the hash table

    \Input *pRef    - reference pointer
    \Input iKey     - record key
    \Input *pData   - record data

    \Output void *  - old record pointer (NULL=no match)

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void *HashNumReplace(HasherRef *pRef, int32_t iKey, void *pData)
{
    void *pOld;
    HasherT *pItem;
    int32_t iHash = (iKey >= 0 ? iKey : -iKey) % pRef->iHashLen;

    for (pItem = pRef->ppHashBuf[iHash]; pItem != NULL; pItem = pItem->pNext)
    {
        if (pItem->Key.iVal == iKey)
        {
            pOld = pItem->pData;
            pItem->pData = pData;
            return(pOld);
        }
    }

    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function    HashNumFind

    \Description
        Find existing item within hash table

    \Input *pRef    - reference pointer
    \Input iKey     - record key

    \Output void *  - record pointer (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashNumFind(HasherRef *pRef, int32_t iKey)
{
    HasherT *pItem;
    int32_t iHash = (iKey >= 0 ? iKey : -iKey) % pRef->iHashLen;

    // search for match
    for (pItem = pRef->ppHashBuf[iHash]; pItem != NULL; pItem = pItem->pNext)
    {
        if (pItem->Key.iVal == iKey)
        {
            return(pItem->pData);
        }
    }

    // no match
    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function    HashNumDel

    \Description
        Delete existing record from hash table

    \Input *pRef    - reference pointer
    \Input iKey     - item to delete

    \Output void *  - pointer to record data (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashNumDel(HasherRef *pRef, int32_t iKey)
{
    HasherT *pItem, **ppLink;
    int32_t iHash = (iKey >= 0 ? iKey : -iKey) % pRef->iHashLen;

    // search for match
    for (ppLink = &pRef->ppHashBuf[iHash];; ppLink = &(*ppLink)->pNext)
    {
        pItem = *ppLink;
        // see if we hit end of list
        if (pItem == NULL)
        {
            return(NULL);
        }
        // check for match
        if ((*ppLink)->Key.iVal == iKey)
        {
            break;
        }
    }

    // remove item from hash
    *ppLink = pItem->pNext;
    if (pRef->Iter.pCurr == pItem)
    {
        pRef->Iter.pCurr = pItem->pNext;
    }

    // add to available list
    pItem->pNext = pRef->pAvail;
    pRef->pAvail = pItem;

    // count the delete
    pRef->iItems -= 1;
    return(pItem->pData);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherEnumInit

    \Description
        Initialize enumeration structure prior to calls to HasherEnum

    \Input *pRef    - reference pointer

    \Output None

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void HasherEnumInit(HasherRef *pRef)
{
    pRef->Iter.iIdx = 0;
    pRef->Iter.pCurr = NULL;
}

/*F*************************************************************************************************/
/*!
    \Function    HasherEnum

    \Description
        Enumerate the hash records one at a time.

    \Input *pRef    - reference pointer
    \Input **ppKey  - if not NULL, return pointer to entry key

    \Output void *  - record pointer (NULL=no such record)

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void *HasherEnum(HasherRef *pRef, void **ppKey)
{
    HasherT *pItem;

    if (pRef->Iter.pCurr == NULL)
    {
        // find the next item
        for (; (pRef->Iter.iIdx < pRef->iHashLen) &&
               (pRef->ppHashBuf[pRef->Iter.iIdx] == NULL); pRef->Iter.iIdx++)
            ;
        if (pRef->Iter.iIdx >= pRef->iHashLen)
        {
            return(NULL);
        }
        pRef->Iter.pCurr = pRef->ppHashBuf[pRef->Iter.iIdx];
        pRef->Iter.iIdx++;
    }

    pItem = pRef->Iter.pCurr;
    pRef->Iter.pCurr = pRef->Iter.pCurr->pNext;

    if (ppKey != NULL)
    {
        *ppKey = (void *)pItem->Key.pPtr;
    }

    // return data pointer
    return(pItem->pData);
}

