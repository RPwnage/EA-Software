/*H*************************************************************************************************/
/*!

    \File    lobbysort.c

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


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef EAFN_SERVER
 #define DirtyMemAlloc(_iSize, _iMemModule, _iMemGroup, _pMemGroupUserData) malloc(_iSize)
 #define DirtyMemFree(_pMem, _iMemModule, _iMemGroup, _pMemGroupUserData) free(_pMem)
 #define LOBBYSORT_MEMID (0)
#else
 #include "dirtymem.h"
#endif

#include "platform.h"
#include "lobbysort.h"

/*** Defines ***************************************************************************/

// default stack usage
#ifdef EAFN_SERVER
 #define STACK_ELEM  1024        // max elements in stack implementation
 #define STACK_SWAP  4096        // size of stack based swap buffer
#else
// limit stack usage for console applications
 #define STACK_ELEM  512         // max elements in stack implementation
 #define STACK_SWAP  1024        // size of stack based swap buffer
#endif

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    msort

    \Description
        Perform mergesort on fixed width array

    \Input *refptr  - ref value to pass to compare
    \Input *base    - array base
    \Input nmemb    - array members
    \Input size     - array width
    \Input *compare - compare function

    \Output
        None.

    \Notes
        Preserves ordering between identical keys.  May allow up to (12*nmemb)+size bytes

    \Version    1.0        ??/??/98 (GWS) First Version

*/
/*************************************************************************************************F*/
void LobbyMSort(void *refptr, void *base, int32_t nmemb, int32_t size,
	    int32_t (*compare)(void *refptr, const void *, const void *))
{
    int32_t pass;
    int32_t *rbuf, *rget, *rput, rtmp[STACK_ELEM];
    char **mbuf[2], **msrc, **mdst, *mtmp[2][STACK_ELEM];
    char *xbuf, xtmp[STACK_SWAP];
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // step 1: reject trivial cases
    if (nmemb < 2)
        return;

    // step 2: allocate needed temp arrays
    if (nmemb < (signed)(sizeof(rtmp)/sizeof(rtmp[0]))) {
	    // use stack based storage (more efficient)
	    rbuf = rtmp;
	    mbuf[0] = mtmp[0];
	    mbuf[1] = mtmp[1];
    } else {
	    // this is the run length buffer and merge arrays
        rbuf = (int32_t *) DirtyMemAlloc(nmemb*sizeof(int32_t), LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);
        mbuf[0] = (char **) DirtyMemAlloc(nmemb*sizeof(mbuf[0]), LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);
        mbuf[1] = (char **) DirtyMemAlloc(nmemb*sizeof(mbuf[1]), LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);
    }
    if (size <= (signed)sizeof(xtmp)) {
	    // use stack based storage (more efficient)
	    xbuf = xtmp;
    } else {
	    // this is the element exchange buffer
	    xbuf = (char *) DirtyMemAlloc(size, LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);
    }

    // step 3: build data index buffer and run length buffer
    {
        char *p = (char *)base;
        char *q = p+nmemb*size;
        // put first element into merge buffer
        msrc = mbuf[0];
        *msrc = p;
        p += size;
        // init run length buffer to 1 element run
        rput = rbuf;
        *rput = 1;
        // add pointers to each item and calculate run lengths
        for (; p != q; p += size) {
	        if (compare(refptr, *msrc, p) <= 0)
	            *rput += 1;
	        else
	            *++rput = 1;
	        *++msrc = p;
        }
        // terminate the run length buffer (must double terminate)
        *++rput = 0;
        *++rput = 0;
    }

    // step 4: while runs remain, merge them together
    pass = 0;
    while (rbuf[1] != 0) {
	    // point to start of run buffer
	    rget = rput = rbuf;
	    // determine source and dest buffers
	    msrc = mbuf[(pass & 1)];
	    mdst = mbuf[(pass & 1) ^ 1];
	    // merge the first two runs together
	    for (;;) {
	        // get the run size info
	        int32_t r1 = *rget++;
	        int32_t r2 = *rget++;

	        // setup buffer pointers
	        char **xbeg = msrc;
	        char **xend = xbeg+r1;
	        char **ybeg = xend;
	        char **yend = ybeg+r2;

            // make sure there is work left
	        if (r1 == 0)
    		    break;

	        // merge the elements
	        for (;;) {
		        if (xbeg == xend) {
		            if (ybeg == yend)
    			        break;
		            *mdst++ = *ybeg++;
		        } else if (ybeg == yend) {
		            *mdst++ = *xbeg++;
		        } else if (compare(refptr, *xbeg, *ybeg) <= 0) {
		            *mdst++ = *xbeg++;
		        } else {
		            *mdst++ = *ybeg++;
		        }
	        }

	        // point to next run sequence
	        msrc = yend;
	        // update run counts
	        *rput++ = r1+r2;
	    }
	    // must double terminate run list
	    *rput++ = 0;
	    *rput++ = 0;
	    pass += 1;
    }


    // step 5: must rearrange elements in buffer
    if (pass != 0) {
	    int32_t i, j, k;

#if 0
	    // point to sorted index array
        char *tmp = (char *) DirtyMemAlloc(nmemb*size, iMemGroup, pMemGroupUserData);
	    msrc = mbuf[pass & 1];
	    for (i = 0; i < nmemb; ++i) {
            memcpy(tmp+i*size, msrc[i], size);
        }
        memcpy(base, base, nmemb*size);
        DirtyMemFree(tmp, DirtyMemGroupQuery());
#else
	    // point to sorted index array
	    msrc = mbuf[pass & 1];
	    for (i = 0; i < nmemb; ++i) {
	        // skip completed elements
	        if (msrc[i] == NULL)
    		    continue;
	        // save the first element
	        memcpy(xbuf, msrc[i], size);
	        // shift all other elements into place
	        for (j = i;;) {
		        k = (msrc[j]-(char *)base)/size;
		        if (k == i)
		            break;
		        memcpy(msrc[j], msrc[k], size);
		        msrc[j] = NULL;
		        j = k;
	        }
	        memcpy(msrc[j], xbuf, size);
	        msrc[j] = NULL;
	    }
#endif
    }

    // step 6: release any allocated memory
    if (rbuf != rtmp) {
	    DirtyMemFree(rbuf, LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);
	    DirtyMemFree(mbuf[0], LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);
	    DirtyMemFree(mbuf[1], LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);
    }
    if (xbuf != xtmp)
	    DirtyMemFree(xbuf, LOBBYSORT_MEMID, iMemGroup, pMemGroupUserData);

    // sort is complete
    return;
}

