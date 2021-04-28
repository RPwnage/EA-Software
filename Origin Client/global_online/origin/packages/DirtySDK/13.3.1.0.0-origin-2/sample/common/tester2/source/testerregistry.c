/*H********************************************************************************/
/*!
    \File testerregistry.h

    \Description
        Maintains a global registry for the tester2 application.

    \Notes
        This module works a little differently than other DirtySock modules.
        Because of the nature of a registry (a global collector of shared
        information) there is no pointer to pass around - Create simply
        allocates memory of a given size and Destroy frees this memory.
        Registry entries are stored in tagfield format.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 04/08/2005 (jfrank) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/util/tagfield.h"
#include "zmem.h"
#include "zlib.h"
#include "testerregistry.h"

/*** Defines **********************************************************************/
#define TESTERREGISTRY_REFCOUNT "_REFCOUNT"

/*** Type Definitions *************************************************************/

// registry module structure - private and not intended for external use
typedef struct TesterRegistryT
{
    int32_t iRegSize;               //!< registry size, in bytes
    char *pReg;                 //!< the actual registry
} TesterRegistryT;

/*** Variables ********************************************************************/

static TesterRegistryT *_TesterRegistry = NULL;

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function TesterRegistryCreate

    \Description
        Create a new registry of a given size.

    \Input iSize - Size of registry, in bytes, to create.  (<0) for default size
    
    \Output None

    \Notes
        Calling this function creates a global (static, private) registry which
        any module can access.  If TesterRegistryCreate is called multiple times,
        the old registry is blown away (if it existed) the memory is freed, and
        a new registry is created.  No memory will be leaked if TesterRegistryCreate
        is called multiple times.

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterRegistryCreate(int32_t iSize)
{
    // dump the old registry if it exists  
    if(_TesterRegistry != NULL)
        TesterRegistryDestroy();

    // now create a new registry
    _TesterRegistry = ZMemAlloc(sizeof(TesterRegistryT));
    ZMemSet(_TesterRegistry, 0, sizeof(TesterRegistryT));
    
    if(iSize < 0)
    {
        // use default size
        _TesterRegistry->iRegSize = TESTERREGISTRY_SIZE_DEFAULT;
    }
    else
    {
        // use passed in size
        _TesterRegistry->iRegSize = iSize;
    }
    _TesterRegistry->pReg = ZMemAlloc(_TesterRegistry->iRegSize);
    ZMemSet(_TesterRegistry->pReg, 0, _TesterRegistry->iRegSize);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistrySetPointer

    \Description
        Set a registry entry for a pointer

    \Input  *pEntryName - the registry entry name
    \Input  *pPtr       - the pointer to save
    
    \Output  int32_t        - 0=success, error code otherwise

    \Notes
        Reference count is automatically set to 1. To increment reference count
        use TesterRegistryGetPointer. To decrement reference count use
        TesterRegistryDecrementRefCount.

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistrySetPointer(const char *pEntryName, const void *pPtr)
{
    char strRefCountField[128];

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if(pEntryName == NULL)
    {
        ZPrintf("WARNING: TesterRegistrySet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    strcpy(strRefCountField, pEntryName);
    strcat(strRefCountField, TESTERREGISTRY_REFCOUNT);

    TagFieldSetNumber64(_TesterRegistry->pReg, _TesterRegistry->iRegSize, pEntryName, (uintptr_t)pPtr);
    TagFieldSetNumber(_TesterRegistry->pReg, _TesterRegistry->iRegSize, strRefCountField, 1);
    return(TESTERREGISTRY_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function TesterRegistrySetNumber

    \Description
        Set a registry entry for a number

    \Input  *pEntryName - the registry entry name
    \Input   iNum       - the number to save
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistrySetNumber(const char *pEntryName, const int32_t iNum)
{
    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if(pEntryName == NULL)
    {
        ZPrintf("WARNING: TesterRegistrySet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    TagFieldSetNumber(_TesterRegistry->pReg, _TesterRegistry->iRegSize, pEntryName, iNum);
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistrySetString

    \Description
        Set a registry entry for a pointer

    \Input  *pEntryName - the registry entry name
    \Input  *pStr       - the string to save
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistrySetString(const char *pEntryName, const char *pStr)
{
    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if((pEntryName == NULL) || (pStr == NULL))
    {
        ZPrintf("WARNING: TesterRegistrySet got NULL pointer for entry name or entry value\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    TagFieldSetString(_TesterRegistry->pReg, _TesterRegistry->iRegSize, pEntryName, pStr);
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistryGetPointer

    \Description
        Get a pointer entry from the registry

    \Input  *pEntryName - the registry entry name
    
    \Output
        void *          - requested pointer, or NULL if no pointer is set

    \Notes
        Reference count is incremented when Get is called. To decrement
        reference count use TesterRegsitryDecrementRefCount.

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
void *TesterRegistryGetPointer(const char *pEntryName)
{
    char *pField;
    char strRefCountField[128];
    int32_t iRefCount;

    // check for errors
    if (_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(NULL);
    }

    pField = (char *)TagFieldFind(_TesterRegistry->pReg, pEntryName);
    if (pField == NULL)
    {
        // no entry found
        return(NULL);
    }

    // get the current reference count
    strcpy(strRefCountField, pEntryName);
    strcat(strRefCountField, TESTERREGISTRY_REFCOUNT);
    iRefCount = TagFieldGetNumber(TagFieldFind(_TesterRegistry->pReg, strRefCountField), 1);

    // increment and set the reference count
    TagFieldSetNumber(_TesterRegistry->pReg, _TesterRegistry->iRegSize, strRefCountField, ++iRefCount);

    return((void *)(uintptr_t)TagFieldGetNumber64(pField, 0));
}

/*F********************************************************************************/
/*!
    \Function TesterRegistryDecrementRefCount

    \Description
        Decrement the reference count of a pointer entry in the registry

    \Input  *pEntryName - the registry entry name
    
    \Output
        int32_t - the reference count after the decrement; if 0 then pointer is
                  removed from the registry.

    \Notes
        The caller should check the return value. If the return value is 0 the
        pointer is removed from TesterRegistry and the caller should destroy 
        the object.

    \Version 06/09/2006 (tdills)
*/
/********************************************************************************F*/
int32_t TesterRegistryDecrementRefCount(const char *pEntryName)
{
    char strRefCountField[128];
    int32_t iRefCount;

    // check for errors
    if (_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }

    // get the current reference count
    strcpy(strRefCountField, pEntryName);
    strcat(strRefCountField, TESTERREGISTRY_REFCOUNT);
    iRefCount = TagFieldGetNumber(TagFieldFind(_TesterRegistry->pReg, strRefCountField), 0);

    // decrement the reference count
    --iRefCount;

    if (iRefCount <= 0)
    {
        // remove the pointer and reference count from the registry
        TagFieldDelete(_TesterRegistry->pReg, pEntryName);
        TagFieldDelete(_TesterRegistry->pReg, strRefCountField);
    }
    else
    {
        // set the decremenet reference count in the registry
        TagFieldSetNumber(_TesterRegistry->pReg, _TesterRegistry->iRegSize, strRefCountField, iRefCount);
    }
    
    return(iRefCount);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistryGetNumber

    \Description
        Get a numeric entry from the registry

    \Input  *pEntryName - the registry entry name
    \Input  *pNum       - destination integer
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistryGetNumber(const char *pEntryName, int32_t *pNum)
{
    char *pField;

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if((pEntryName == NULL) || (pNum == NULL))
    {
        ZPrintf("WARNING: TesterRegistryGet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    pField = (char *)TagFieldFind(_TesterRegistry->pReg, pEntryName);
    if(pField == NULL)
    {
        // no entry found
        *pNum = 0;
        return(TESTERREGISTRY_ERROR_NOSUCHENTRY);
    }

    *pNum = TagFieldGetNumber(pField, 0);
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistryGetString

    \Description
        Get a string entry from the registry

    \Input  *pEntryName - the registry entry name
    \Input  *pBuf       - destination string
    \Input   iBufSize   - size of the destination string
    
    \Output  int32_t        - 0=success, error code otherwise

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t TesterRegistryGetString(const char *pEntryName, char *pBuf, int32_t iBufSize)
{
    char *pField;

    // check for errors
    if(_TesterRegistry == NULL)
    {
        ZPrintf("WARNING: TesterRegistry used before Create.\n");
        return(TESTERREGISTRY_ERROR_NOTINITIALIZED);
    }
    if((pEntryName == NULL) || (pBuf == NULL) || (iBufSize == 0))
    {
        ZPrintf("WARNING: TesterRegistryGet got NULL pointer for entry name\n");
        return(TESTERREGISTRY_ERROR_BADDATA);
    }

    ZMemSet(pBuf, 0, iBufSize);
    pField = (char *)TagFieldFind(_TesterRegistry->pReg, pEntryName);
    if(pField == NULL)
    {
        // no entry found
        return(TESTERREGISTRY_ERROR_NOSUCHENTRY);
    }

    TagFieldGetString(pField, pBuf, iBufSize-1, "");
    return(TESTERREGISTRY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function TesterRegistryPrint

    \Description
        Print all the members of the registry to the console

    \Input  None
    
    \Output None

    \Version 04/11/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterRegistryPrint(void)
{
    char strRegEntry[256];
    char *pRegPtr;
    int32_t iEntryIndex;

    if(_TesterRegistry->pReg[0] == 0)
    {
        ZPrintf("REGISTRY: SIZE=%d {<empty>}\n",_TesterRegistry->iRegSize);
    }
    else
    {
        ZPrintf("REGISTRY: SIZE=%d :\n",_TesterRegistry->iRegSize);
        pRegPtr = _TesterRegistry->pReg;
        iEntryIndex = 0;
        while((*pRegPtr != 0) && (pRegPtr <= (_TesterRegistry->pReg + _TesterRegistry->iRegSize)))
        {
            strRegEntry[iEntryIndex] = *pRegPtr;
            iEntryIndex++;
            pRegPtr++;
            // cap a registry entry (for printing) at 254 bytes
            if (iEntryIndex >= 254)
            {
                while(  (*pRegPtr != '\r') && 
                        (*pRegPtr != '\n') && 
                        (*pRegPtr != '\t') && 
                        (*pRegPtr != ' ') && 
                        (*pRegPtr != 0))
                {
                    pRegPtr++;
                }
            }
            // print the entry
            if ((*pRegPtr == '\r') || 
                (*pRegPtr == '\n') || 
                (*pRegPtr == '\t') || 
                (*pRegPtr == ' ') ||
                (*pRegPtr == 0) )
            {
                strRegEntry[iEntryIndex]=0;
                ZPrintf("REGISTRY: -{%s}\n",strRegEntry);
                iEntryIndex=0;
                // find the beginning of the next entry
                while( ((*pRegPtr == '\r') || (*pRegPtr == '\n') || (*pRegPtr == '\t') || (*pRegPtr == ' ')) && 
                        (*pRegPtr != 0) )
                {
                    pRegPtr++;
                }
            }
        }


    }
}

/*F********************************************************************************/
/*!
    \Function TesterRegistryDestroy

    \Description
        Destroy the registry and free all associated memory.

    \Input  None
    
    \Output None

    \Version 04/08/2005 (jfrank)
*/
/********************************************************************************F*/
void TesterRegistryDestroy(void)
{
    _TesterRegistry->iRegSize = 0;
    ZMemFree(_TesterRegistry->pReg);
    ZMemFree(_TesterRegistry);
    _TesterRegistry = NULL;
}

