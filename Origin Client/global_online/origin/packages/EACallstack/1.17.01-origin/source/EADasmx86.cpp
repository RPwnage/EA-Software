///////////////////////////////////////////////////////////////////////////////
// EADasmx86.cpp
//
// Copyright (c) 2005, Electronic Arts. All rights reserved.
// Maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/EADasm.h>

#if EACALLSTACK_DASM_X86_ENABLED

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#ifdef EA_PLATFORM_WIN32
    #include <EACallstack/Win32/DasmX86Dll.h>
    #pragma warning(push, 0)
    #include <Windows.h>
    #pragma warning(pop)
#endif


namespace EA
{

namespace Dasm
{


///////////////////////////////////////////////////////////////////////////////
// DisassemblerX86
///////////////////////////////////////////////////////////////////////////////

#ifdef EA_PLATFORM_WIN32
    typedef int          (*DasmX86_initFunction)(x86_options, DISASM_REPORTER, void*);
    typedef int          (*DasmX86_cleanupFunction)();
    typedef unsigned int (*DasmX86_disasmFunction)(unsigned char*, unsigned int, unsigned long, unsigned int, x86_insn_t*);
    typedef int          (*DasmX86_format_insnFunction)(x86_insn_t*, char*, int, x86_asm_format);

    static DasmX86_initFunction        gpDasmX86_init        = NULL;
    static DasmX86_cleanupFunction     gpDasmX86_cleanup     = NULL;
    static DasmX86_disasmFunction      gpDasmX86_disasm      = NULL;
    static DasmX86_format_insnFunction gpDasmX86_format_insn = NULL;
    static HMODULE                     hDasmX86Dll           = NULL;
#endif


///////////////////////////////////////////////////////////////////////////////
// DisassemblerX86
//
DisassemblerX86::DisassemblerX86()
{
    #ifdef EA_PLATFORM_WIN32
        if(!hDasmX86Dll)
        {
            hDasmX86Dll = LoadLibraryA("DasmX86Dll.dll");

            if(hDasmX86Dll)
            {
                gpDasmX86_init        = (DasmX86_initFunction)       (uintptr_t)GetProcAddress(hDasmX86Dll, "DasmX86_init");
                gpDasmX86_cleanup     = (DasmX86_cleanupFunction)    (uintptr_t)GetProcAddress(hDasmX86Dll, "DasmX86_cleanup");
                gpDasmX86_disasm      = (DasmX86_disasmFunction)     (uintptr_t)GetProcAddress(hDasmX86Dll, "DasmX86_disasm");
                gpDasmX86_format_insn = (DasmX86_format_insnFunction)(uintptr_t)GetProcAddress(hDasmX86Dll, "DasmX86_format_insn");

                if(gpDasmX86_init)
                    gpDasmX86_init(opt_none, NULL, NULL);
            }
        }
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// ~DisassemblerX86
//
DisassemblerX86::~DisassemblerX86()
{
    #ifdef EA_PLATFORM_WIN32
        if(gpDasmX86_cleanup)
            gpDasmX86_cleanup();

        // We don't unload hDasmX86Dll, as other instances of this class might use it. 
    #endif
}



///////////////////////////////////////////////////////////////////////////////
// Dasm
//
const void* DisassemblerX86::Dasm(const void* pData, const void* pDataEnd, 
                                    DasmData& dd, uint32_t optionFlags, uint64_t dataAddress)
{
    // Use Microsoft's MSDIS140.dll or our DasmX86DLL.dll to do the work.
    // Currently we use the latter, but if we can find out how the former works (it is 
    // currently not publicly documented), then it would probably be preferable to use it.

    dd.ClearLastInstruction();

    if(pData >= pDataEnd) // If there isn't space for one instruction...
    {
        dd.mInstructionFlags = kIFInvalid;
        return pDataEnd;
    }

    dd.mOptionFlags   |= optionFlags;
    dd.mProcessorType  = kProcessorX86;

    // If this is the first time in use, set the base address.
    if(dd.mBaseAddress == 0)
        dd.mBaseAddress = dataAddress;

    // Print mInstructionAddress
    // mInstructionAddress is the address for this particular instruction. mBaseAddress is
    // the address for the first instruction in the span of instructions we are using DasmData for.
    // So mInstructionAddress is always >= mBaseAddress. 
    //if(!dd.mInstructionAddress) This doesn't work right. // If the user didn't specify it already...
    {
        if(dataAddress)
            dd.mInstructionAddress = dataAddress;
        else
            dd.mInstructionAddress = (uint64_t)(uintptr_t)pData;
    }

    sprintf(dd.mAddress, "%08x", (unsigned)dd.mInstructionAddress);


    // Print the instruction (operation and operands)
    #ifdef EA_PLATFORM_WIN32
        if(gpDasmX86_init) // If we were able to load the library...
        {
            x86_insn_t     insn;
            const unsigned dataLength = (unsigned)((uintptr_t)pDataEnd - (uintptr_t)pData);
            int            instructionSize = (int)gpDasmX86_disasm((unsigned char*)const_cast<void*>(pData), dataLength, (unsigned long)dd.mInstructionAddress, 0, &insn);
            const char*    pData8 = (const char*)pData;

            if(instructionSize) // If the instruction was valid...
            {
                // To do: Break up the operation into mOperation and mOperands.
                gpDasmX86_format_insn(&insn, dd.mOperation, sizeof(dd.mOperation), intel_syntax);
            }
            else
            {
                sprintf(dd.mOperation, "(invalid) 0x%02x", *(const uint8_t*)pData); // e.g. "invalid 0xdd"
                instructionSize = 1;
                dd.mOperands[0] = 0;
                dd.mInstructionFlags |= kIFInvalid;
            }

            char* pDest = dd.mInstruction;
            for(int i = 0; (i < instructionSize) && ((pDest + 3) < (dd.mInstruction + sizeof(dd.mInstruction))); i++, pDest += 2)
                EA::StdC::Sprintf(pDest, "%02x", *pData8++);

            return (const char*)pData + instructionSize;
        }
        else
        {
            sprintf(dd.mOperation, "DasmX86Dll.dll not found.");
            dd.mOperands[0] = 0;
            dd.mInstructionFlags |= kIFInvalid;

            return pDataEnd;
        }
    #else
        // Currently, because we use the DLL approach, we can only disassemble x86 code on 
        // a Windows machine. It turns out that the underlying source code within the DLL
        // is platform independent, so we can make this work cross-platform if we need to.
        sprintf(dd.mOperation, "Disassember not available");
        dd.mOperands[0] = 0;
        dd.mInstructionFlags |= kIFInvalid;

        return pDataEnd;
    #endif
}



} // namespace Dasm

} // namespace EA


#endif // EACALLSTACK_DASM_X86_ENABLED




