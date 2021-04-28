/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/system/fiberlocal.h"


namespace Blaze
{

EA_THREAD_LOCAL uint8_t* FiberLocalVarBase::msFiberLocalStorageBase = nullptr;

FiberLocalVarBase::FiberLocalVarBase(size_t varSize)
{
    mOffset = getGlobalVarSize();        
    getGlobalVarSize() += varSize;
    getGlobalFiberLocalList().push_back(*this);
}

FiberLocalVarBase::~FiberLocalVarBase()
{
    // Remove the list node:
    getGlobalFiberLocalList().remove(*this);
}


uint8_t* FiberLocalVarBase::initFiberLocals(MemoryGroupId memId)
{
    uint8_t* result = BLAZE_NEW_ARRAY_MGID(uint8_t, getGlobalVarSize(), memId, "Fiber Local Storage");

    FiberLocalList& list = getGlobalFiberLocalList();
    for (FiberLocalList::iterator itr = list.begin(), end = list.end(); itr != end; ++itr)
    {
        itr->init(result + itr->mOffset);
    }

    return result;
}

void FiberLocalVarBase::resetFiberLocals()
{
    FiberLocalList& list = getGlobalFiberLocalList();
    for (FiberLocalList::iterator itr = list.begin(), end = list.end(); itr != end; ++itr)
    {
        itr->reset();
    }
}

void FiberLocalVarBase::destroyFiberLocals(uint8_t* mem)
{
    FiberLocalList& list = getGlobalFiberLocalList();
    for (FiberLocalList::iterator itr = list.begin(), end = list.end(); itr != end; ++itr)
    {
        itr->destroy(mem + itr->mOffset);
    }

    delete [] mem;
}

FiberLocalVarBase::FiberLocalList& FiberLocalVarBase::getGlobalFiberLocalList() 
{
    static FiberLocalList list;
    return list;
}

size_t& FiberLocalVarBase::getGlobalVarSize() 
{ 
    //Doing this here ensures initialization ordering doesn't cause a problem
    static size_t sVarSize = 0; 
    return sVarSize; 
}

} // Blaze

