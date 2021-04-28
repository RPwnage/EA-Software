//Copyright (c) 2007-2008, Marton Anka
//
//Permission is hereby granted, free of charge, to any person obtaining a 
//copy of this software and associated documentation files (the "Software"), 
//to deal in the Software without restriction, including without limitation 
//the rights to use, copy, modify, merge, publish, distribute, sublicense, 
//and/or sell copies of the Software, and to permit persons to whom the 
//Software is furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included 
//in all copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
//OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
//THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
//FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
//IN THE SOFTWARE.


#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include "mhook.h"
#include "../Disasm/disasm.h"
#include <crtdbg.h>
#pragma warning(push)
#pragma warning(disable:4548) // expression before comma has no effect; expected expression with side-effect
#include "EASTL/vector.h"
#include "EASTL/hash_map.h"
#include "EASTL/functional.h"
#pragma warning(pop)
#include "../../IGO/madCodeHook3/Sources/C++/SystemIncludes.h"
#include "../../IGO/madCodeHook3/Sources/C++/Systems.h"
#include "../../IGO/madCodeHook3/Sources/C++/ProcessTools.h"
#include "../../IGO/madCodeHook3/Sources/C++/Stubs.h"
#include "../../IGO/madCodeHook3/Sources/C++/StringConstants.h"
#include "../../IGO/madCodeHook3/Sources/C++/Hooking.h"

#include "eathread/eathread_futex.h"
#include "eathread/eathread_rwspinlock.h"

#include "../IGOSharedStructs.h"
#include "../IGOTelemetry.h"

#ifdef _DEBUG
#define _DEBUG_HOOKING_
#endif

#ifdef _USE_IGO_LOGGER_		// we only use the logger in the igo32.dll, not in the Origin.exe !!!
#include "../IGOLogger.h"
#endif

//=========================================================================
const unsigned int MHOOKS_MAX_CODE_BYTES = 32;
const unsigned int MHOOKS_MAX_RIPS = 4;
//=========================================================================
// The trampoline structure - stores every bit of info about a hook
__declspec(align(8)) struct MHOOKS_TRAMPOLINE {
    __declspec(align(8)) BYTE   pbTempCode[MHOOKS_MAX_CODE_BYTES];              // the first jump that we wrote

    __declspec(align(8)) BYTE	codeJumpToHookFunction[MHOOKS_MAX_CODE_BYTES];	// placeholder for code that jumps to the hook function
    __declspec(align(8)) BYTE	codeTrampoline[MHOOKS_MAX_CODE_BYTES];			// placeholder for code that holds the first few

    __declspec(align(8)) BYTE	codeUntouched[MHOOKS_MAX_CODE_BYTES];			// placeholder for unmodified original code
    __declspec(align(8)) BYTE	codeHookedSystemFunction[MHOOKS_MAX_CODE_BYTES];			// placeholder for our hoooked system function code

    __declspec(align(8)) PBYTE  pUnalteredSystemFunction;// the original system function pointer
    __declspec(align(8)) PBYTE	pSystemFunction;								// the original system function pointer we wrote our hook to
    __declspec(align(8)) PBYTE	pHookFunction;									// the hook function that we provide

    __declspec(align(8)) HMODULE hModule;                                       // original code module
    __declspec(align(8)) PBYTE   preallocationModule;                           // module used as a reference when pre-allocating this trampoline
    __declspec(align(8)) DWORD	 cbOverwrittenCode;								// number of bytes overwritten by the jump
    __declspec(align(8)) BYTE	 codeHookedSystemFunctionSize;			        // max. MHOOKS_MAX_CODE_BYTES

    __declspec(align(8)) MHOOKS_TRAMPOLINE* pPrevTrampoline;					// When in the free list, thess are pointers to the prev and next entry.
	__declspec(align(8)) MHOOKS_TRAMPOLINE* pNextTrampoline;					// When not in the free list, this is a pointer to the prev and next trampoline in use.
};

//=========================================================================
// The patch data structures - store info about rip-relative instructions
// during hook placement
struct MHOOKS_RIPINFO
{
    DWORD	dwOffset;
    S64		nDisplacement;
};

struct MHOOKS_PATCHDATA
{
    S64				nLimitUp;
    S64				nLimitDown;
    DWORD			nRipCnt;
    DWORD           nBranchIndex;
    MHOOKS_RIPINFO	rips[MHOOKS_MAX_RIPS];
};

//=========================================================================
// Global vars
static EA::Thread::RWSpinLock g_HookingLock;
static eastl::hash_map<PBYTE /*pHookedFunction*/, MHOOKS_TRAMPOLINE*> g_pHooks;
static MHOOKS_TRAMPOLINE* g_pFreeList = NULL;
typedef eastl::hash_map<size_t, HMODULE> CheckedDllMap;
static CheckedDllMap g_pCheckedDlls;
static DWORD g_nHooksInUse = 0;
static eastl::vector<HANDLE> g_hThreadHandles;
static DWORD g_nThreadHandles = 0;
#define MHOOK_JMPSIZE 5
#define MHOOK_MINALLOCSIZE 4096
static char g_lastErrorDetails[256] = {0};

//=========================================================================
// Internal function:
//
// Remove the trampoline from the specified list, updating the head pointer
// if necessary.
//=========================================================================
static VOID ListRemove(MHOOKS_TRAMPOLINE** pListHead, MHOOKS_TRAMPOLINE* pNode) {
	if (pNode->pPrevTrampoline) {
		pNode->pPrevTrampoline->pNextTrampoline = pNode->pNextTrampoline;
	}

	if (pNode->pNextTrampoline) {
		pNode->pNextTrampoline->pPrevTrampoline = pNode->pPrevTrampoline;
	}

	if ((*pListHead) == pNode) {
		(*pListHead) = pNode->pNextTrampoline;
		assert((*pListHead)->pPrevTrampoline == NULL);
	}

	pNode->pPrevTrampoline = NULL;
	pNode->pNextTrampoline = NULL;
}

//=========================================================================
// Internal function:
//
// Prepend the trampoline from the specified list and update the head pointer.
//=========================================================================
static VOID ListPrepend(MHOOKS_TRAMPOLINE** pListHead, MHOOKS_TRAMPOLINE* pNode) {
	pNode->pPrevTrampoline = NULL;
	pNode->pNextTrampoline = (*pListHead);
	if ((*pListHead)) {
		(*pListHead)->pPrevTrampoline = pNode;
	}
	(*pListHead) = pNode;
}

//=========================================================================
// Internal function:
//
// Round down to the next multiple of rndDown
//=========================================================================
static size_t RoundDown(size_t addr, size_t rndDown)
{
	return (addr / rndDown) * rndDown;
}

//=========================================================================
// Internal function:
// 
// Will attempt allocate a block of memory within the specified range, as 
// near as possible to the specified function.
//=========================================================================
static MHOOKS_TRAMPOLINE* BlockAlloc(PBYTE pSystemFunction, PBYTE pbLower, PBYTE pbUpper, bool moduleSpecificPreallocation) {
	SYSTEM_INFO sSysInfo =  {0};
	::GetSystemInfo(&sSysInfo);

	// Always allocate in bulk, in case the system actually has a smaller allocation granularity than MINALLOCSIZE.
    const ptrdiff_t cAllocSize = (sSysInfo.dwAllocationGranularity > MHOOK_MINALLOCSIZE) ? sSysInfo.dwAllocationGranularity : MHOOK_MINALLOCSIZE;

	MHOOKS_TRAMPOLINE* pRetVal = NULL;
	PBYTE pModuleGuess = (PBYTE) RoundDown((size_t)pSystemFunction, cAllocSize);
	int loopCount = 0;
	for (PBYTE pbAlloc = pModuleGuess; pbLower < pbAlloc && pbAlloc < pbUpper; ++loopCount) {
		// determine current state
		MEMORY_BASIC_INFORMATION mbi;
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
		//OriginIGO::IGOLogDebug("mhooks: BlockAlloc: Looking at address %p", pbAlloc);
#endif
		if (!VirtualQuery(pbAlloc, &mbi, sizeof(mbi)))
			break;
		// free & large enough?
		if (mbi.State == MEM_FREE && mbi.RegionSize >= (unsigned)cAllocSize) {
			// and then try to allocate it
			pRetVal = (MHOOKS_TRAMPOLINE*) VirtualAlloc(pbAlloc, cAllocSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);
			if (pRetVal) {
				size_t trampolineCount = cAllocSize / sizeof(MHOOKS_TRAMPOLINE);
				OriginIGO::IGOLogInfo("mhooks: BlockAlloc: Allocated block at %p as %d trampolines", pRetVal, trampolineCount);

				pRetVal[0].pPrevTrampoline = NULL;
				pRetVal[0].pNextTrampoline = &pRetVal[1];
                pRetVal[0].preallocationModule = moduleSpecificPreallocation ? pSystemFunction : NULL;

				// prepare them by having them point down the line at the next entry.
				for (size_t s = 1; s < trampolineCount; ++s) {
					pRetVal[s].pPrevTrampoline = &pRetVal[s - 1];
					pRetVal[s].pNextTrampoline = &pRetVal[s + 1];
                    pRetVal[s].preallocationModule = moduleSpecificPreallocation ? pSystemFunction : NULL;
				}

				// last entry points to the current head of the free list
				pRetVal[trampolineCount - 1].pNextTrampoline = g_pFreeList;
                if (g_pFreeList)
                    g_pFreeList->pPrevTrampoline = &pRetVal[trampolineCount - 1];

				break;
			}
		}
				
		// This is a spiral, should be -1, 1, -2, 2, -3, 3, etc. (* cAllocSize)
		ptrdiff_t bytesToOffset = (cAllocSize * (loopCount + 1) * ((loopCount % 2 == 0) ? -1 : 1));
		pbAlloc = pbAlloc + bytesToOffset;
	}
	
    if (!pRetVal)
        OriginIGO::IGOLogWarn("mhooks: BlockAlloc: Unable to allocate a block for trampolines");

	return pRetVal;
}

