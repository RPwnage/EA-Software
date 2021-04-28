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

#ifdef EAFN_SERVER
 #define DirtyMemAlloc(_iSize, _iMemModule, _iMemGroup, _pMemGroupUserData) malloc(_iSize)
 #define DirtyMemFree(_pMem, _iMemModule, _iMemGroup, _pMemGroupUserData) free(_pMem)
 #define DirtyMemGroupQuery() (0)
 #define HASHER_MEMID         (0)
#else
 #include "DirtySDK/dirtysock/dirtymem.h"
#endif

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynames.h"
#include "DirtySDK/util/hasher.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! individual hash record
typedef struct HasherT
{
    struct HasherT *next;       //!< link to next item
    void *data;                 //!< pointer to corresponding data

    //! hash key
    union {
        int32_t val;            //!< numeric key value
        const char *ptr;        //!< key string value
    } key;
} HasherT;

//! enumeration state
typedef struct HashEnumT
{
    int32_t idx;                //!< current index for enumation
    HasherT *curr;              //!< pointer to current record
} HashEnumT;

//! module state
struct HasherRef
{ 
    int32_t memgroup;           //!< module mem group id
    void *memgrpusrdata;        //!< user data associated with mem group
    
    int32_t items;              //!< total records in hash table
    int32_t flush;              //!< index for efficient table flush
    HashEnumT iter;             //!< info for table enumeration

    int32_t hashlen;            //!< length of hash table
    HasherT **hashbuf;          //!< table index

    HasherT *list;              //!< linked list of hash storage blocks
    HasherT *avail;             //!< linked list of available records

    HasherCompareF *compare;    //!< key compare function
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

    \Input initial  - initial number of records
    \Input hashlen  - size of hash table

