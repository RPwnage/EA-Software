/*H*************************************************************************************************/
/*!

    \File    displist.h

    \Description
        This module implements a simple shared list structure appropriate for use
        in a UI display environment. The list supports both filtering and sorting
        through user supplied functions. Multiple modules can share a list as long
        as they both operate within the same thread.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2000-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/15/00 (GWS) First Version
    \Version    1.1        01/29/01 (GWS) Fixed dumb memory bug in expand

*/
/*************************************************************************************************H*/

#ifndef _displist_h
#define _displist_h

/*!
\Moduledef DispList DispList
\Modulemember Util
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct DispListRef DispListRef;

typedef struct DispListT DispListT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create a new display list
DIRTYCODE_API DispListRef *DispListCreate(int32_t basis, int32_t addcnt, int32_t addpct);

// destroy existing display list
DIRTYCODE_API void DispListDestroy(DispListRef *ref);

// clear all items out of list
DIRTYCODE_API void DispListClear(DispListRef *ref);

// add an item to the list
DIRTYCODE_API DispListT *DispListAdd(DispListRef *ref, void *recptr);

// change the data pointer for an existing item
DIRTYCODE_API void DispListSet(DispListRef *ref, DispListT *item, void *recptr);

// return data pointer for an item
DIRTYCODE_API void *DispListGet(DispListRef *ref, DispListT *item);

// delete an item from the list
DIRTYCODE_API void *DispListDel(DispListRef *ref, DispListT *item);

// delete an item from the list, given its index.
DIRTYCODE_API void *DispListDelByIndex (DispListRef *ref, int32_t index );

// allow marking as changed
DIRTYCODE_API int32_t DispListChange(DispListRef *ref, int32_t set);

// allow marking as dirty
DIRTYCODE_API int32_t DispListDirty(DispListRef *ref, int32_t set);

// return total item count in list
DIRTYCODE_API int32_t DispListCount(DispListRef *ref);

// return number of shown records
DIRTYCODE_API int32_t DispListShown(DispListRef *ref);

// return a record by index
DIRTYCODE_API void *DispListIndex(DispListRef *ref, int32_t index);

// assign the list sort routine
DIRTYCODE_API void DispListSort(DispListRef *ref, void *sortref, int32_t sortcon,
                  int32_t (*sortptr)(void *sortref, int32_t sortcon, void *recptr1, void *recptr2));

// assign the list filter routine
DIRTYCODE_API void DispListFilt(DispListRef *ref, void *filtref, int32_t filtcon,
                  int32_t (*filtptr)(void *filtref, int32_t filtcon, void *recptr));

// order the according to filter/sort
DIRTYCODE_API int32_t DispListOrder(DispListRef *ref);

// store data storage pointer (used by calling code)
DIRTYCODE_API void DispListDataSet(DispListRef *ref, void *dataref);

// return data storage pointer (used by calling code)
DIRTYCODE_API void *DispListDataGet(DispListRef *ref);


#ifdef __cplusplus
}
#endif

//@}

#endif // _displist_h

