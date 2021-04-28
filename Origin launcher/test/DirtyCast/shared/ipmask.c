/*H********************************************************************************/
/*!
    \File ipmask.c

    \Description
        This helper class provides a way to check if a given IP address matched
        a set of configured IP/mask combinations.  An IP matches if:

            ((matchAddr & mask) == addr)

        This class allows for configuration of a set of addr/mask pairs that will
        all be used for match comparison.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 02/04/2005 (doneill) First Version
    \Version 07/30/2009 (jbrookes)  C conversion for DirtyCast
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "dirtycast.h"

#include "ipmask.h"

#if DIRTYCODE_LINUX
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>             // struct ifreq
#include <unistd.h>
#endif

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
     \Function _IPMaskAdd

     \Description
         Add a mask to IP mask list

     \Input *pIPMask    - pointer to mask list to add to
     \Input *pMask      - IP mask, if specified separately (may be null)
     \Input *pAddr      - IP address for mask (may include /allow notation if *pMask is null)

     \Version 09/21/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _IPMaskAdd(IPMaskT *pIPMask, const char *pMask, const char *pAddr)
{
    int32_t iIdx;

    if (pIPMask->iCount >= IPMASK_MAX_ADDRS)
    {
        DirtyCastLogPrintf("ipmask: overflow adding masks\n");
        return;
    }

    // allocate a new mask entry
    iIdx = pIPMask->iCount++;

    // if an explicit mask is specified...
    if (pMask != NULL)
    {
        if (pMask[0] == '\0')
        {
            // Mask is missing, so use the previous entry's mask if it exists
            pIPMask->aMasks[iIdx] = (iIdx == 0) ? 0xffffffff : pIPMask->aMasks[iIdx - 1];
        }
        else
        {
            pIPMask->aMasks[iIdx] = TagFieldGetAddress(pMask, 0xffffffff);
        }
    }
    else // support a.b.c.d/m notation
    {
        const char *pBits = pAddr;
        int32_t iBits;

        for (pIPMask->aMasks[iIdx] = 0xffffffff; pBits[0] != '\0'; pBits += 1)
        {
            if (pBits[0] == '/')
            {
                iBits = TagFieldGetNumber(&pBits[1], 0);
                if (iBits < 32)
                {
                    pIPMask->aMasks[iIdx] = ((1 << iBits) - 1) << (32 - iBits);
                }
                break;
            }
        }
    }

    pIPMask->aAddrs[iIdx] = TagFieldGetAddress(pAddr, 0) & pIPMask->aMasks[iIdx];

    DirtyCastLogPrintf("ipmask: added allow %a/%a\n", pIPMask->aAddrs[iIdx], pIPMask->aMasks[iIdx]);
}

/*F********************************************************************************/
/*!
     \Function _IPMaskAddLocal

     \Description
         Add local interface addresses to IP mask list

     \Input *pIPMask    - pointer to mask list to add to
     \Input *pStrAllow  - pointer to string for allow range (cannot be specified as a mask)

     \Todo
         Figure out a way to support this with the interface functionality
         encapsulated in dirtynet* instead of inline (possibly a function to walk
         interface list, with a callback?).

     \Version 09/21/2012 (jbrookes)
*/
/********************************************************************************F*/
#ifdef DIRTYCODE_LINUX
static void _IPMaskAddLocal(IPMaskT *pIPMask, const char *pStrAllow)
{
    struct sockaddr_in HostAddr;
    struct sockaddr_in DestAddr;
    int32_t iSocket;
    char strAddr[64];

    DirtyCastLogPrintf("ipmask: adding allowances for local addresses\n");

    // create a temp socket (must be datagram)
    iSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSocket != -1)
    {
        int32_t iIndex, iCount;
        struct ifreq EndpRec[16];
        struct ifconf EndpList;
        uint32_t uAddr;

        // request list of interfaces
        ds_memclr(&EndpList, sizeof(EndpList));
        EndpList.ifc_req = EndpRec;
        EndpList.ifc_len = sizeof(EndpRec);
        if (ioctl(iSocket, SIOCGIFCONF, &EndpList) >= 0)
        {
            // figure out number and walk the list
            iCount = EndpList.ifc_len / sizeof(EndpRec[0]);
            for (iIndex = 0; iIndex < iCount; iIndex += 1)
            {
                // extract the individual fields
                ds_memcpy(&HostAddr, &EndpRec[iIndex].ifr_addr, sizeof(HostAddr));
                uAddr = ntohl(HostAddr.sin_addr.s_addr);
                ioctl(iSocket, SIOCGIFNETMASK, &EndpRec[iIndex]);
                ds_memcpy(&DestAddr, &EndpRec[iIndex].ifr_broadaddr, sizeof(DestAddr));
                ioctl(iSocket, SIOCGIFFLAGS, &EndpRec[iIndex]);

                NetPrintf(("ipmask: checking interface name=%s, fam=%d, flags=%04x, addr=%08x, mask=%08x\n",
                    EndpRec[iIndex].ifr_name, HostAddr.sin_family,
                    EndpRec[iIndex].ifr_flags, uAddr, ntohl(DestAddr.sin_addr.s_addr)));

                // only consider live inet interfaces
                if ((HostAddr.sin_family == AF_INET) && ((EndpRec[iIndex].ifr_flags & (IFF_LOOPBACK+IFF_UP)) == (IFF_UP)))
                {
                    ds_snzprintf(strAddr, sizeof(strAddr), "%a/%s", uAddr, pStrAllow);
                    _IPMaskAdd(pIPMask, NULL, strAddr);
                }
            }
        }
        // close the socket
        close(iSocket);
    }

}
#endif

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
     \Function IPMaskInit

     \Description
          Initialize the addr/mask pairs from the given TagField encoded config
          string.  These pairs should be encoded in the form:

          <pAddrTag>=<addr1>,<addr2>,...,<addrN>
          <pMaskTag>=<mask1>,<mask2>,...,<maskN>

          Where addrX and maskX are dotted decimal IP addresses
          (eg. 159.153.253.82 or 255.255.255.0)

     \Input *pIPMask        - mask pointer
     \Input *pTagPrefix     - prefix to tag names
     \Input *pAddrTag       - tag to extract addresses from
     \Input *pMaskTag       - tag to extract masks from
     \Input *pCommandTags   - tagfield-encoded command-line config data
     \Input *pConfigTags    - tagfield-encoded config data
     \Input bAddLocal       - add all local entries to mask (linux only)

     \Version 05/30/2005 (doneill)