//=========================================================================
// Internal function:
//
// Will try to allocate a big block of memory inside the required range. 
//=========================================================================
static MHOOKS_TRAMPOLINE* FindTrampolineInRange(HMODULE hModule, PBYTE pLower, PBYTE pUpper) {
	if (!g_pFreeList) {
		return NULL;
	}

    // This is a standard free list, except we're doubly linked to deal with some return shenanigans.
    MHOOKS_TRAMPOLINE* curEntry = g_pFreeList;

    // First look for entries specifically allocated for a module - not really necessary, just being fancy!
    if (hModule)
    {
	    while (curEntry) {
            if (curEntry->preallocationModule == (PBYTE)hModule 
               && (MHOOKS_TRAMPOLINE*) pLower < curEntry && curEntry < (MHOOKS_TRAMPOLINE*) pUpper) {

			    ListRemove(&g_pFreeList, curEntry);
			    return curEntry;
		    }

		    curEntry = curEntry->pNextTrampoline;
	    }
    }

    // K, any entry will do!
	curEntry = g_pFreeList;
	while (curEntry) {
		if ((MHOOKS_TRAMPOLINE*) pLower < curEntry && curEntry < (MHOOKS_TRAMPOLINE*) pUpper) {
			ListRemove(&g_pFreeList, curEntry);

			return curEntry;
		}

		curEntry = curEntry->pNextTrampoline;
	}

	return NULL;
}

//=========================================================================
// Internal function:
// 
// Skip over jumps that lead to the real function. Gets around import
// jump tables, etc.
//=========================================================================
#if 0 // UNUSED
static bool isImportedFunction(PBYTE pbCode, PBYTE pbAddress)
{
    MEMORY_BASIC_INFORMATION mbi;
    VirtualQuery((PVOID)pbCode, &mbi, sizeof(mbi));
    __try {
        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)mbi.AllocationBase;
        if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            return false;
        }

        PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)((PBYTE)pDosHeader +
            pDosHeader->e_lfanew);
        if (pNtHeader->Signature != IMAGE_NT_SIGNATURE) {
            return false;
        }

        if (pbAddress >= ((PBYTE)pDosHeader +
            pNtHeader->OptionalHeader
            .DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress) &&
            pbAddress < ((PBYTE)pDosHeader +
            pNtHeader->OptionalHeader
            .DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress +
            pNtHeader->OptionalHeader
            .DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size)) {
            return true;
        }
        return false;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
#endif

//=========================================================================
// Internal function:
//
// Writes code in a temporary buffer pbTempCode that jumps from pbCode to pbJumpTo.
// Will attempt to do this
// in as few bytes as possible. Important on x64 where the long jump
// (0xff 0x25 ....) can take up 14 bytes.
//=========================================================================
static PBYTE EmitJump(PBYTE pbTempCode, PBYTE pbCode, PBYTE pbJumpTo, bool addRedundantCall = false, DWORD availableSpacesInBytes = 0) {
#ifdef _M_IX86_X64
    PBYTE pbJumpFrom = pbCode + 5;
    SIZE_T cbDiff = pbJumpFrom > pbJumpTo ? pbJumpFrom - pbJumpTo : pbJumpTo - pbJumpFrom;
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
    OriginIGO::IGOLogDebug("EmitJump: Jumping from %p to %p, diff is %p", pbJumpFrom, pbJumpTo, cbDiff);
#endif
    static BYTE knownPrefix[] = 
    {
        0x8B, 0xFF,     // mov edi, edi
        0x55,           // push ebp
        0x8B, 0xEC,     // mov ebp, esp
    };
    
    BYTE originalOpcodes[_countof(knownPrefix)];
    for (int idx = 0; idx < _countof(knownPrefix); ++idx)
        originalOpcodes[idx] = pbTempCode[idx];

    if (cbDiff <= 0x7fff0000) {
        pbTempCode[0] = 0xe9;
        pbTempCode += 1;
        *((PDWORD)pbTempCode) = (DWORD)(DWORD_PTR)(pbJumpTo - pbJumpFrom);
        pbTempCode += sizeof(DWORD);

        // For 32-bit games, Fraps doesn't restore our hooks properly BUT it also uses an 0xe9 jump call -> if we add two in a row, then it can't override us
        // Of course, we need to know the overwritten opcodes are "safe"/need to restore whatever they do, so we limit ourselves right now to what we need for the call to Present
        if (addRedundantCall && availableSpacesInBytes >= 13)
        {
            int idx = 0;
            for (; idx < _countof(knownPrefix); ++idx)
            {
                if (originalOpcodes[idx] != knownPrefix[idx])
                    break;
            }

            if (idx == _countof(knownPrefix))
            {
                pbJumpFrom = pbCode + 13;
                cbDiff = pbJumpFrom > pbJumpTo ? pbJumpFrom - pbJumpTo : pbJumpTo - pbJumpFrom;
                if (cbDiff <= 0x7fff0000) {
                    pbTempCode[0] = 0x8B;   // mov esp, ebp
                    pbTempCode[1] = 0xE5;
                    pbTempCode[2] = 0x5D;   // pop ebp
                    pbTempCode[3] = 0xe9;
                    pbTempCode += 4;
                    *((PDWORD)pbTempCode) = (DWORD)(DWORD_PTR)(pbJumpTo - pbJumpFrom);
                    pbTempCode += sizeof(DWORD);
                }
            }
        }
    }
    else {
        pbTempCode[0] = 0xff;
        pbTempCode[1] = 0x25;
        pbTempCode += 2;
#ifdef _M_IX86
        // on x86 we write an absolute address (just behind the instruction)
        *((PDWORD)pbTempCode) = (DWORD)(DWORD_PTR)((pbCode + 2) + sizeof(DWORD));
#elif defined _M_X64
        // on x64 we write the relative address of the same location
        *((PDWORD)pbTempCode) = (DWORD)0;
#endif
        pbTempCode += sizeof(DWORD);
        *((PDWORD_PTR)pbTempCode) = (DWORD_PTR)(pbJumpTo);
        pbTempCode += sizeof(DWORD_PTR);
    }
#else 
#error unsupported platform
#endif
    return pbTempCode;
}

//=========================================================================
// Internal function:
//
// Writes code in a temporary buffer pbTempCode that calls from pbCode to pbJumpTo.
//=========================================================================
static bool EmitRelativeCallOp(PBYTE pbTempCode, PBYTE pbCode, PBYTE pbCallTo) 
{
#ifdef _M_IX86_X64
    PBYTE pbCallFrom = pbCode + 5;
    SIZE_T cbDiff = pbCallFrom > pbCallTo ? pbCallFrom - pbCallTo : pbCallTo - pbCallFrom;
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
    OriginIGO::IGOLogDebug("EmitCall: Calling from %p to %p, diff is %p", pbCallFrom, pbCallTo, cbDiff);
#endif
    if (cbDiff <= 0x7fff0000) 
    {
        pbTempCode[0] = 0xe8;
        pbTempCode += 1;
        *((PDWORD)pbTempCode) = (DWORD)(DWORD_PTR)(pbCallTo - pbCallFrom);

        return true;
    }
#else 
#error unsupported platform
#endif
    return false;
}

//=========================================================================
// Internal function:
//
// Will try to allocate the trampoline structure within 2 gigabytes of
// the target function. 
//=========================================================================
#pragma warning(push)
#pragma warning(disable:4310) // cast truncates constant value
static MHOOKS_TRAMPOLINE* TrampolineAlloc(HMODULE hModule, LPCSTR moduleName, LPCSTR apiName, PBYTE pSystemFunction, S64 nLimitUp, S64 nLimitDown) {

        // determine lower and upper bounds for the allocation locations.
        // in the basic scenario this is +/- 2GB but IP-relative instructions
        // found in the original code may require a smaller window.

        PBYTE pLower = pSystemFunction + nLimitUp;
        pLower = pLower < (PBYTE)(DWORD_PTR)0x0000000080000000 ?
            (PBYTE)(0x1) : (PBYTE)(pLower - (PBYTE)0x7fff0000);
        PBYTE pUpper = pSystemFunction + nLimitDown;
        pUpper = pUpper < (PBYTE)(DWORD_PTR)0xffffffff80000000 ?
            (PBYTE)(pUpper + (DWORD_PTR)0x7ff80000) : (PBYTE)(DWORD_PTR)0xfffffffffff80000;

    OriginIGO::IGOLogInfo("TrampolineAlloc: for %p (%s) between %p and %p", pSystemFunction, apiName ? apiName : "", pLower, pUpper);

    // try to find a trampoline in the specified range
	MHOOKS_TRAMPOLINE* pTrampoline = FindTrampolineInRange(hModule, pLower, pUpper);
	if (!pTrampoline) {
		// if it we can't find it, then we need to allocate a new block and 
		// try again. Just fail if that doesn't work 
		g_pFreeList = BlockAlloc(pSystemFunction, pLower, pUpper, false);
		pTrampoline = FindTrampolineInRange(NULL, pLower, pUpper);

                        }

        // found and allocated a trampoline?
        if (pTrampoline) {

            pTrampoline->hModule = hModule;
            g_pHooks[pTrampoline->codeTrampoline] = pTrampoline;
        }
        else
        {
#if defined(_USE_IGO_LOGGER_)
        OriginIGO::IGOLogWarn("Trampoline not allocated: %s", apiName);
#endif
    }

    return pTrampoline;
}
#pragma warning(pop)

//=========================================================================
// Internal function:
//
// Return the internal trampoline structure that belongs to a hooked function.
//=========================================================================
static MHOOKS_TRAMPOLINE* TrampolineGet(PBYTE pHookedFunction) {

    eastl::hash_map<PBYTE /*pHookedFunction*/, MHOOKS_TRAMPOLINE*>::const_iterator iter = g_pHooks.find(pHookedFunction);
    if (iter != g_pHooks.end())
        return iter->second;

    return NULL;
}

//=========================================================================
// Internal function:
//
// Free a trampoline structure.
//=========================================================================
static VOID TrampolineFree(MHOOKS_TRAMPOLINE* pTrampoline, bool doNotDealloc = false) {

    eastl::hash_map<PBYTE /*pHookedFunction*/, MHOOKS_TRAMPOLINE*>::iterator iter = g_pHooks.find(pTrampoline->codeTrampoline);
    if (iter != g_pHooks.end())
    {
        g_pHooks.erase(iter);

        if (doNotDealloc)
        {
            OriginIGO::IGOLogWarn("TrampolineFree: Do not dealloc %p", pTrampoline);
            return;
        }

        // It might be OK to call VirtualFree, but quite possibly it isn't: 
        // If a thread has some of our trampoline code on its stack
        // and we yank the region from underneath it then it will
        // surely crash upon returning. So instead of freeing the 
        // memory we just let it leak. Ugly, but safe.
        //if (bNeverUsed)
        // clear trampoline....
        DWORD dwOldProtectTrampolineFunction = 0;
        HANDLE hProc = GetCurrentProcess();

        if (VirtualProtectEx(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), PAGE_EXECUTE_READWRITE, &dwOldProtectTrampolineFunction))
        {
            PBYTE preallocationModule = pTrampoline->preallocationModule;
            ZeroMemory(pTrampoline, sizeof(MHOOKS_TRAMPOLINE));
            pTrampoline->preallocationModule = preallocationModule;

            if (!VirtualProtectEx(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), dwOldProtectTrampolineFunction, &dwOldProtectTrampolineFunction))
            {
                OriginIGO::IGOLogWarn("VirtualProtectEx failed on 1");
            }

        else
        {
                OriginIGO::IGOLogInfo("Trampoline %p ready for reuse", pTrampoline);
                ListPrepend(&g_pFreeList, pTrampoline);
            }
        }
        else
        {
            OriginIGO::IGOLogWarn("VirtualProtectEx failed on 2");
        }
    }

    else
        OriginIGO::IGOLogWarn("TrampolineFree: no such trampoline %p", pTrampoline);
}

