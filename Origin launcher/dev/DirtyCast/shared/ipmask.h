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

    \Version 02/04/2005 (doneill)   First Version
    \Version 07/30/2009 (jbrookes)  C conversion for DirtyCast
*/
/********************************************************************************H*/

#ifndef _ipmask_h
#define _ipmask_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define IPMASK_MAX_ADDRS    32

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! ipmask datatype
typedef struct IPMaskT
{
    int32_t iCount;                     //!< number of configured addr/mask pairs
    uint32_t aAddrs[IPMASK_MAX_ADDRS];  //!< addresses to match
    uint32_t aMasks[IPMASK_MAX_ADDRS];  //!< masks to apply
    uint8_t _pad[4];
} IPMaskT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// (re)initialize this instance from the given config data
void IPMaskInit(IPMaskT *pIPMask, const char *pTagPrefix, const char *pAddrTag, const char *pMaskTag, const char *pCommandTags, const char *pConfigTags, uint8_t bAddLocal);

// check if the given address is a match
uint8_t IPMaskMatch(IPMaskT *pIPMask, uint32_t uAddr);

#ifdef __cplusplus
};
#endif

#endif // _ipmask_h