    \Output HasherRef * - reference pointer (needed for all other functions)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
HasherRef *HasherCreate(int32_t initial, int32_t hashlen)
{
    HasherRef *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp entry parms
    if (initial < 1)
        initial = 1;
    if (hashlen < 16)
        hashlen = 16;

    // allocate module state
    ref = DirtyMemAlloc(sizeof(*ref), HASHER_MEMID, iMemGroup, pMemGroupUserData);
    memset(ref, 0, sizeof(*ref));
    ref->memgroup = iMemGroup;
    ref->memgrpusrdata = pMemGroupUserData;

    // allocate hash table
    ref->hashlen = hashlen;
    ref->hashbuf = DirtyMemAlloc(ref->hashlen*sizeof(ref->hashbuf[0]), HASHER_MEMID, ref->memgroup, ref->memgrpusrdata);
    memset(ref->hashbuf, 0, ref->hashlen*sizeof(ref->hashbuf[0]));

    // set default string compare function
    ref->compare = (HasherCompareF *)DirtyUsernameCompare;

    // allocate initial records
    if (initial > 0)
    {
        HasherExpand(ref, initial);
    }

    // return the state
    return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherDestroy

    \Description
        Destroy the hash table

    \Input *ref     - reference pointer

    \Output None.

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void HasherDestroy(HasherRef *ref)
{
    // destroy item tables
    while (ref->list != NULL)
    {
        HasherT *root = ref->list;
        ref->list = root->next;
        DirtyMemFree(root, HASHER_MEMID, ref->memgroup, ref->memgrpusrdata);
    }

    // destroy hash table
    DirtyMemFree(ref->hashbuf, HASHER_MEMID, ref->memgroup, ref->memgrpusrdata);
    // destroy state
    DirtyMemFree(ref, HASHER_MEMID, ref->memgroup, ref->memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherSetStrCompareFunc

    \Description
        Override the default string compare function for the given hash table.

    \Input *ref     - reference pointer
    \Input *compare - pointer to new compare function

    \Output None.

    \Version    1.0        04/30/03 (DBO) First Version
*/
/*************************************************************************************************F*/
void HasherSetStrCompareFunc(HasherRef *ref, HasherCompareF *compare)
{
    ref->compare = compare;
}


/*F*************************************************************************************************/
/*!
    \Function    HasherClear

    \Description
        Clear an existing hash table

    \Input *ref     - reference pointer

    \Output None.

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void HasherClear(HasherRef *ref)
{
    int32_t idx;
    HasherT *item;

    // reset the item count
    ref->items = 0;

    // remove everything from hash table
    for (idx = 0; idx < ref->hashlen; ++idx)
    {
        // remove from hash table
        while ((item = ref->hashbuf[idx]) != NULL)
        {
            ref->hashbuf[idx] = item->next;
            // put into free table
            item->next = ref->avail;
            ref->avail = item;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    HasherExpand

    \Description
        Add additional records to a hash table

    \Input *ref     - reference pointer
    \Input expand   - number of records to add

    \Output int32_t     - number of free records

    \Notes
        Not normally called by user (table is auto-expanded)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HasherExpand(HasherRef *ref, int32_t expand)
{
    HasherT *table;
    HasherT *item;

    // put into range
    if (expand < 1)
    {
        expand = 1;
    }
    // add extra header record
    expand += 1;

    // allocate a new table
    table = DirtyMemAlloc(expand*sizeof(*table), HASHER_MEMID, ref->memgroup, ref->memgrpusrdata);
    if (table == NULL)
    {
        return(-1);
    }

    // add first record to master list
    table->next = ref->list;
    ref->list = table;

    // add remaining records to available list
    for (item = table+1; item != table+expand; ++item)
    {
        item->next = ref->avail;
        ref->avail = item;
    }

    // return the count
    return(expand-1);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherFlush

    \Description
        Flush the hash records one at a time

    \Input *ref     - reference pointer

    \Output void *  - record pointer (NULL=no more records, table cleared)

    \Notes
        Similar to clear, but allows caller to get back records

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HasherFlush(HasherRef *ref)
{
    HasherT *item;

    // all done if nothing in table
    if (ref->items == 0)
    {
        return(NULL);
    }

    // find the next item
    for (; ref->hashbuf[ref->flush] == NULL; ref->flush = (ref->flush+1)%ref->hashlen)
        ;

    // remove item from hash
    item = ref->hashbuf[ref->flush];
    ref->hashbuf[ref->flush] = item->next;
    item->next = ref->avail;
    ref->avail = item;
    ref->items -= 1;

    // return data pointer
    return(item->data);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherCount

    \Description
        Return count of items in hash table

    \Input *ref     - reference pointer

    \Output Number of items in hash table

    \Version    1.0        07/29/03 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HasherCount(HasherRef *ref)
{
    return(ref->items);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrHash

    \Description
        Calculate hash key for string

    \Input *ref     - reference pointer
    \Input *key     - null terminated string

    \Output int32_t     - hash value

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t HashStrHash(HasherRef *ref, const char *key)
{
    int32_t hash;

    // build the hash
    for (hash = 0; *key != 0; ++key)
    {
        if (*key > ' ')
        {
            hash = 131*hash + (*key & 0x5f);
        }
    }

    // make sure its not negative
    hash &= 0x7fffffff;

    // return the hash
    return(hash % ref->hashlen);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrAdd

    \Description
        Add new record to hash table

    \Input *ref     - reference pointer
    \Input *key     - record key
    \Input *data    - record data

    \Output int32_t     - negative=failed, positive=success

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HashStrAdd(HasherRef *ref, const char *key, void *data)
{
    HasherT *item;
    int32_t hash = HashStrHash(ref, key);

    // see if we need to expand
    if ((ref->avail == NULL) && (HasherExpand(ref, (ref->items*3+10)/2) <= 0))
    {
        return(-1);
    }

    // get a free record
    item = ref->avail;
    ref->avail = item->next;

    // populate the record
    item->key.ptr = key;
    item->data = data;

    // put into hash table
    item->next = ref->hashbuf[hash];
    ref->hashbuf[hash] = item;

    // count the item
    ref->items += 1;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrReplace

    \Description
        Replace an existing record in the hash table

    \Input *ref     - reference pointer
    \Input *key     - record key
    \Input *data    - record data

    \Output void *  - old record pointer (NULL=no match)

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void *HashStrReplace(HasherRef *ref, const char *key, void *data)
{
    void *old;
    HasherT *item;
    int32_t hash = HashStrHash(ref, key);

    for (item = ref->hashbuf[hash]; item != NULL; item = item->next)
    {
        if (ref->compare((unsigned char *)item->key.ptr, (unsigned char *)key) == 0)
        {
            old = item->data;
            item->data = data;
            return(old);
        }
    }

    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function    HashStrFind

    \Description
        Find existing item within hash table

    \Input *ref     - reference pointer
    \Input *key     - record key

    \Output void *  - record pointer (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashStrFind(HasherRef *ref, const char *key)
{
    HasherT *item;
    int32_t hash = HashStrHash(ref, key);

    // search for match
    for (item = ref->hashbuf[hash]; item != NULL; item = item->next)
    {
        if (ref->compare((unsigned char *)item->key.ptr, (unsigned char *)key) == 0)
        {
            return(item->data);
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

    \Input *ref     - reference pointer
    \Input *key     - item to delete

    \Output void *  - pointer to record data (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashStrDel(HasherRef *ref, const char *key)
{
    HasherT *item, **link;
    int32_t hash = HashStrHash(ref, key);

    // search for match
    for (link = &ref->hashbuf[hash];; link = &(*link)->next)
    {
        item = *link;
        // see if we hit end of list
        if (item == NULL)
        {
            return(NULL);
        }
        // check for match
        if (ref->compare((unsigned char *)(*link)->key.ptr, (unsigned char *)key) == 0)
        {
            break;
        }
    }

    // remove item from hash
    *link = item->next;
    if (ref->iter.curr == item)
    {
        ref->iter.curr = item->next;
    }

    // add to available list
    item->next = ref->avail;
    ref->avail = item;

    // count the delete
    ref->items -= 1;
    return(item->data);
}

/*F*************************************************************************************************/
/*!
    \Function    HashNumAdd

    \Description
        Add new record to hash table

    \Input *ref     - reference pointer
    \Input key      - record key
    \Input *data    - record data

    \Output int32_t     - negative=failed, positive=success

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t HashNumAdd(HasherRef *ref, int32_t key, void *data)
{
    HasherT *item;
    int32_t hash = (key >= 0 ? key : -key) % ref->hashlen;

    // see if we need to expand
    if ((ref->avail == NULL) && (HasherExpand(ref, (ref->items*3+10)/2) <= 0))
    {
        return(-1);
    }

    // get a free record
    item = ref->avail;
    ref->avail = item->next;

    // populate the record
    item->key.val = key;
    item->data = data;

    // put into hash table
    item->next = ref->hashbuf[hash];
    ref->hashbuf[hash] = item;

    // count the item
    ref->items += 1;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    HashNumReplace

    \Description
        Replace an existing record in the hash table

    \Input *ref     - reference pointer
    \Input *key     - record key
    \Input *data    - record data

    \Output void *  - old record pointer (NULL=no match)

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void *HashNumReplace(HasherRef *ref, int32_t key, void *data)
{
    void *old;
    HasherT *item;
    int32_t hash = (key >= 0 ? key : -key) % ref->hashlen;

    for (item = ref->hashbuf[hash]; item != NULL; item = item->next)
    {
        if (item->key.val == key)
        {
            old = item->data;
            item->data = data;
            return(old);
        }
    }

    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function    HashNumFind

    \Description
        Find existing item within hash table

    \Input *ref     - reference pointer
    \Input key      - record key

    \Output void *  - record pointer (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashNumFind(HasherRef *ref, int32_t key)
{
    HasherT *item;
    int32_t hash = (key >= 0 ? key : -key) % ref->hashlen;

    // search for match
    for (item = ref->hashbuf[hash]; item != NULL; item = item->next)
    {
        if (item->key.val == key)
        {
            return(item->data);
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

    \Input *ref     - reference pointer
    \Input key      - item to delete

    \Output void *  - pointer to record data (NULL=no such record)

    \Version    1.0        03/12/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void *HashNumDel(HasherRef *ref, int32_t key)
{
    HasherT *item, **link;
    int32_t hash = (key >= 0 ? key : -key) % ref->hashlen;

    // search for match
    for (link = &ref->hashbuf[hash];; link = &(*link)->next)
    {
        item = *link;
        // see if we hit end of list
        if (item == NULL)
        {
            return(NULL);
        }
        // check for match
        if ((*link)->key.val == key)
        {
            break;
        }
    }

    // remove item from hash
    *link = item->next;
    if (ref->iter.curr == item)
    {
        ref->iter.curr = item->next;
    }

    // add to available list
    item->next = ref->avail;
    ref->avail = item;

    // count the delete
    ref->items -= 1;
    return(item->data);
}

/*F*************************************************************************************************/
/*!
    \Function    HasherEnumInit

    \Description
        Initialize enumeration structure prior to calls to HasherEnum

    \Input *ref     - reference pointer

    \Output None

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void HasherEnumInit(HasherRef *ref)
{
    ref->iter.idx = 0;
    ref->iter.curr = NULL;
}

/*F*************************************************************************************************/
/*!
    \Function    HasherEnum

    \Description
        Enumerate the hash records one at a time.

    \Input *ref     - reference pointer
    \Input *key     - if not NULL, return pointer to entry key

    \Output void *  - record pointer (NULL=no such record)

    \Version    1.0        01/15/2003 (DO) First Version
*/
/*************************************************************************************************F*/
void *HasherEnum(HasherRef *ref, void **key)
{
    HasherT *item;

    if (ref->iter.curr == NULL)
    {
        // find the next item
        for (; (ref->iter.idx < ref->hashlen) &&
               (ref->hashbuf[ref->iter.idx] == NULL); ref->iter.idx++)
            ;
        if ( ref->iter.idx >= ref->hashlen )
        {
            return(NULL);
        }
        ref->iter.curr = ref->hashbuf[ref->iter.idx];
        ref->iter.idx++;
    }

    item = ref->iter.curr;
    ref->iter.curr = ref->iter.curr->next;

    if (key)
    {
        *key = (void *)item->key.ptr;
    }

    // return data pointer
    return(item->data);
}