//=========================================================================
// if IP-relative addressing has been detected, fix up the code so the
// offset points to the original location
static void FixupIPRelativeAddressing(PBYTE pbNew, PBYTE pbOriginal, MHOOKS_PATCHDATA* pdata)
{
#if defined _M_X64
    S64 diff = pbNew - pbOriginal;
    for (DWORD i = 0; i < pdata->nRipCnt; i++) {
        DWORD dwNewDisplacement = (DWORD)(pdata->rips[i].nDisplacement - diff);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
        OriginIGO::IGOLogDebug("fixing up RIP instruction operand for code at 0x%p: old displacement: 0x%8.8x, new displacement: 0x%8.8x",
            pbNew + pdata->rips[i].dwOffset,
            (DWORD)pdata->rips[i].nDisplacement,
            dwNewDisplacement);
#endif
        *(PDWORD)(pbNew + pdata->rips[i].dwOffset) = dwNewDisplacement;
    }
#endif
}



static DWORD FindFunctionEnd(PVOID pFunction, DWORD dwMaxLen) {
    DWORD dwRet = 0;
#ifdef _M_IX86
    ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined _M_X64
    ARCHITECTURE_TYPE arch = ARCH_X64;
#else
#error unsupported platform
#endif
    DISASSEMBLER dis;

    if (InitDisassembler(&dis, arch))
    {
        U8* pLoc = (U8*)pFunction;
        DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT;

#if !defined(_DEBUG_HOOKING_) && !defined(_DEBUG)
        dwFlags |= DISASM_SUPPRESSERRORS;
#endif

        while (dwRet < dwMaxLen)
        {
            INSTRUCTION* pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags);
            if (!pins)
                break;

            if (pins->Type == ITYPE_RET)    // return - indicates our end
            {
                dwRet += pins->Length;
                pLoc += pins->Length;

                break;
            }
            else
                if (pins->Type == ITYPE_BRANCH)    // unconditional jump -  indicates our end
                {
                    dwRet += pins->Length;
                    pLoc += pins->Length;

                    break;
                }
                else
                {
                    dwRet += pins->Length;
                    pLoc += pins->Length;
                }
        }


        if (dwRet > dwMaxLen)   // clip it at the max length!
            dwRet = dwMaxLen;

        CloseDisassembler(&dis);
    }

    return dwRet;
}

enum BranchType{
    NONE = 0,
    JMP16REL,
    JMP32REL,
    JMP8REL_RIP,
    JMP32REL_RIP,
    JMP32ABS,
};

#if 0 // UNUSED
static DWORD FindAlteredTarget(PVOID pFunction, BranchType& branchType) {
    DWORD dwRet = 0;
#ifdef _M_IX86
    ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined _M_X64
    ARCHITECTURE_TYPE arch = ARCH_X64;
#else
#error unsupported platform
#endif
    DISASSEMBLER dis;

    if (InitDisassembler(&dis, arch))
    {
        INSTRUCTION* pins = NULL;
        U8* pLoc = (U8*)pFunction;
        DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT;

        // get prev. instruction
        INSTRUCTION prevPins = { 0 };
        U8* prevpLoc = NULL;

#if !defined(_DEBUG_HOOKING_) && !defined(_DEBUG)
        dwFlags |= DISASM_SUPPRESSERRORS;
#endif

        while ((pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags)))
        {
            if (pins->Type == ITYPE_RET)    // return - indicates our end
            {
                dwRet = 0;
                break;
            }
            else
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
                OriginIGO::IGOLogDebug("BRANCH");
#endif

#if defined _M_IX86
            if (pins->OpcodeBytes[0] == 0xE9 && pins->Length == 5)
            {
                OriginIGO::IGOLogDebug("JMP16REL");
                branchType = JMP16REL;
                break;
            }
            else
                if (pins->OpcodeBytes[0] == 0xE9 && pins->Length == 6)
                {
                    OriginIGO::IGOLogDebug("JMP32REL");
                    branchType = JMP32REL;
                    break;
                }
                else
                    if (pins->OpcodeBytes[0] == 0xFF && (pins->Length == 5 || pins->Length == 6))
                    {
                        OriginIGO::IGOLogDebug("JMP32IN");
                        branchType = NONE;
                        break;
                    }
#else
#if defined _M_X64
            if (pins->OpcodeBytes[0] == 0xEB && pins->Length == 2)
            {
                OriginIGO::IGOLogDebug("JMP8REL_RIP");
                branchType = JMP8REL_RIP;
                break;
            }
            else
                if (pins->OpcodeBytes[0] == 0xE9 && pins->Length == 5)
                {
                    OriginIGO::IGOLogDebug("JMP32REL_RIP");
                    branchType = JMP32REL_RIP;
                    break;
                }
                else
                    // jmp         rax 
                    if (pins->Type == ITYPE_BRANCH && pins->Length == 2 && pins->OperandCount == 1 &&
                        (pins->Operands[0].Register >= AMD64_REG_RAX &&
                        pins->Operands[0].Register <= AMD64_REG_R15))
                    {
                        // 48 C7 C0 F0 A8 0F 73 mov         rax,730FA8F0h  
                        if (prevPins.Type == ITYPE_MOV && prevPins.Length == 7 &&
                            (prevPins.Operands[0].Register >= AMD64_REG_RAX &&
                            prevPins.Operands[0].Register <= AMD64_REG_R15)
                            && prevPins.Operands[0].Register == pins->Operands[0].Register)
                        {
                            dwRet -= (prevPins.Length - 3); // 7 - 3 -> 4 byte (32 bit) address left
                            OriginIGO::IGOLogDebug("JMP32ABS");
                            branchType = JMP32ABS;
                            break;
                        }
                    }

#endif
#endif

            memcpy(&prevPins, pins, sizeof(INSTRUCTION));
            prevpLoc = pLoc;

            dwRet += pins->Length;
            pLoc += pins->Length;
        }

        CloseDisassembler(&dis);
    }

    return dwRet;
}
#endif

//=========================================================================
// Examine the machine code at the target function's entry point, and
// skip bytes in a way that we'll always end on an instruction boundary.
// We also detect branches and subroutine calls (as well as returns)
// at which point disassembly must stop.
// Finally, detect and collect information on IP-relative instructions
// that we can patch.