*/
/********************************************************************************F*/
void IPMaskInit(IPMaskT *pIPMask, const char *pTagPrefix, const char *pAddrTag, const char *pMaskTag, const char *pCommandTags, const char *pConfigTags, uint8_t bAddLocal)
{
    char strAddr[64], strMask[64], strAddrTag[256], strMaskTag[256];
    const char *pAddrs, *pMasks;
    int32_t iIdx, iMax;

    // reset mask
    ds_memclr(pIPMask, sizeof(*pIPMask));

    // set up full tagnames
    ds_snzprintf(strAddrTag, sizeof(strAddrTag), "%s%s", pTagPrefix, pAddrTag);
    ds_snzprintf(strMaskTag, sizeof(strMaskTag), "%s%s", pTagPrefix, pMaskTag);

    pAddrs = DirtyCastConfigFind(pCommandTags, pConfigTags, strAddrTag);
    pMasks = DirtyCastConfigFind(pCommandTags, pConfigTags, strMaskTag);

    // if nothing is specified, allow local access
    if (pAddrs == NULL)
    {
        DirtyCastLogPrintf("ipmask: no address list specified, using default localhost\n");
        pAddrs = "127.0.0.1";
        pMasks = "255.255.255.255";
    }

    // parse addr/mask entries and add them
    iMax = (int32_t)(sizeof(pIPMask->aAddrs) / sizeof(pIPMask->aAddrs[0]));
    for (iIdx = 0; iIdx < iMax; iIdx++)
    {
        TagFieldGetDelim(pAddrs, strAddr, sizeof(strAddr), "", iIdx, ',');
        if (strAddr[0] == '\0')
        {
            break;
        }
        if (pMasks != NULL)
        {
            TagFieldGetDelim(pMasks, strMask, sizeof(strMask), "", iIdx, ',');
            _IPMaskAdd(pIPMask, strMask, strAddr);
        }
        else
        {
            _IPMaskAdd(pIPMask, NULL, strAddr);
        }
    }

    // add allowances for "local" addresses
    #ifdef DIRTYCODE_LINUX
    if (bAddLocal == TRUE)
    {
        _IPMaskAddLocal(pIPMask, "20");
    }
    #endif

    // log total entries added
    DirtyCastLogPrintf("ipmask: added %d entries\n", pIPMask->iCount);
}

/*F********************************************************************************/
/*!
     \Function IPMaskMatch

     \Description
          Check if the given IP address matching the addr/mask pairs configured
          for this instance.

     \Input *pIPMask    - mask pointer
     \Input uAddr       - IP address to check for match.

     \Output
        uint8_t         - TRUE if IP matches; FALSE otherwise.

     \Version 05/30/2005 (doneill)
*/
/********************************************************************************F*/
uint8_t IPMaskMatch(IPMaskT *pIPMask, uint32_t uAddr)
{
    int32_t iIdx;

    // Look for a match on one of the configured pairs.
    for (iIdx = 0; iIdx < pIPMask->iCount; iIdx++)
    {
        if ((uAddr & pIPMask->aMasks[iIdx]) == pIPMask->aAddrs[iIdx])
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

