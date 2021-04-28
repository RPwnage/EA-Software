/*H*************************************************************************************************/
/*!

    \File    lobbysort.h

    \Description
        High performance memory based merge-sort routine. Does temporarily
        require twice the memory of the sort array (though this is not usually
        a problem as long as the sort array is small objects such as pointers
        or small structures). Performance is O(n log n) which allows it to 
        provide very consistant and fast performance for even extremely large
        data sets. This version is optimized so that mostly-sorted data gets
        completely sorted in very few steps.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Version    1.0        ??/??/98 (GWS) First Version

*/
/*************************************************************************************************H*/

#ifndef _lobbysort_h
#define _lobbysort_h

/*!
\Module Lobby
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! user sort function type
typedef int32_t (LobbySortFuncT)(void *pRef, const void *, const void *);

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

void LobbyMSort(void *refptr, void *base, int32_t nmemb, int32_t size, LobbySortFuncT *pSortFunc);
                                                                                          
#ifdef __cplusplus
}
#endif

//@}

#endif // _lobbysort_h