static DWORD DisassembleAndSkip(PVOID pFunction, DWORD dwMinLen, MHOOKS_PATCHDATA* pdata, bool &earlyCallOpFlag, BranchType& branchType) {
    DWORD dwRet = 0;
    pdata->nLimitDown = 0;
    pdata->nLimitUp = 0;
    pdata->nRipCnt = 0;
    pdata->nBranchIndex = 0;
#ifdef _M_IX86
    ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined _M_X64
    ARCHITECTURE_TYPE arch = ARCH_X64;
#else
#error unsupported platform
#endif
    DISASSEMBLER dis;

#if !defined _M_X64
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
    if (earlyCallOpFlag)
        OriginIGO::IGOLogWarn("Hooking detected earlyCallOpFlag (64 bit)!");
#endif
#endif

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
    OriginIGO::IGOLogDebug("hook start");
#endif

    if (InitDisassembler(&dis, arch)) {
        U8* pLoc = (U8*)pFunction;
        DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT;

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
        OriginIGO::IGOLogDebug("DisassembleAndSkip: Disassembling %p", pLoc);
#endif
        bool opBarrier = false;

#if !defined(_DEBUG_HOOKING_) && !defined(_DEBUG)
        dwFlags |= DISASM_SUPPRESSERRORS;
#endif

        while (!opBarrier && dwRet < dwMinLen)
        {
             INSTRUCTION* pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags);
             if (!pins)
                 break;

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
            OriginIGO::IGOLogDebug("DisassembleAndSkip: %p: %s", pLoc, pins->String);
#endif

            if (pins->Type == ITYPE_RET)
            {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
                OriginIGO::IGOLogDebug("RET");
#endif
                break;
            }
            if (pins->Type == ITYPE_BRANCH)
            {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
                OriginIGO::IGOLogDebug("BRANCH");
#endif

#if defined _M_IX86
                if (pins->OpcodeBytes[0] == 0xE9)
                {
                    OriginIGO::IGOLogDebug("JMP32REL");
                    branchType = JMP32REL;
                    pdata->nBranchIndex = dwRet;
                    opBarrier = true;
                }
                else
                if (pins->OpcodeBytes[0] == 0xFF && (pins->Length == 5 || pins->Length == 6))
                {
                    OriginIGO::IGOLogDebug("JMP32IN");
                    branchType = NONE;
                    opBarrier = true;
                }
                else
                {
                    break;
                }
#else
#if defined _M_X64
                if (pins->OpcodeBytes[0] == 0xEB && pins->Length == 2)
                {
                    OriginIGO::IGOLogDebug("JMP8REL_RIP");
                    branchType = JMP8REL_RIP;
                    pdata->nBranchIndex = dwRet;
                }
                else
                if (pins->OpcodeBytes[0] == 0xE9 && pins->Length == 5)
                {
                    OriginIGO::IGOLogDebug("JMP32REL_RIP");
                    branchType = JMP32REL_RIP;
                    pdata->nBranchIndex = dwRet;
                    opBarrier = true;
                }
                else
                {
                    break;
                }
#endif
#endif
            }

            if (pins->Type == ITYPE_BRANCHCC)
            {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
                OriginIGO::IGOLogDebug("BRANCHCC");
#endif
                break;
            }
            if (pins->Type == ITYPE_CALL)
            {

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
                OriginIGO::IGOLogDebug("CALL 0x%02x", pins->OpcodeBytes[0]);
#endif
                if (pins->OpcodeBytes[0] == 0xE8)
                {                     
                    earlyCallOpFlag = true;
                    pdata->nBranchIndex = dwRet;
                }

                else
                    break;
            }
            if (pins->Type == ITYPE_CALLCC)
            {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
                OriginIGO::IGOLogDebug("CALLCC");
#endif
                break;
            }
#if defined _M_X64
            BOOL bProcessRip = FALSE;
            // mov or lea to register from rip+imm32
            if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA) && (pins->X86.Relative) &&
                (pins->X86.OperandSize == 8) && (pins->OperandCount == 2) &&
                (pins->Operands[1].Flags & OP_IPREL) && (pins->Operands[1].Register == AMD64_REG_RIP))
            {
                // rip-addressing "mov reg, [rip+imm32]"
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                OriginIGO::IGOLogDebug("DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%x (in memory: 0x%x)", 1, pins->X86.Displacement, *(PDWORD)(pLoc + 3));
#endif
                bProcessRip = TRUE;
            }
            // mov or lea to rip+imm32 from register
            else if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA) && (pins->X86.Relative) &&
                (pins->X86.OperandSize == 8) && (pins->OperandCount == 2) &&
                (pins->Operands[0].Flags & OP_IPREL) && (pins->Operands[0].Register == AMD64_REG_RIP))
            {
                // rip-addressing "mov [rip+imm32], reg"
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                OriginIGO::IGOLogDebug("DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%x (in memory: 0x%x)", 0, pins->X86.Displacement, *(PDWORD)(pLoc + 3));
#endif
                bProcessRip = TRUE;
            }
            else if ((pins->OperandCount >= 1) && (pins->Operands[0].Flags & OP_IPREL))
            {
                //if we've detected the D3D11.dll special case, we need to allow the disassembly to continue even when hitting a relative op function
                if (!earlyCallOpFlag && branchType == NONE)
                {
                    // unsupported rip-addressing
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                    OriginIGO::IGOLogDebug("DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 0);
#endif
                    // dump instruction bytes to the debug output
                    for (DWORD i = 0; i < pins->Length; i++) {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        OriginIGO::IGOLogDebug("DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]);
#endif
                    }

                    break;
                }
            }
            else if ((pins->OperandCount >= 2) && (pins->Operands[1].Flags & OP_IPREL))
            {
                // unsupported rip-addressing
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                OriginIGO::IGOLogDebug("DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 1);
#endif
                // dump instruction bytes to the debug output
                for (DWORD i = 0; i<pins->Length; i++) {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                    OriginIGO::IGOLogDebug("DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]);
#endif
                }

                if (!earlyCallOpFlag)
                {
                    //if we've detected the D3D11.dll special case, we need to allow the disassembly to continue even when hitting a relative op function
                    break;
                }
            }
            else if ((pins->OperandCount >= 3) && (pins->Operands[2].Flags & OP_IPREL))
            {
                // unsupported rip-addressing
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                OriginIGO::IGOLogDebug("DisassembleAndSkip: found unsupported OP_IPREL on operand %d", 2);
#endif
                // dump instruction bytes to the debug output
                for (DWORD i = 0; i<pins->Length; i++) {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                    OriginIGO::IGOLogDebug("DisassembleAndSkip: instr byte %2.2d: 0x%2.2x", i, pLoc[i]);
#endif
                }

                if (!earlyCallOpFlag)
                {
                    //if we've detected the D3D11.dll special case, we need to allow the disassembly to continue even when hitting a relative op function
                    break;
                }
            }
            // follow through with RIP-processing if needed
            if (bProcessRip) {
                // calculate displacement relative to function start
                S64 nAdjustedDisplacement = pins->X86.Displacement + ((pLoc + pins->Length)  - (U8*)pFunction);
                // store displacement values furthest from zero (both positive and negative)
                if (nAdjustedDisplacement < pdata->nLimitDown)
                    pdata->nLimitDown = nAdjustedDisplacement;
                if (nAdjustedDisplacement > pdata->nLimitUp)
                    pdata->nLimitUp = nAdjustedDisplacement;
                // store patch info
                if (pdata->nRipCnt < MHOOKS_MAX_RIPS) {
                    pdata->rips[pdata->nRipCnt].dwOffset = dwRet + 3;
                    pdata->rips[pdata->nRipCnt].nDisplacement = pins->X86.Displacement;
                    pdata->nRipCnt++;
                }
                else {
                    // no room for patch info, stop disassembly
                    break;
                }
            }
#endif

            dwRet += pins->Length;
            pLoc += pins->Length;
        } // while

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
        OriginIGO::IGOLogDebug("hook end");
#endif

        CloseDisassembler(&dis);
    }

    return dwRet;
}



// Exception handling material
typedef int (WINAPI *PFN_RTL_DISPATCH_EXCEPTION_NEXT)(EXCEPTION_RECORD *pExceptionRecord, CONTEXT *pExceptionContext);
typedef LPVOID(WINAPI *PFN_ADD_VECTORED_EXCEPTION_HANDLER)(ULONG FirstHandler, PVECTORED_EXCEPTION_HANDLER pVectoredHandler);
typedef ULONG(WINAPI *PFN_REMOVE_VECTORED_EXCEPTION_HANDLER)(LPVOID VectoredHandlerHandle);

#undef EXTERN
#ifdef _CCODEHOOK_C
#define EXTERN
#else
#define EXTERN extern
#endif

EXTERN ULONG_PTR gOldEip
#ifdef _CCODEHOOK_C
= 0
#endif
;
EXTERN ULONG_PTR gNewEip
#ifdef _CCODEHOOK_C
= 0
#endif
;

EXTERN PFN_RTL_DISPATCH_EXCEPTION_NEXT pfnRtlDispatchExceptionNext
#ifdef _CCODEHOOK_C
= NULL
#endif
;

EXTERN PFN_ADD_VECTORED_EXCEPTION_HANDLER pfnAddVectoredExceptionHandler
#ifdef _CCODEHOOK_C
= NULL
#endif
;

EXTERN PFN_REMOVE_VECTORED_EXCEPTION_HANDLER pfnRemoveVectoredExceptionHandler
#ifdef _CCODEHOOK_C
= NULL
#endif
;

int WINAPI RtlDispatchExceptionCallback(EXCEPTION_RECORD *pExceptionRecord, CONTEXT *pExceptionContext);
int WINAPI VectoredExceptionHandler(EXCEPTION_POINTERS *exceptionInfo);


BOOL InitRtlDispatchException(HANDLE& hMutex, LPVOID oldEip, LPVOID newEip)
{
    BOOL result = FALSE;

    pfnAddVectoredExceptionHandler = (PFN_ADD_VECTORED_EXCEPTION_HANDLER)KernelProc(CAddVecExceptHandler);
    pfnRemoveVectoredExceptionHandler = (PFN_REMOVE_VECTORED_EXCEPTION_HANDLER)KernelProc(CRemoveVecExceptHandler);
    // create unique mutext name...
    wchar_t mutexName[64] = { 0 };
    char buffer[32] = { 0 };
    DecryptStr(CMutex, buffer, 32);
    lstrcatA(buffer, ", rdec ");
    _snwprintf_s(mutexName, _countof(mutexName), _TRUNCATE, L"%S$%x", buffer, GetCurrentProcessId());
    hMutex = CreateMutex(NULL, FALSE, mutexName);
    if (hMutex != NULL)
    {
        WaitForSingleObject(hMutex, INFINITE);
        gOldEip = (ULONG_PTR)oldEip;
        gNewEip = (ULONG_PTR)newEip;
        if ((pfnAddVectoredExceptionHandler == NULL) || (pfnRemoveVectoredExceptionHandler == NULL))
        {
            // When would the above not be found?
            // Can this section be explained:
            LPVOID p = NtProc(CKiUserExceptionDispatcher, true);
            ULONG_PTR pd = (ULONG_PTR)p;
            DWORD p0 = *((DWORD *)(pd + 0));               // mov ecx, [esp+4] ; pContext
            DWORD p4 = *((DWORD *)(pd + 4)) & 0x00ffffff;  // mov ebx, [esp+0] ; pExceptionRecord
            BYTE  p7 = *((BYTE  *)(pd + 7));               // push ecx
            BYTE  p8 = *((BYTE  *)(pd + 8));               // push ebx
            BYTE  p9 = *((BYTE  *)(pd + 9));               // call RtlDispatchException

            DWORD oldProtect;
            // check Opcodes & memory protection
            if ((p0 == 0x04244c8b) && (p4 == 0x241c8b) && (p7 == 0x51) && (p8 == 0x53) && (p9 == 0xe8) &&
                VirtualProtect((LPVOID)(pd + 10), 4, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                pfnRtlDispatchExceptionNext = (PFN_RTL_DISPATCH_EXCEPTION_NEXT)((LPVOID)(pd + 14 + *((DWORD *)(pd + 10))));
                *((DWORD *)(pd + 10)) = (DWORD)((ULONG_PTR)RtlDispatchExceptionCallback - pd - 14);
                VirtualProtect((LPVOID)(pd + 10), 4, oldProtect, &oldProtect);
                result = TRUE;
            }
        }
        else
        {
            LPVOID pExceptionHandler = pfnAddVectoredExceptionHandler(1, (PVECTORED_EXCEPTION_HANDLER)VectoredExceptionHandler);
            result = (pExceptionHandler != NULL);
        }
    }

    if (result == FALSE)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        hMutex = NULL;
    }
    return result;
}

void CloseRtlDispatchException(HANDLE& hMutex)
{
    if (hMutex != NULL)
    {
        if ((pfnAddVectoredExceptionHandler == NULL) || (pfnRemoveVectoredExceptionHandler == NULL))
        {
            ULONG_PTR p = (ULONG_PTR)NtProc(CKiUserExceptionDispatcher, true);
            DWORD oldProtect;
            if (VirtualProtect((LPVOID)(p + 10), 4, PAGE_EXECUTE_READWRITE, &oldProtect))
            {
                *((DWORD *)(p + 10)) = (DWORD)((ULONG_PTR)pfnRtlDispatchExceptionNext - p - 14);
                VirtualProtect((LPVOID)(p + 10), 4, oldProtect, &oldProtect);
            }
        }
        else
        {
            pfnRemoveVectoredExceptionHandler(VectoredExceptionHandler);
        }

        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        hMutex = NULL;
    }
}


int roundUpToMultiple8(int number)
{
    int remainder = number % 8;
    if (remainder == 0)
        return number;
    return number + 8 - remainder;
}

//=========================================================================

BOOL Mhook_IsHook(PVOID *ppHookedFunction, PVOID unknownFunction) {
    EA::Thread::AutoRWSpinLock hookingLockInstanc(g_HookingLock, EA::Thread::AutoRWSpinLock::kLockTypeRead);

    if (ppHookedFunction != NULL && unknownFunction != NULL)
    {
        // get the trampoline structure that corresponds to our function
        MHOOKS_TRAMPOLINE* pTrampoline = TrampolineGet((PBYTE)*ppHookedFunction);

        if (pTrampoline)
        {
            if (pTrampoline->pUnalteredSystemFunction == unknownFunction)
                return TRUE;
        }
    }
    return FALSE;
}


BOOL Mhook_CheckHook(PVOID *ppHookedFunction, PVOID *ppSystemFunction, bool checkOriginalCode, bool insideHookCheck) {
    EA::Thread::AutoRWSpinLock hookingLockInstanc(g_HookingLock, EA::Thread::AutoRWSpinLock::kLockTypeRead);

    BOOL bRet = TRUE;  // default: hook was not altered 
    g_lastErrorDetails[0] = '\0';

    if (ppHookedFunction != NULL && ppSystemFunction != NULL)
    {
        // get the trampoline structure that corresponds to our function
        MHOOKS_TRAMPOLINE* pTrampoline = TrampolineGet((PBYTE)*ppHookedFunction);

        if (pTrampoline)
        {
            // Are we calling this from inside the hook function itself? If so, check how much of the original opcodes we were able to save - if large enough,
            // don't worry whether some other 3rd party hooked on top of us.
            // 1) The original CheckHook call from DEFINE_HOOK_SAFE_EX and co was for EBIBUGS-27031/CL#264653 - Fraps64 was using the set of instructions: 
            //    mov rax, 0x... - jmp rax (48 C7 C0 xx xx xx xx FF E0) which takes more room than our immediate JMP call (E9 xx xx xx xx), so if Fraps was injected while the Present call 
            //    was running, we would jump back to Present+5, which would be wrong! (should be at that point Present+9 AND we would have needed the save more of the original opcodes in our trampoline)
            // 2) Recently I updated the code to store as much of the original context as possible -> the previous scenario shouldn't be happening as long as we have store enough "context"/original opcodes
            // 3) The problem is right now, when Fraps gets injected, we immediately return 0 from the Present call -> the screen freezes while Fraps is loaded (if started and stopped after the game
            //    is started - if Fraps is started before the game, looks like when we close Fraps AFTER IGO is hooked in, the game can never recover from the freeze)
            // 4) Don't want to completely remove the CheckHook call if we hardly have room for our hook, although at that point other 3rd party would have the same space limitation
            if (insideHookCheck)
            {
                if (pTrampoline->cbOverwrittenCode >= 10)
                    return TRUE;
            }

            if (checkOriginalCode)  // this is CPU intensive
            {
                LPVOID pCode = FindRealCode(pTrampoline->pUnalteredSystemFunction);

                if (!pCode)
                {
                    OriginIGO::IGOLogWarn("CheckHook: Detected DLL unloaded - resetting hooks(%p).", pTrampoline->pUnalteredSystemFunction);
                    _snprintf(g_lastErrorDetails, sizeof(g_lastErrorDetails), "DLL unloaded");
                    CALL_ONCE_ONLY(OriginIGO::TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_MHOOK, OriginIGO::TelemetryRenderer_UNKNOWN, "Detected DLL unloaded - resetting hooks (%p).", pTrampoline->pUnalteredSystemFunction));
                    return FALSE;
                }

                HMODULE hModule = NULL;
                char moduleName[MAX_PATH];
                char apiName[MAX_PATH];
                *moduleName = '\0';
                *apiName = '\0';
                if (FindModule(pCode, &hModule, moduleName, MAX_PATH))
                {
                    GetImageProcName(hModule, pCode, apiName, MAX_PATH);
                }


                if (WasTargetChanged(hModule, pTrampoline->pUnalteredSystemFunction, pTrampoline->cbOverwrittenCode) == false)
                {
                    // hook no longer modified, so it is safe for rehook!
                    return bRet;
                }
            }
            // still modified, check if it is our modification....

            HANDLE hProc = GetCurrentProcess();
            DWORD dwOldProtectSystemFunction = 0;

            // real hooked destination
            {
                // make memory read+writable, at least 8 bytes for AtomicMove
                int instructLenRoundedToMultiple8 = roundUpToMultiple8(pTrampoline->cbOverwrittenCode);

                if (VirtualProtectEx(hProc, pTrampoline->pSystemFunction, eastl::max(8, instructLenRoundedToMultiple8), PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction))
                {
                    if (memcmp(pTrampoline->pbTempCode, pTrampoline->pSystemFunction, pTrampoline->cbOverwrittenCode) != 0)  // check our hook
                    {
                        bRet = FALSE;

                        _snprintf(g_lastErrorDetails, sizeof(g_lastErrorDetails), "Hook altered");
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        OriginIGO::IGOLogWarn("CheckHook: hook was altered!");
#endif
                    }

                    VirtualProtectEx(hProc, pTrampoline->pSystemFunction, eastl::max(8, instructLenRoundedToMultiple8), dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
                }
            }
        }

        if (bRet == TRUE)
        {
            if (pTrampoline)
            {
                // open ourselves so we can VirtualProtectEx
                HANDLE hProc = GetCurrentProcess();
                DWORD dwOldProtectSystemFunction = 0;

                // real system function
                {
                    // make memory read+writable
                    IGO_ASSERT(pTrampoline->codeHookedSystemFunctionSize > 0);
                    IGO_ASSERT(pTrampoline->codeHookedSystemFunctionSize <= MHOOKS_MAX_CODE_BYTES);
                    // make memory read+writable,
                    if (VirtualProtectEx(hProc, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction))
                    {
                        if (memcmp(pTrampoline->codeHookedSystemFunction, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize) != 0)  // check our hook
                        {
                            bRet = FALSE;
                            _snprintf(g_lastErrorDetails, sizeof(g_lastErrorDetails), "Original function altered");
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                            OriginIGO::IGOLogWarn("CheckHook: original function was altered, hook is broken!");
#endif
                        }

                        VirtualProtectEx(hProc, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
                    }
                }
            }
        }
    }

    return bRet;
}


BOOL Mhook_CheckTarget(PVOID pSystemFunction, DWORD opcodeSize) {
    EA::Thread::AutoRWSpinLock hookingLockInstanc(g_HookingLock, EA::Thread::AutoRWSpinLock::kLockTypeRead);

    BOOL bRet = FALSE;  // default: target was not altered 

    if (pSystemFunction != NULL)
    {
        LPVOID pCode = FindRealCode(pSystemFunction);

        if (!pCode)
        {
            OriginIGO::IGOLogWarn("CheckHook: Detected DLL unloaded - resetting hooks.");
            CALL_ONCE_ONLY(OriginIGO::TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_MHOOK, OriginIGO::TelemetryRenderer_UNKNOWN, "Detected DLL unloaded - resetting hooks (%p).", pSystemFunction));
            return bRet;
        }

        HMODULE hModule = NULL;
        char moduleName[MAX_PATH];
        char apiName[MAX_PATH];
        *moduleName = '\0';
        *apiName = '\0';
        if (FindModule(pCode, &hModule, moduleName, MAX_PATH))
        {
            GetImageProcName(hModule, pCode, apiName, MAX_PATH);
        }

        // pre check the API for modification, before we hook in
        if (WasTargetChanged(hModule, pSystemFunction, opcodeSize))
        {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
            OriginIGO::IGOLogWarn("Mhook_CheckTarget: To be hooked function (%s %s %p) is already altered.", moduleName, apiName, pSystemFunction);
#endif
            return TRUE;
        }

    }

    return bRet;
}

//=========================================================================
// This method gathers a bunch of driver/scenario-specific cases that involve having 'call'/'relative ops' opcodes in the set of opcodes we are saving 
// in the trampoline that would normally stop the jokking process
bool FunctionContainsScenarioSpecificPattern(const PVOID pSystemFunction, DWORD* defaultInstSize)
    {
    // The Present call in D3D11 - 6.1.7601.17541 has a call instruction within the first 5 bytes of the function.
    // This fails the check of the hooking code, where it requires the first 5 bytes of the function to not have any call,jmp,branch,ret instructions.
    // Normally when this is the case, we just will not hook and IGO will not work.

    // However we added this hack here to check for the exact 17 byte signature below. The 17 byte signature corresponds to the following instruction set:

    // For Win 7 64 bit
    //    000007FEF842D954 48 83 EC 28          sub         rsp,28h  
    //    000007FEF842D958 E8 B7 07 00 00       call        000007FEF842E114  
    //    000007FEF842D95D 8B D0                mov         edx,eax  
    //    000007FEF842D95F 81 EA 02 00 7A 08    sub         edx,87A0002h  

    //    000007FEF55805D8 48 83 EC 28          sub         rsp,28h  
    //    000007FEF55805DC E8 5F 14 00 00       call        000007FEF5581A40  
    //    000007FEF55805E1 8B D0                mov         edx,eax  
    //    000007FEF55805E3 81 EA 02 00 7A 08    sub         edx,87A0002h  

    // If we detect this instruction set, we will save off the first 17 bytes of the function and patch the relative addressing used in the call function

    // begin special case for the old 64 bit D3D11
    const int sigLength = 17;
    BYTE presentWithEarlyCallOpSigWin7[sigLength] = { 0x48, 0x83, 0xEC, 0x28, 0xE8, 0xB7, 0x07, 0x00, 0x00, 0x8B, 0xD0, 0x81, 0xEA, 0x02, 0x00, 0x7A, 0x08 };
    BYTE presentWithEarlyCallOpSigVista[sigLength] = { 0x48, 0x83, 0xEC, 0x28, 0xE8, 0x5F, 0x14, 0x00, 0x00, 0x8B, 0xD0, 0x81, 0xEA, 0x02, 0x00, 0x7A, 0x08 };

    DWORD i = 0;
    for (; i < sigLength; i++)
    {
        if ((presentWithEarlyCallOpSigWin7[i] != ((PBYTE)pSystemFunction)[i]) &&
            (presentWithEarlyCallOpSigVista[i] != ((PBYTE)pSystemFunction)[i]))
        {
            // we didn't find a matching signature so lets just use the minimum hook size
            break;
        }
    }

    if (i == sigLength)
    {
        if (defaultInstSize)
            *defaultInstSize = sigLength;

        return true;
    }

    return false;
}

//=========================================================================
BOOL Mhook_SetHook(HMODULE hModule, LPCSTR moduleName, LPCSTR apiName, PVOID *ppSystemFunction, PVOID pHookFunction, bool noJMPskip, bool noPreCheck) {
    EA::Thread::AutoRWSpinLock hookingLockInstanc(g_HookingLock, EA::Thread::AutoRWSpinLock::kLockTypeWrite);

    MHOOKS_TRAMPOLINE* pTrampoline = NULL;
    PVOID pSystemFunction = *ppSystemFunction;
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
    OriginIGO::IGOLogDebug("SetHook: Started on the job: %p / %p - %s %s", pSystemFunction, pHookFunction, moduleName, apiName);
#endif
    
    // figure out the length of the overwrite zone
    MHOOKS_PATCHDATA patchdata = { 0 };


    DWORD defaultInstSize = MHOOK_JMPSIZE;
    bool earlyCallOpFlag = FunctionContainsScenarioSpecificPattern(pSystemFunction, &defaultInstSize);

    BranchType branchType = NONE;
    // We want to grab/save as much of the original code as possible, as a 3rd party tool may hook on top of us, take more space than we do,
    // -> kaboom when we try to return to the original code from our hooked-in function (for example Fraps64 while hooking SwapChain->Present
    // (for example while playing Hardline)
    // Right now I'm using MHOOK_JUMPSIZE * 3 (15), but we could go up to MHOOKS_MAX_CODE_BYTES if necessary
    DWORD dwInstructionLength = DisassembleAndSkip(pSystemFunction, MHOOK_JMPSIZE * 3, &patchdata, earlyCallOpFlag, branchType);

    // pre check the API for modification, before we hook in
    if (!noPreCheck && WasTargetChanged(hModule, pSystemFunction, dwInstructionLength))
    {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
        // TODO: add code that follows the altered JMP target and hook that destination in the 3rd party module, like fraps64.dll !
        OriginIGO::IGOLogWarn("SetHook: To be hooked function is already hooked, hooking aborted for safety reasons! (length %i)", dwInstructionLength);
#endif
        return FALSE;
    }
    
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
    OriginIGO::IGOLogDebug("SetHook: dwInstructionLength %i", dwInstructionLength);
#endif
    if (dwInstructionLength > MHOOKS_MAX_CODE_BYTES)
    {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
        OriginIGO::IGOLogDebug("SetHook: max code bytes exceeded.");
#endif
    }
    else
        if (dwInstructionLength >= defaultInstSize)
        {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
            OriginIGO::IGOLogDebug("SetHook: disassembly signals %d bytes", dwInstructionLength);
#endif
            // allocate a trampoline structure (TODO: it is pretty wasteful to get
            // VirtualAlloc to grab chunks of memory smaller than 100 bytes)
            pTrampoline = TrampolineAlloc(hModule, moduleName, apiName, (PBYTE)pSystemFunction, patchdata.nLimitUp, patchdata.nLimitDown);
            if (pTrampoline) {

                //find the difference between our current address and our new address at the trampoline area
                if (earlyCallOpFlag)
                {
                    DWORD_PTR distance = (PBYTE)pTrampoline->codeTrampoline < (PBYTE)pSystemFunction ?
                        (PBYTE)pSystemFunction - (PBYTE)pTrampoline->codeTrampoline : (PBYTE)pTrampoline->codeTrampoline - (PBYTE)pSystemFunction;

                    if (distance > 0x7fffff00)
                    {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        OriginIGO::IGOLogDebug("SetHook: d3d11 present reroute -- bytes: %d -- to far to do relative jump, will NOT hook", distance);
#endif
                        // if the difference is too big to address the relative address in our call function, we should just fail the set hook and leave the function
                        TrampolineFree(pTrampoline);
                        pTrampoline = NULL;

                        OriginIGO::TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_MHOOK, OriginIGO::TelemetryRenderer_UNKNOWN, "SetHook: '%s/%s' -- bytes: %d -- too far to do relative jump, will NOT hook", apiName ? apiName : "", moduleName ? moduleName : "", distance);

                        //clean up

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        OriginIGO::IGOLogDebug("SetHook Failed Too Far: %s", apiName);
#endif
                        return false;
                    }
                }

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                OriginIGO::IGOLogDebug("SetHook: allocated structure at %p", pTrampoline);
#endif
                // open ourselves so we can VirtualProtectEx
                HANDLE hProc = GetCurrentProcess();
                DWORD dwOldProtectSystemFunction = 0;
                DWORD dwOldProtectTrampolineFunction = 0;
                // set the system function to PAGE_EXECUTE_READWRITE

                //we need to round up the length of data we need to copy to the next multiple of 8. The AtomicCopy function copies in chunks of 8. 
                unsigned int instructLenRoundedToMultiple8 = roundUpToMultiple8(dwInstructionLength);
                unsigned int minAtomicCopy = 8;

                if (VirtualProtectEx(hProc, pSystemFunction, eastl::max(minAtomicCopy, instructLenRoundedToMultiple8), PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction)) {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                    OriginIGO::IGOLogDebug("SetHook: readwrite set on system function");
#endif
                    // mark our trampoline buffer to PAGE_EXECUTE_READWRITE
                    if (VirtualProtectEx(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), PAGE_EXECUTE_READWRITE, &dwOldProtectTrampolineFunction)) {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        OriginIGO::IGOLogDebug("SetHook: readwrite set on trampoline structure");
#endif

                        // create our trampoline function
                        PBYTE pbCode = pTrampoline->codeTrampoline;
                        // save original code..
                        for (DWORD i = 0; i < eastl::max(minAtomicCopy, instructLenRoundedToMultiple8); i++) {
                            pTrampoline->codeUntouched[i] = pTrampoline->pbTempCode[i] = pbCode[i] = ((PBYTE)pSystemFunction)[i];
                        }

                        DWORD trampolineCodeLen = dwInstructionLength;
                        if (branchType != NONE)
                        {
                            switch (branchType)
                            {
                        case JMP8REL_RIP: // x64
                                {
                                    PBYTE originalJumpTarget = (((PBYTE)pSystemFunction) + patchdata.nBranchIndex + 2 + *(CHAR *)&((PBYTE)pSystemFunction)[patchdata.nBranchIndex + 1]);
                                    PBYTE trampolineCodeNextAddr = EmitJump(pTrampoline->codeTrampoline + patchdata.nBranchIndex, pTrampoline->codeTrampoline + patchdata.nBranchIndex, originalJumpTarget);
                                    trampolineCodeLen = static_cast<DWORD>(trampolineCodeNextAddr - pTrampoline->codeTrampoline);
                                }
                                break;

                        case JMP32REL: // x86
                        case JMP32REL_RIP: // x64
                                {
                                    PBYTE originalJumpTarget = (((PBYTE)pSystemFunction) + patchdata.nBranchIndex + 5 + *(INT32 *)&((PBYTE)pSystemFunction)[patchdata.nBranchIndex + 1]);
                                    PBYTE trampolineCodeNextAddr = EmitJump(pTrampoline->codeTrampoline + patchdata.nBranchIndex, pTrampoline->codeTrampoline + patchdata.nBranchIndex, originalJumpTarget);
                                    trampolineCodeLen = static_cast<DWORD>(trampolineCodeNextAddr - pTrampoline->codeTrampoline);
                                }
                                break;

                            default:
                                OriginIGO::IGOLogDebug("SetHook: illegal branch type");
                            }
                            }
                        else
                            if (earlyCallOpFlag)
                            {
                        // We have a 32-bit Call opcode (0xE8) in our trampoline code -> we need to update the address
                        PBYTE originalCallTarget = (((PBYTE)pSystemFunction) + patchdata.nBranchIndex + 5 + *(INT32 *)&((PBYTE)pSystemFunction)[patchdata.nBranchIndex + 1]);
                        bool isInRange = EmitRelativeCallOp(pTrampoline->codeTrampoline + patchdata.nBranchIndex, pTrampoline->codeTrampoline + patchdata.nBranchIndex, originalCallTarget);
                        if (!isInRange)
                        {
                            // If we get in there and we really need to hook this method, we'll need to update the way we allocate our trampolines
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                                OriginIGO::IGOLogDebug("SetHook: CALL too far to update, will NOT hook");
#endif
                            VirtualProtectEx(hProc, pSystemFunction, eastl::max(minAtomicCopy, instructLenRoundedToMultiple8), dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
                            VirtualProtectEx(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), dwOldProtectTrampolineFunction, &dwOldProtectTrampolineFunction); // not sure this is useful but...

                            TrampolineFree(pTrampoline);
                            pTrampoline = NULL;

                            OriginIGO::TelemetryDispatcher::instance()->Send(OriginIGO::TelemetryFcn_IGO_HOOKING_INFO, OriginIGO::TelemetryContext_MHOOK, OriginIGO::TelemetryRenderer_UNKNOWN, "SetHook: '%s/%s' -- CALL too far to update, will NOT hook", apiName ? apiName : "", moduleName ? moduleName : "");
                            return false;
                        }
                    }

                    // plus a jump to the continuation in the original location, except if already got a jump in the original opcodes
                    pbCode += trampolineCodeLen;
                    if (branchType == NONE)
                    {
                        pbCode = EmitJump(pbCode, pbCode, ((PBYTE)pSystemFunction) + dwInstructionLength);
                        trampolineCodeLen = static_cast<DWORD>(pbCode - pTrampoline->codeTrampoline);
                    }

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        OriginIGO::IGOLogDebug("SetHook: updated the trampoline");
#endif

                        // fix up any IP-relative addressing in the code
                        FixupIPRelativeAddressing(pTrampoline->codeTrampoline, (PBYTE)pSystemFunction, &patchdata);

                        DWORD_PTR dwDistance = (PBYTE)pHookFunction < (PBYTE)pSystemFunction ?
                            (PBYTE)pSystemFunction - (PBYTE)pHookFunction : (PBYTE)pHookFunction - (PBYTE)pSystemFunction;
                        if (dwDistance > 0x7fff0000) {
                            // create a stub that jumps to the replacement function.
                            // we need this because jumping from the API to the hook directly 
                            // will be a long jump, which is 14 bytes on x64, and we want to 
                            // avoid that - the API may or may not have room for such stuff. 
                            // (remember, we only have 5 bytes guaranteed in the API.)
                            // on the other hand we do have room, and the trampoline will always be
                            // within +/- 2GB of the API, so we do the long jump in there. 
                            // the API will jump to the "reverse trampoline" which
                            // will jump to the user's hook code.
                            pbCode = pTrampoline->codeJumpToHookFunction;
                            pbCode = EmitJump(pbCode, pbCode, (PBYTE)pHookFunction);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                            OriginIGO::IGOLogDebug("SetHook: created reverse trampoline");
#endif
                            FlushInstructionCache(hProc, pTrampoline->codeJumpToHookFunction, pbCode - pTrampoline->codeJumpToHookFunction);

                            // update the API itself
                            pbCode = (PBYTE)pTrampoline->pbTempCode;
                            pbCode = EmitJump(pbCode, (PBYTE)pSystemFunction, pTrampoline->codeJumpToHookFunction, true, dwInstructionLength);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                            OriginIGO::IGOLogDebug("SetHook: Hooked the function!");
#endif
                        }
                        else {
                            // the jump will be at most 5 bytes so we can do it directly
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                            OriginIGO::IGOLogDebug("SetHook: no need for reverse trampoline");
#endif
                            // update the API itself
                            pbCode = (PBYTE)pTrampoline->pbTempCode;
                            pbCode = EmitJump(pbCode, (PBYTE)pSystemFunction, (PBYTE)pHookFunction, true, dwInstructionLength);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                            OriginIGO::IGOLogDebug("SetHook: Hooked the function!");
#endif
                        }

                        // update data members
                        pTrampoline->cbOverwrittenCode = dwInstructionLength;
                        pTrampoline->pSystemFunction = (PBYTE)pSystemFunction;
                        pTrampoline->pUnalteredSystemFunction = (PBYTE)*ppSystemFunction;
                        pTrampoline->pHookFunction = (PBYTE)pHookFunction;

                        DWORD dwOldHookedFunction = 0;
                        // set the system function pointer to PAGE_EXECUTE_READWRITE
                        if (VirtualProtectEx(hProc, ppSystemFunction, sizeof(PVOID), PAGE_EXECUTE_READWRITE, &dwOldHookedFunction))
                        {
                            // this is what the application will use as the entry point
                            // to the "original" unhooked function.
                            *ppSystemFunction = pTrampoline->codeTrampoline;

                            FlushInstructionCache(hProc, ppSystemFunction, sizeof(PVOID));
                            VirtualProtectEx(hProc, ppSystemFunction, sizeof(PVOID), dwOldHookedFunction, &dwOldHookedFunction);
                        }


                        // flush instruction cache and restore original protection
                    unsigned int trampolineCodeRoundedLen = roundUpToMultiple8(trampolineCodeLen);
                    FlushInstructionCache(hProc, pTrampoline->codeTrampoline, trampolineCodeRoundedLen);
                        VirtualProtectEx(hProc, pTrampoline, sizeof(MHOOKS_TRAMPOLINE), dwOldProtectTrampolineFunction, &dwOldProtectTrampolineFunction);

                    }
                    else {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        DWORD dwError = ::GetLastError();
                        char* pError = NULL;
                        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pError, 0, NULL);
                        OriginIGO::IGOLogWarn("SetHook: failed VirtualProtectEx 2: (%x) % s.", dwError, pError);
                        LocalFree(pError);
#endif
                    }
                    // WARNING: Any change to this 'if' statement should be reflected in the corresponding 'if' statement in Mhook_Unhook() below.
                    //#ifndef _M_X64
                    //              if (dwInstructionLength>0 && dwInstructionLength<=8 && AtomicCopy8b(pTrampoline->pbTempCode, pSystemFunction, pTrampoline->codeUntouched))
                    //#else
                    if (dwInstructionLength > 0 && AtomicCopy(pTrampoline->pbTempCode, pSystemFunction, pTrampoline->codeUntouched, dwInstructionLength))
                        //#endif
                    {
                        // flush instruction cache
                        FlushInstructionCache(hProc, pSystemFunction, instructLenRoundedToMultiple8);
                    }
                    else
                    {
                        // if we failed discard the trampoline (forcing VirtualFree)
                        // first restore the systemFunction in case it was set to the codeTrampoline location above
                        DWORD dwOldHookedFunction = 0;
                        // set the system function pointer to PAGE_EXECUTE_READWRITE
                        if (VirtualProtectEx(hProc, ppSystemFunction, sizeof(PVOID), PAGE_EXECUTE_READWRITE, &dwOldHookedFunction))
                        {
                            if (ppSystemFunction)
                                *ppSystemFunction = pTrampoline->pUnalteredSystemFunction;

                            FlushInstructionCache(hProc, ppSystemFunction, sizeof(PVOID));
                            VirtualProtectEx(hProc, ppSystemFunction, sizeof(PVOID), dwOldHookedFunction, &dwOldHookedFunction);
                        }

                        TrampolineFree(pTrampoline);
                        pTrampoline = NULL;
                    }
                    // restore original protection
                    VirtualProtectEx(hProc, pSystemFunction, eastl::max(minAtomicCopy, instructLenRoundedToMultiple8), dwOldProtectSystemFunction, &dwOldProtectSystemFunction);

                    if (pTrampoline != NULL)
                    {
                        // fill with NOP opcodes to make is better readable in the debugger...
                        memset(pTrampoline->codeHookedSystemFunction, 0x90, sizeof(pTrampoline->codeHookedSystemFunction));

                        // make a backup of the hooked function code
                        pTrampoline->codeHookedSystemFunctionSize = static_cast<BYTE>(roundUpToMultiple8(FindFunctionEnd(pTrampoline->pUnalteredSystemFunction, MHOOKS_MAX_CODE_BYTES)));

                        IGO_ASSERT(pTrampoline->codeHookedSystemFunctionSize > 0);
                        IGO_ASSERT(pTrampoline->codeHookedSystemFunctionSize <= MHOOKS_MAX_CODE_BYTES);

                        if (VirtualProtectEx(hProc, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction))
                        {
                            AtomicCopy(pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunction, pTrampoline->codeHookedSystemFunction, pTrampoline->codeHookedSystemFunctionSize);

                            VirtualProtectEx(hProc, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
                        }
                    }
                }
                else {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                    DWORD dwError = ::GetLastError();
                    char* pError = NULL;
                    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pError, 0, NULL);
                    OriginIGO::IGOLogWarn("SetHook: failed VirtualProtectEx 1: (%x) % s.", dwError, pError);
                    LocalFree(pError);
#endif

                }

                if (pTrampoline != NULL && !pTrampoline->pSystemFunction)
                {
                    // if we failed discard the trampoline (forcing VirtualFree)
                    TrampolineFree(pTrampoline);
                    pTrampoline = NULL;
                }
            }

        }
        else {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
            OriginIGO::IGOLogDebug("SetHook: disassembly signals %d bytes (unacceptable)", dwInstructionLength);
#endif
        }


        if (!pTrampoline)
        {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
            OriginIGO::IGOLogDebug("SetHook Failed NULL Trampoline: %s", apiName);
#endif
        }
        return (pTrampoline != NULL);
}

