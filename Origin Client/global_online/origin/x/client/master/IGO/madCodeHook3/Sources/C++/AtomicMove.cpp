// ***************************************************************
//  AtomicMove.cpp            version: 1.0.0  ·  date: 2010-01-10
//  -------------------------------------------------------------
//  move up to 8 bytes atomically (multi cpu safe)
//  -------------------------------------------------------------
//  Copyright (C) 1999 - 2010 www.madshi.net, All Rights Reserved
// ***************************************************************

#define _ATOMICMOVE_C

#include "SystemIncludes.h"
#include "Systems.h"
#include "SystemsInternal.h"
#include <intrin.h>

SYSTEMS_API BOOL WINAPI AtomicMove(volatile LPVOID source, volatile LPVOID destination, int length)
{
  BOOL result = FALSE;

  if ((length > 0) && (length <= 8))
  {
    long long srcMask = 0;
    long long dstMask = 0;

    switch (length)
    {
        // Little endian version 
    case 1: srcMask = 0x00000000000000FF; dstMask = 0xFFFFFFFFFFFFFF00; break;
    case 2: srcMask = 0x000000000000FFFF; dstMask = 0xFFFFFFFFFFFF0000; break;
    case 3: srcMask = 0x0000000000FFFFFF; dstMask = 0xFFFFFFFFFF000000; break;
    case 4: srcMask = 0x00000000FFFFFFFF; dstMask = 0xFFFFFFFF00000000; break;
    case 5: srcMask = 0x000000FFFFFFFFFF; dstMask = 0xFFFFFF0000000000; break;
    case 6: srcMask = 0x0000FFFFFFFFFFFF; dstMask = 0xFFFF000000000000; break;
    case 7: srcMask = 0x00FFFFFFFFFFFFFF; dstMask = 0xFF00000000000000; break;
    case 8: srcMask = 0xFFFFFFFFFFFFFFFF; dstMask = 0x0000000000000000; break;
    }

    // intrinsic _InterlockedCompareExchange64 is available on x86 systems running on any Pentium architecture or later
    // don't use InterlockedCompareExchange64, it's only available on x86 Vista or later!
    __declspec(align(8)) long long expectedValue = *((long long *)destination);
    _InterlockedCompareExchange64((long long *)destination, (*(long long *) source) & srcMask | (*(long long *) destination) & dstMask, expectedValue);

    result = TRUE;
  }

  return result;
}


SYSTEMS_API BOOL WINAPI AtomicCopy8b(volatile LPVOID source, volatile LPVOID destination, volatile LPVOID expectedDestination)
{
    BOOL result = FALSE;
    // intrinsic _InterlockedCompareExchange64 is available on x86 systems running on any Pentium architecture or later
    // don't use InterlockedCompareExchange64, it's only available on x86 Vista or later!
    __declspec(align(8)) long long expectedValue = *((long long *)expectedDestination);
    long long returnedValue = _InterlockedCompareExchange64((long long *)destination, *((long long *)source), expectedValue);
    if (returnedValue == expectedValue)  // we succeeded, no other thread changed our destination
        result = TRUE;

    return result;
}

SYSTEMS_API BOOL WINAPI AtomicCopy(volatile LPVOID source, volatile LPVOID destination, volatile LPVOID expectedDestination, int length)
{
  BOOL result = TRUE;
    __declspec(align(8)) char *sourcePtr = (char *)source;
    __declspec(align(8)) char *destinationPtr = (char *)destination;
    __declspec(align(8)) char *expectedDestinationPtr = (char *)expectedDestination;
    if (length<=8)
    {
        result&=AtomicCopy8b(sourcePtr, destinationPtr, expectedDestinationPtr);
    }
    else
    {
        // don't use AtomicCopy8b, since we cannot safely do a compare & exchange meaning:
        // for AtomicCopy8b - we can be sure that we only change 8 bytes or less, so we do a compare & exchange which protects us from changing a memeory location
        // that has been changed before/while we do a actual memory write

        // BUT:
        // in this branch, we change 8 or more bytes, so we forcefully write the memory, hoping that everything will be fine,
        // since we are at a point of no return, because one memory write may succeed, the next may fail (when using compare & exchange) and so on

        //We always copy 8 bytes at a time here, so even though length is not a multiple of 8, _InterlockedExchange64 copy 8 bytes of data. 
        while (length > 0)   // copy 8 byte chunks
        {
#ifdef _M_X64
			_InterlockedExchange64((long long *)destinationPtr, *((long long *)sourcePtr));
#else
			// because WinXP 32 bit does not support _InterlockedExchange64 - we have to use _InterlockedCompareExchange64
			__declspec(align(8)) long long expectedValue = *((long long *)destinationPtr);
			/*long long returnedValue = */_InterlockedCompareExchange64((long long *)destinationPtr, *((long long *)sourcePtr), expectedValue);
			// ignore this for now, since we have no way to return to the original memory state !!!!
			//if (returnedValue != expectedValue)  // we succeeded, no other thread changed our destination
			//	result = FALSE;
#endif
            length-=8;
            sourcePtr+=8;
            destinationPtr+=8;
        }

    }

  return result;
}
