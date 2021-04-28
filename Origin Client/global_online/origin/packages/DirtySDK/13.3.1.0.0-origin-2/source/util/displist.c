/*H*************************************************************************************************/
/*!
    \File    displist.c

    \Description
        This module implements a simple shared list structure appropriate for use
        in a UI display environment. The list supports both filtering and sorting
        through user supplied functions. Multiple modules can share a list as long
        as they both operate within the same thread.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2000-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/15/00 (GWS) First Version
    \Version    1.1        01/29/01 (GWS) Fixed dumb memory bug in expand

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/util/displist.h"
#include "DirtySDK/util/sort.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! list item record
struct DispListT
{
    void *data;         //!< pointer to list item
};

//! module state
struct DispListRef
{
    // module memory group
    int32_t memgroup;       //!< module mem group id
    void *memgrpusrdata;    //!< user data associated with mem group
    
    int32_t addcnt;         //!< number of records to add when expansion needed
    int32_t addpct;         //!< percent of records to add when expansion needed

    int32_t change;         //!< has record list changed since last update
    int32_t dirty;          //!< has record list been made dirty since last update

    int32_t avail;          //!< records available for use
    int32_t shown;          //!< number of visible records (non filtered)
    int32_t count;          //!< total number of records

    int32_t filtcon;        //!< constant for user filter proc
    void *filtref;      //!< constant for user filter proc
    int32_t (*filtptr)(void *filtref, int32_t filtcon, void *recptr);  //!< user filter proc

    int32_t sortcon;        //!< constant for user sort proc
    void *sortref;      //!< constant for user sort proc
    int32_t (*sortptr)(void *sortref, int32_t sortcon, void *recptr1, void *recptr2); //!< user sort proc

	void *dataref;		//!< reference to data storage (caller user)

    DispListT *free;    //!< linked list of free records
    DispListT *list;    //!< start of record buffer
    DispListT **view;   //!< current sorted view buffer
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    DispListExpand

    \Description
        Expand the display record list

    \Input *ref     - reference pointer

    \Output
        int32_t         - free records

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
static int32_t DispListExpand(DispListRef *ref)
{
    int32_t size;
    DispListT *list;
    DispListT **view;

    // figure out the expand count
    if (ref->addpct < 1)
        size = (ref->addcnt < 1 ? 1 : ref->addcnt);
    else if (ref->addcnt == 0)
        size = (ref->count*100)/ref->addpct;
    else
        size = (ref->addcnt+(ref->count*100)/ref->addpct)/2;
    if (size < 2)
        size = 2;

    // add in leading/trailing record
    size += 2;

    // allocate new elements
    list = DirtyMemAlloc(size*sizeof(list[0]), DISPLIST_MEMID, ref->memgroup, ref->memgrpusrdata);
    if (list == NULL)
        return(0);
    memset(list, 0, size*sizeof(list[0]));

    // setup head record
    list[0].data = ref->list;
    // mark end of record
    // this code aims to set the pointer to the value -1
    // without generating a warning on any of our compilers, including the 64-bit one for PC
    list[size-1].data = (void *)(intptr_t) - 1;

    // insert into linked list
    ref->list = list;

    // put records into free list
    for (size = 1; list[size].data == NULL; ++size) {
        list[size].data = ref->free;
        ref->free = &list[size];
    }

    // add to the available count
    // (note that size is adjusted by preceding loop and is only
    // off by one -- the starting index)
    ref->avail += size-1;

    // reallocate the view index
    view = DirtyMemAlloc((ref->count+ref->avail)*2*sizeof(view[0]), DISPLIST_MEMID, ref->memgroup, ref->memgrpusrdata);
    if (ref->view != NULL) {
        memcpy(view, ref->view, ref->count*sizeof(view[0]));
        DirtyMemFree(ref->view, DISPLIST_MEMID, ref->memgroup, ref->memgrpusrdata);
    }
    ref->view = view;

    // return expand count
    return(size-1);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListCompare

    \Description
        Call the user sort function

    \Input *zref    -
    \Input *p1      -
    \Input *p2      -

    \Output
        int32_t         - compare results (from user compare function)

    \Notes
        Takes care of parameter mangling from msort

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
static int32_t DispListCompare(void *zref, const void *p1, const void *p2)
{
    // deref the items
    DispListRef *ref = (DispListRef *)zref;
    DispListT *q1 = *((DispListT **)p1);
    DispListT *q2 = *((DispListT **)p2);

    // pass to compare function
    return(ref->sortptr(ref->sortref, ref->sortcon, q1->data, q2->data));
}

/*F*************************************************************************************************/
/*!
    \Function    DispListCreate

    \Description
        Create a new display list

    \Input basis        - initial number of records
    \Input addcnt       - number of record to add, when expansion needed
    \Input addpct       - percent of records to add during expansion

    \Output
        DispListRef *   - reference pointer (need for all other functions)

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
DispListRef *DispListCreate(int32_t basis, int32_t addcnt, int32_t addpct)
{
    DispListRef *ref;
    int32_t memgroup;
    void *memgrpusrdata;
    
    // Query current mem group data
    DirtyMemGroupQuery(&memgroup, &memgrpusrdata);

    // allocate module state
    ref = DirtyMemAlloc(sizeof(*ref), DISPLIST_MEMID, memgroup, memgrpusrdata);
    memset(ref, 0, sizeof(*ref));
    ref->memgroup = memgroup;
    ref->memgrpusrdata = memgrpusrdata;

    // allocate initial list
    ref->addcnt = basis;
    DispListExpand(ref);

    // save the expansion info
    ref->addcnt = addcnt;
    ref->addpct = addpct;

    // return the state
    return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListDestroy

    \Description
        Destroy a display list

    \Input *ref     - reference pointer

    \Output
        None.

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void DispListDestroy(DispListRef *ref)
{
    // delete all the record lists
    while (ref->list != NULL) {
        DispListT *kill = ref->list;
        ref->list = ref->list[0].data;
        DirtyMemFree(kill, DISPLIST_MEMID, ref->memgroup, ref->memgrpusrdata);
    }
    // done with view
    DirtyMemFree(ref->view, DISPLIST_MEMID, ref->memgroup, ref->memgrpusrdata);
    // done with state
    DirtyMemFree(ref, DISPLIST_MEMID, ref->memgroup, ref->memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListClear

    \Description
        Clear all items from existing list

    \Input *ref     - reference pointer

    \Output
        None.

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void DispListClear(DispListRef *ref)
{
    int32_t idx;
    DispListT *item;

    // walk the list and release all records
    for (idx = 0; idx < ref->count; ++idx) {
        item = ref->view[idx];
        // put into free list
        item->data = ref->free;
        ref->free = item;
    }

    // reset the counts
    ref->avail += ref->count;
    ref->count = ref->shown = 0;
    ref->change = 1;
}

/*F*************************************************************************************************/
/*!
    \Function    DispListAdd

    \Description
        Add an item to the display list

    \Input *ref     - reference pointer
    \Input *recptr  - pointer to record to add

    \Output
        DispListT*  - record refernce (NULL=unable to add)

    \Notes
        Caller must maintain storage for record

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
DispListT *DispListAdd(DispListRef *ref, void *recptr)
{
    DispListT *item;

    // add records if needed
    if ((ref->free == NULL) && (DispListExpand(ref) < 1))
        return(NULL);

    // remove from free list
    item = ref->free;
    ref->free = item->data;

    // populate the record
    item->data = recptr;

    // add to view index
    ref->view[ref->count] = item;

    // add to main count
    ref->count += 1;
    ref->avail -= 1;

    // see if need to resort
    if ((ref->filtptr == NULL) || (ref->filtptr(ref->filtref, ref->filtcon, item->data) > 0))
        ref->change += 1;

    // return pointer for reference
    return(item);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListSet

    \Description
        Change the data associated with a record

    \Input *ref     - reference pointer
    \Input *item    - record reference (from add)
    \Input *recptr  - new record pointer

    \Output
        None.

    \Notes
        If item->data == recptr then record was changed and we cannot
        determine its prior visibility and thus must assume it changed

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void DispListSet(DispListRef *ref, DispListT *item, void *recptr)
{
    // see this forces a change
    if ((ref->filtptr == NULL) ||
        (item->data == recptr) ||
        (ref->filtptr(ref->filtref, ref->filtcon, item->data) > 0) ||
        (ref->filtptr(ref->filtref, ref->filtcon, recptr) > 0))
        ref->change += 1;

    // update the record
    item->data = recptr;
}

/*F*************************************************************************************************/
/*!
    \Function    DispListGet

    \Description
        Return data pointer for a record

    \Input *ref     - reference pointer
    \Input *item    - record reference

    \Output
        void *      - record pointer

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void *DispListGet(DispListRef *ref, DispListT *item)
{
    return(item ? item->data : NULL);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListDelByIndex

    \Description
        Delete an item  given an index to it.

    \Input *ref     - reference pointer
    \Input *idx     - record reference (from add)

    \Output
        void *      - record pointer, the data held by item.

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void *DispListDelByIndex(DispListRef *ref,  int32_t idx)
{
    void *data;
    DispListT *item;
    // see if it exists
    if (idx >= ref->count)
        return(NULL);
    if (ref->view[idx] == NULL)
        return(NULL);

    item = ref->view[idx];

    // mark as changed if item was visible
    if (idx < ref->shown)
        ref->change += 1, ref->shown -= 1;
    // delete from list
    ref->count -= 1;
    for (; idx < ref->count; ++idx)
        ref->view[idx] = ref->view[idx+1];

    // save the data pointer
	
	data = item->data;

    // put into free list
    item->data = ref->free;
    ref->free = item;

    // add to available count
    ref->avail += 1;

    // all done
    return(data);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListDel

    \Description
        Delete an item from the list

    \Input *ref     - reference pointer
    \Input *item    - record reference (from add)

    \Output
        void *      - record pointer

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void *DispListDel(DispListRef *ref, DispListT *item)
{
    int32_t idx;
    // need to locate item in list
    for (idx = 0; (idx < ref->count) && (ref->view[idx] != item); ++idx)
        ;
    return(DispListDelByIndex(ref, idx));
}



/*F*************************************************************************************************/
/*!
    \Function    DispListCount

    \Description
        Return total number of records (visible+invisible)

    \Input *ref     - reference pointer

    \Output
        int32_t         - number of records

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t DispListCount(DispListRef *ref)
{
    // return total record count
    return(ref->count);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListShown

    \Description
        Return total number of visible records

    \Input *ref     - reference pointer

    \Output
        int32_t         - number of visible records

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t DispListShown(DispListRef *ref)
{
    // return visible record count
    return(ref->shown);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListIndex

    \Description
        Return a record data pointer by its display index

    \Input *ref     - reference pointer
    \Input index    - record index

    \Output
        void *      - record data pointer (NULL=invalid index)

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void *DispListIndex(DispListRef *ref, int32_t index)
{
    // see the index is valid
    if ((ref == NULL) || (index < 0) || (index >= ref->count))
        return(NULL);

    // return the index
    return(ref->view[index]->data);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListSort

    \Description
        Assign the sort routine for the list

    \Input *ref     - reference pointer
    \Input *sortref - callback value
    \Input sortcon  - callback value
    \Input sortptr  - user sort function

    \Output
        None.

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void DispListSort(DispListRef *ref, void *sortref, int32_t sortcon,
                  int32_t (*sortptr)(void *sortref, int32_t sortcon, void *recptr1, void *recptr2))
{
    if (ref->count > 0)
        ref->change = 1;
    ref->sortref = sortref;
    ref->sortcon = sortcon;
    ref->sortptr = sortptr;
}

/*F*************************************************************************************************/
/*!
    \Function    DispListFilt

    \Description
        Assign the filter routine for the list

    \Input *ref     - reference pointer
    \Input *filtref - callback value
    \Input filtcon  - callback value
    \Input *filtptr - user filter function

    \Output
        None.

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
void DispListFilt(DispListRef *ref, void *filtref, int32_t filtcon,
                  int32_t (*filtptr)(void *filtref, int32_t filtcon, void *recptr))
{
    if (ref->count > 0)
        ref->change = 1;
    ref->filtref = filtref;
    ref->filtcon = filtcon;
    ref->filtptr = filtptr;
}

/*F*************************************************************************************************/
/*!
    \Function    DispListChange

    \Description
        Get/set the list "changed" value

    \Input *ref     - reference pointer
    \Input set      - indicates if the changed value should be set

    \Output
        int32_t         - change value

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t DispListChange(DispListRef *ref, int32_t set)
{
    if (set)
        ref->change += 1;
    return(ref->change > 0);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListDirty

    \Description
        Get/set the list "dirty" value

    \Input *ref     - reference pointer
    \Input set      - indicates if the dirty value should be set

    \Output
        int32_t         - dirty value

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t DispListDirty(DispListRef *ref, int32_t set)
{
    if (set)
        ref->dirty += 1;
    return(ref->dirty > 0);
}

/*F*************************************************************************************************/
/*!
    \Function    DispListOrder

    \Description
        Order the list using the current filter/sort functions

    \Input *ref     - reference pointer

    \Output
        int32_t         - zero=no changes made, positive=list was resorted/refiltered

    \Notes
        None.

    \Version    1.0        03/15/00 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t DispListOrder(DispListRef *ref)
{
    DispListT **src, **dst1, **dst2;

    // see if any change needed
    if (ref->change+ref->dirty == 0)
        return(0);

    // see if it was marked as dirty (but nothing changed)
    if (ref->change == 0) {
        ref->dirty = 0;
        return(1);
    }

    // setup to filter view list
    dst1 = ref->view;
    dst2 = ref->view+ref->count;

    // filter the view list
    for (src = ref->view; src != ref->view+ref->count; ++src) {
        // handle filtering items
        if ((ref->filtptr == NULL) || (ref->filtptr(ref->filtref, ref->filtcon, (*src)->data) > 0))
            *dst1++ = *src;
        else
            *dst2++ = *src;
    }
    ref->shown = dst1 - ref->view;

    // shift back the filtered items
    for (src = ref->view+ref->count; src != dst2; ++src)
        *dst1++ = *src;
    ref->count = dst1 - ref->view;

    // sort the non-filtered items
    if (ref->sortptr != NULL)
		LobbyMSort(ref, ref->view, ref->shown, sizeof(ref->view[0]), &DispListCompare);

    // mark as up to date
    ref->change = 0;
    ref->dirty = 0;
    return(2);
}


/*F*************************************************************************************************/
/*!
    \Function    DispListDataSet

    \Description
        Store data storage pointer (used by calling code)

    \Input *ref     - reference pointer
    \Input *dataref - caller defined

    \Output None.

    \Version    1.0        12/19/2002 (GWS) First Version
*/
/*************************************************************************************************F*/
void DispListDataSet(DispListRef *ref, void *dataref)
{
    ref->dataref = dataref;
}


/*F*************************************************************************************************/
/*!
    \Function    DispListDataGet

    \Description
        Return data storage pointer (used by calling code)

    \Input *ref     - reference pointer

    \Output void* - the data storage constant (used entirely by caller)

    \Version    1.0        12/19/2002 (GWS) First Version
*/
/*************************************************************************************************F*/
void *DispListDataGet(DispListRef *ref)
{
	return(ref->dataref);
}