//=========================================================================
BOOL Mhook_Unhook(PVOID *ppHookedFunction, PVOID *ppSystemFunction) {
    EA::Thread::AutoRWSpinLock hookingLockInstanc(g_HookingLock, EA::Thread::AutoRWSpinLock::kLockTypeWrite);

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
    OriginIGO::IGOLogDebug("Unhook: %p", *ppHookedFunction);
#endif
    BOOL bRet = FALSE;

    // get the trampoline structure that corresponds to our function
    MHOOKS_TRAMPOLINE* pTrampoline = TrampolineGet((PBYTE)*ppHookedFunction);

    if (pTrampoline)
    {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
        OriginIGO::IGOLogDebug("Unhook: found struct at %p", pTrampoline);
#endif
        // open ourselves so we can VirtualProtectEx
        HANDLE hProc = GetCurrentProcess();
        DWORD dwOldProtectSystemFunction = 0;
        // make memory writable, at least 8 bytes for AtomicMove
        int instructLenRoundedToMultiple8 = roundUpToMultiple8(pTrampoline->cbOverwrittenCode);

        if (VirtualProtectEx(hProc, pTrampoline->pSystemFunction, eastl::max(8, instructLenRoundedToMultiple8), PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction))
        {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
            OriginIGO::IGOLogDebug("Unhook: readwrite set on system function");
#endif
            PBYTE pbCode = (PBYTE)pTrampoline->pSystemFunction;

            // store our expected code for later verification
            memcpy(pTrampoline->pbTempCode, pbCode, eastl::max(8, instructLenRoundedToMultiple8));

            // WARNING: Any change to this 'if' statement should be reflected in the corresponding 'if' statement in Mhook_SetHook() above.
            //#ifndef _M_X64
            //          if (pTrampoline->cbOverwrittenCode>0 && pTrampoline->cbOverwrittenCode<=8 && AtomicCopy8b(pTrampoline->codeUntouched, pbCode, pTrampoline->pbTempCode)==TRUE)
            //#else
            if (pTrampoline->cbOverwrittenCode > 0 && AtomicCopy(pTrampoline->codeUntouched, pbCode, pTrampoline->pbTempCode, pTrampoline->cbOverwrittenCode) == TRUE)
                //#endif
            {
                DWORD dwOldHookedFunction = 0;
                // set the original system function pointer to PAGE_EXECUTE_READWRITE
                if (VirtualProtectEx(hProc, ppHookedFunction, sizeof(PVOID), PAGE_EXECUTE_READWRITE, &dwOldHookedFunction))
                {
                    // return the original function pointer
                    *ppHookedFunction = pTrampoline->pUnalteredSystemFunction;
                    FlushInstructionCache(hProc, ppHookedFunction, sizeof(PVOID));
                    VirtualProtectEx(hProc, ppHookedFunction, sizeof(PVOID), dwOldHookedFunction, &dwOldHookedFunction);
                }

                // flush instruction cache and make memory unwritable
                FlushInstructionCache(hProc, pTrampoline->pSystemFunction, pTrampoline->cbOverwrittenCode);
                bRet = TRUE;
                VirtualProtectEx(hProc, pTrampoline->pSystemFunction, eastl::max(8, instructLenRoundedToMultiple8), dwOldProtectSystemFunction, &dwOldProtectSystemFunction);

            }
            else
            {
                VirtualProtectEx(hProc, pTrampoline->pSystemFunction, eastl::max(8, instructLenRoundedToMultiple8), dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                OriginIGO::IGOLogDebug("Unhook: sysfunc: %p", *ppHookedFunction);
                OriginIGO::IGOLogDebug("Unhook: unhook failed");
#endif
            }
        }
        else
        {
            // this may fail, because of 3rd party apps that unhooked and invalidated our pointer, but also
            // because the game unloaded the DLL
            DWORD dwError = ::GetLastError();
            if (dwError == ERROR_INVALID_ADDRESS)
            {
                OriginIGO::IGOLogWarn("Unhook: failed VP, assuming unloaded DLL");

                // Not sure this is really useful, especially because this is our pointer/should
                // already have the proper permissions, but...
                DWORD dwOldHookedFunction = 0;
                if (VirtualProtectEx(hProc, ppHookedFunction, sizeof(PVOID), PAGE_EXECUTE_READWRITE, &dwOldHookedFunction))
                {
                    *ppHookedFunction = NULL;
                    FlushInstructionCache(hProc, ppHookedFunction, sizeof(PVOID));
                    VirtualProtectEx(hProc, ppHookedFunction, sizeof(PVOID), dwOldHookedFunction, &dwOldHookedFunction);
                }

                bRet = TRUE;
            }

            else
            {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)
                char* pError = NULL;
                FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pError, 0, NULL);
                OriginIGO::IGOLogWarn("Unhook: failed VirtualProtectEx 1: (%x) % s.", dwError, pError);
                LocalFree(pError);
#endif
            }
        }

#ifdef _DO_NOT_USE_THIS_CODE_YET__FORCED_CODE_RESTORE_FOR_HOOKED_FUNCTIONS_
        // do a sanity check.... if the code location is still modified after unhooking, restore the orrignal code from the dll binaries!
        // this will kill 3rd party hooks!
        {

            // pTrampoline->pSystemFunction; might point to a different location because of SkipJumps(...)
            PBYTE pbCode = (PBYTE)pTrampoline->pUnalteredSystemFunction;
            pbCode = (PBYTE)FindRealCode(pbCode);
            HMODULE hModule = NULL;
            char moduleName[MAX_PATH];
            char apiName[MAX_PATH];
            *moduleName = '\0';
            *apiName = '\0';

            if (FindModule(pbCode, &hModule, moduleName, MAX_PATH))
            {
                GetImageProcName(hModule, pbCode, apiName, MAX_PATH);
            }

            IGO_ASSERT(hModule);

            if (WasTargetChanged(hModule, pbCode))
            {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                OriginIGO::IGOLogWarn("Unhook: Orig. function after unhooking still modified. 3rd party app?");
#endif
                BranchType branchType = BranchType::NONE;
                DWORD nBranchIndex = FindAlteredTarget(pbCode, branchType);
                bool codeIsValid = false;

                if (branchType == BranchType::NONE)
                {

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                    OriginIGO::IGOLogWarn("Unhook: Unsupported branch.");
#endif

                }
                else
                {

                    int relativeAddrByteIndex = 0;  // this is the byte index of the start of the relative address 

                    switch (branchType)
                    {
                    case JMP16REL:
                        relativeAddrByteIndex = 1;
                        break;
                    case JMP32REL:
                        relativeAddrByteIndex = 2;
                        break;

                    case JMP8REL_RIP:
                        relativeAddrByteIndex = 0;
                        {
                            PBYTE originalJumpTarget = (((PBYTE)pbCode) + nBranchIndex + 2 + *(CHAR *)&((PBYTE)pbCode)[nBranchIndex + 1]);
                            // check altered target, if it ti still valid!
                            if (FindModule(originalJumpTarget, &hModule, moduleName, MAX_PATH))
                            {
                                GetImageProcName(hModule, originalJumpTarget, apiName, MAX_PATH);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                                OriginIGO::IGOLogWarn("Unhook: altered code still valid: %s %s", moduleName, apiName);
#endif
                                codeIsValid = true;
                            }
                            //EmitJump(pTrampoline->codeTrampoline, pTrampoline->codeTrampoline, originalJumpTarget);
                        }
                        break;

                    case JMP32REL_RIP:
                        relativeAddrByteIndex = 0;
                        {
                            PBYTE originalJumpTarget = (((PBYTE)pbCode) + nBranchIndex + 5 + *(INT32 *)&((PBYTE)pbCode)[nBranchIndex + 1]);
                            // check altered target, if it ti still valid!
                            if (FindModule(originalJumpTarget, &hModule, moduleName, MAX_PATH))
                            {
                                GetImageProcName(hModule, originalJumpTarget, apiName, MAX_PATH);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                                OriginIGO::IGOLogWarn("Unhook: altered code still valid: %s %s", moduleName, apiName);
#endif
                                codeIsValid = true;
                            }

                            //EmitJump(pTrampoline->codeTrampoline, pTrampoline->codeTrampoline, originalJumpTarget);
                        }
                        break;

                    case JMP32ABS:
                        relativeAddrByteIndex = 0;
                        {
                            PBYTE originalJumpTarget = ((PBYTE)*(INT32 *)&((PBYTE)pbCode)[nBranchIndex]);
                            // check altered target, if it ti still valid!
                            if (FindModule(originalJumpTarget, &hModule, moduleName, MAX_PATH))
                            {
                                GetImageProcName(hModule, originalJumpTarget, apiName, MAX_PATH);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                                OriginIGO::IGOLogWarn("Unhook: altered code still valid: %s %s", moduleName, apiName);
#endif
                                codeIsValid = true;
                            }

                            //EmitJump(pTrampoline->codeTrampoline, pTrampoline->codeTrampoline, originalJumpTarget);
                        }
                        break;

                    default:
                        OriginIGO::IGOLogDebug("Unhook: illegal branch type");

                    }

                    if (relativeAddrByteIndex != 0)
                    {
                        PBYTE originalJumpTarget = (((PBYTE)pbCode) + relativeAddrByteIndex + 5 + *(INT32 *)&((PBYTE)pbCode)[relativeAddrByteIndex + 1]);

                        // check altered target, if it ti still valid!
                        if (FindModule(originalJumpTarget, &hModule, moduleName, MAX_PATH))
                        {
                            GetImageProcName(hModule, originalJumpTarget, apiName, MAX_PATH);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                            OriginIGO::IGOLogWarn("Unhook: altered code still valid: %s %s", moduleName, apiName);
#endif
                            codeIsValid = true;
                        }
                    }
                }





                // if the modified code is invalid because the 3rd party dll is unloaded,
                // do a forced restore - this will kill 3rd party tools, but it is safe!!!
                if (!codeIsValid)
                {
                    // open ourselves so we can VirtualProtectEx
                    HANDLE hProc = GetCurrentProcess();
                    DWORD dwOldProtectSystemFunction = 0;

                    // make memory read+writable
                    IGO_ASSERT(pTrampoline->codeHookedSystemFunctionSize > 0);
                    IGO_ASSERT(pTrampoline->codeHookedSystemFunctionSize <= MHOOKS_MAX_CODE_BYTES);
                    // make memory read+writable,
                    if (VirtualProtectEx(hProc, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize, PAGE_EXECUTE_READWRITE, &dwOldProtectSystemFunction))
                    {

                        LPVOID buf = CCollectCache::CacheModuleFileMap(hModule);
                        if (buf)
                        {
                            IMAGE_NT_HEADERS *nh1 = GetImageNtHeaders(hModule);
                            DWORD address = VirtualToRaw(nh1, (DWORD)((ULONG_PTR)pbCode - (ULONG_PTR)hModule));
                            PIMAGE_NT_HEADERS nh2 = GetImageNtHeaders((HMODULE)buf);
                            if ((nh2) && (address < GetSizeOfImage(nh2)))
                            {

                                PBYTE originalImage = (PBYTE)((ULONG_PTR)buf + address);
                                BOOL result = AtomicCopy(originalImage, pTrampoline->pUnalteredSystemFunction, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize);

                                if (!result)
                                {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                                    OriginIGO::IGOLogWarn("Unhook: failed AtomicCopy");
#endif                                
                                }

#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                                OriginIGO::IGOLogWarn("CheckHook: original function was restored, this may break 3rd party hooks!");
#endif
                                FlushInstructionCache(hProc, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize);
                            }
                        }
                        VirtualProtectEx(hProc, pTrampoline->pUnalteredSystemFunction, pTrampoline->codeHookedSystemFunctionSize, dwOldProtectSystemFunction, &dwOldProtectSystemFunction);
                    }
                    else
                    {
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
                        DWORD dwError = ::GetLastError();
                        char* pError = NULL;
                        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pError, 0, NULL);
                        OriginIGO::IGOLogWarn("Unhook: failed VirtualProtectEx 2: (%x) % s.", dwError, pError);
                        LocalFree(pError);
#endif
                    }
                }
            }
        }
#endif

        // free the trampoline
        TrampolineFree(pTrampoline);
#if defined(_USE_IGO_LOGGER_) && defined(_DEBUG_HOOKING_)	
        OriginIGO::IGOLogDebug("Unhook: sysfunc: %p", *ppHookedFunction);
        OriginIGO::IGOLogDebug("Unhook: unhook successful");
#endif

    }

    return bRet;
}

//=========================================================================
#pragma warning(push)
#pragma warning(disable:4310) // cast truncates constant value
void Mhook_PreAllocateTrampolines(LPCWSTR moduleName)
{
    EA::Thread::AutoRWSpinLock hookingLockInstance(g_HookingLock, EA::Thread::AutoRWSpinLock::kLockTypeWrite);

    HMODULE hModule = GetModuleHandleW(moduleName);
    if (hModule)
    {
        // Have we already dealt with this DLL and is it loading in the same location it previously did? (an example of a DLL that loads multiple times
        // would be a shader compiler)
        size_t hash = eastl::hash<wchar_t*>()((wchar_t*)moduleName);
        CheckedDllMap::const_iterator iter = g_pCheckedDlls.find(hash);
        if (iter != g_pCheckedDlls.end())
        {
            if (iter->second == hModule)
            {
                OriginIGO::IGOLogInfo("PreAllocateTrampolines: already handled %S (%u), %p", moduleName, hash, hModule);
                return;
            }
        }

        g_pCheckedDlls[hash] = hModule;

        PBYTE pLower = (PBYTE)hModule;
        pLower = pLower < (PBYTE)(DWORD_PTR)0x0000000080000000 ? (PBYTE)(0x1) : (PBYTE)(pLower - (PBYTE)0x7fff0000);
        PBYTE pUpper = (PBYTE)hModule;
        pUpper = pUpper < (PBYTE)(DWORD_PTR)0xffffffff80000000 ? (PBYTE)(pUpper + (DWORD_PTR)0x7ff80000) : (PBYTE)(DWORD_PTR)0xfffffffffff80000;

        OriginIGO::IGOLogInfo("PreAllocateTrampolines: %S, %p (min %p, max %p)", moduleName, hModule, pLower, pUpper);
		
        MHOOKS_TRAMPOLINE* newHeadList = BlockAlloc((PBYTE)hModule, pLower, pUpper, true);
        if (newHeadList)
            g_pFreeList = newHeadList;
    }
}
#pragma warning(pop)

//=========================================================================

bool Mhook_LastErrorDetails(char* buffer, size_t bufferSize)
{
    EA::Thread::AutoRWSpinLock hookingLockInstanc(g_HookingLock, EA::Thread::AutoRWSpinLock::kLockTypeWrite);
    
    if (!buffer || bufferSize == 0)
        return false;

    if (!g_lastErrorDetails[0])
    {
        buffer[0] = '\0';
        return false;
    }

    strncpy(buffer, g_lastErrorDetails, bufferSize);
    buffer[bufferSize - 1] = '\0';

    g_lastErrorDetails[0] = '\0';
    return true;
}

//=========================================================================

