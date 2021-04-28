/*******************************************************************************
 mach_override.c
 Copyright (c) 2003-2009 Jonathan 'Wolf' Rentzsch: <http://rentzsch.com>
 Some rights reserved: <http://opensource.org/licenses/mit-license.php>
 
 ***************************************************************************/

#include "MacMach_override.h"

#include <mach-o/dyld.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <sys/mman.h>
#include <syslog.h>

#include <CoreServices/CoreServices.h>

// ================================Origin-specific changes==================================

static disasmLogInfo InfoLogger = NULL;
static disasmLogError ErrorLogger = NULL;

#define MachLogInfo(...)        machLogInfo(__FILE__, __LINE__, 0, __VA_ARGS__)
#define MachLogInfoAlways(...)  machLogInfo(__FILE__, __LINE__, 1, __VA_ARGS__)
#define MachLogError(...)       machLogError(__FILE__, __LINE__, __VA_ARGS__)


#define USE_DISASSEMBLER

static const unsigned int MHOOKS_MAX_RIPS = 4;

// The patch data structures - store info about rip-relative instructions during hook placement
typedef struct
{
    S64		nDisplacement;
    DWORD	dwOffset;
    U32     instructionLength;
    U8      opcode0;
    
} MHOOKS_RIPINFO;

typedef struct
{
    S64				nLimitUp;
    S64				nLimitDown;
    DWORD			nRipCnt;
    MHOOKS_RIPINFO	rips[MHOOKS_MAX_RIPS];
} MHOOKS_PATCHDATA;

enum BranchType{
    NONE = 0,
    JMP16REL,
    JMP32REL,
    JMP8REL_RIP,
    JMP32REL_RIP,
    JMP32ABS,
};

void machLogInfo(const char *file, int line, int alwaysLogMessage, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    if (InfoLogger)
        InfoLogger(file, line, alwaysLogMessage, fmt, args);
    
    va_end(args);    
}

void machLogError(const char *file, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    
    if (ErrorLogger)
        ErrorLogger(file, line, fmt, args);
    
    va_end(args);    
}

void mach_setup_logging(disasmLogInfo infoLogger, disasmLogError errorLogger)
{
    InfoLogger = infoLogger;
    ErrorLogger = errorLogger;
    
    DisasmSetupLogging(infoLogger, errorLogger);
}

// Based on PC version:
// Examine the machine code at the target function's entry point, and
// skip bytes in a way that we'll always end on an instruction boundary.
// We also detect branches and subroutine calls (as well as returns)
// at which point disassembly must stop.
// Finally, detect and collect information on IP-relative instructions
// that we can patch.

static DWORD DisassembleAndSkip(PVOID pFunction, DWORD dwMinLen, MHOOKS_PATCHDATA* pdata)
{
    DWORD dwRet = 0;
    pdata->nLimitDown = 0;
    pdata->nLimitUp = 0;
    pdata->nRipCnt = 0;
    
#if defined(__i386__)
    ARCHITECTURE_TYPE arch = ARCH_X86;
#elif defined(__x86_64__)
    ARCHITECTURE_TYPE arch = ARCH_X64;
#else
#error unsupported platform
#endif
    DISASSEMBLER dis;
    
    
    
    
    if (InitDisassembler(&dis, arch)) {
        U8* pLoc = (U8*)pFunction;
        DWORD dwFlags = DISASM_DECODE | DISASM_DISASSEMBLE | DISASM_ALIGNOUTPUT | DISASM_SUPPRESSERRORS;
        
        MachLogInfo("DisassembleAndSkip: Disassembling %p", pLoc);

        
        while (dwRet < dwMinLen)
        {
            BOOL bProcessRip = FALSE;
            INSTRUCTION* pins = GetInstruction(&dis, (ULONG_PTR)pLoc, pLoc, dwFlags);
            if (!pins)
                break;
            
            {
                size_t cnt = 1; // null character
                char tmp[8] = {0};
                char instructions[256] = {0};
                
                for (DWORD i = 0; i<pins->Length; i++)
                {
                    int len = snprintf(tmp, sizeof(tmp), "%s0x%02x", (i > 0) ? ", " : "", pLoc[i]);

                    cnt += len;
                    if (cnt >= sizeof(instructions))
                        break;
                    
                    strcat(instructions, tmp);
                }
                
                MachLogInfoAlways("DisassembleAndSkip: %p: %s (Opcode=%d, Len=%d - Inst:%s)", pLoc, pins->String, pins->OpcodeBytes[0], pins->Length, instructions);
            }
            
            if (pins->Type == ITYPE_RET)
            {
                MachLogError("DisassembleAndSkip: UNSUPPORTED RET");
                break;
            }
            
            else
            if (pins->Type == ITYPE_BRANCH)
            {
                if (pins->OpcodeBytes[0] == 0xE9 && pins->Length == 5)
                {
                    MachLogInfo("DisassembleAndSkip: JMP32REL BRANCH");
                    bProcessRip = TRUE;
                }
                
                else
                if (pins->OpcodeBytes[0] == 0xFF && (pins->Length == 5 || pins->Length == 6))
                {
                    MachLogInfo("DisassembleAndSkip: JMP32IN BRANCH");
                }
                else
                {
                    MachLogError("DisassembleAndSkip: UNSUPPORTED BRANCH OPCODE");
                    break;
                }
            }
            
            else
            if (pins->Type == ITYPE_BRANCHCC)
            {
                MachLogError("DisassembleAndSkip: UNSUPPORTED BRANCHCC OPCODE");
                break;
            }
    
            else
            if (pins->Type == ITYPE_CALL)
            {
                if (pins->OpcodeBytes[0] == 0xE8 && pins->Length == 5)
                {
                    MachLogInfo("DisassembleAndSkip: RELATIVE CALL");
                    bProcessRip = TRUE;
                }
                
                else
                {
                    MachLogError("DisassembleAndSkip: UNSUPPORTED CALL OPCODE");
                    break;
                }
            }
        
            else
            if (pins->Type == ITYPE_CALLCC)
            {
                MachLogError("DisassembleAndSkip: UNSUPPORTED CALLCC OPCODE");
                break;
            }

            else // mov or lea to register from rip+imm32
            if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA || pins->Type == ITYPE_CMP) && (pins->X86.Relative)
                && (pins->OperandCount == 2) && (pins->Operands[1].Flags & OP_IPREL)
                && (pins->Operands[1].Register == AMD64_REG_RIP || pins->Operands[1].Register == X86_REG_EIP))
            {
                // rip-addressing "mov reg, [rip+imm32]"
                int offset = pins->PrefixCount + pins->OpcodeLength + pins->X86.HasModRM + pins->X86.HasIndexRegister;
                MachLogInfo("DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%llx (in memory: 0x%lx)", 1, pins->X86.Displacement, *(PDWORD)(pLoc + offset));
                
                bProcessRip = TRUE;
            }

            else // mov or lea to rip+imm32 from register
            if ((pins->Type == ITYPE_MOV || pins->Type == ITYPE_LEA || pins->Type == ITYPE_CMP) && (pins->X86.Relative)
                && (pins->OperandCount == 2) && (pins->Operands[0].Flags & OP_IPREL)
                && (pins->Operands[0].Register == AMD64_REG_RIP || pins->Operands[0].Register == X86_REG_EIP))
            {
                // rip-addressing "mov [rip+imm32], reg"
                int offset = pins->PrefixCount + pins->OpcodeLength + pins->X86.HasModRM + pins->X86.HasIndexRegister;
                MachLogInfo("DisassembleAndSkip: found OP_IPREL on operand %d with displacement 0x%llx (in memory: 0x%lx)", 0, pins->X86.Displacement, *(PDWORD)(pLoc + offset));
                
                bProcessRip = TRUE;
            }

            else
            if ((pins->OperandCount >= 1) && (pins->Operands[0].Flags & OP_IPREL))
            {
                // unsupported rip-addressing
                MachLogError("DisassembleAndSkip: found unsupported OP_IPREL on operand 0");
                break;
            }

            else
            if ((pins->OperandCount >= 2) && (pins->Operands[1].Flags & OP_IPREL))
            {
                // unsupported rip-addressing
                MachLogError("DisassembleAndSkip: found unsupported OP_IPREL on operand 1");
                break;
            }

            else
            if ((pins->OperandCount >= 3) && (pins->Operands[2].Flags & OP_IPREL))
            {
                // unsupported rip-addressing
                MachLogError("DisassembleAndSkip: found unsupported OP_IPREL on operand 2");
                
                break;
            }

            // follow through with RIP-processing if needed
            if (bProcessRip)
            {
                // calculate displacement relative to function start
                S64 nAdjustedDisplacement = pins->X86.Displacement + ((pLoc + pins->Length)  - (U8*)pFunction);
                // store displacement values furthest from zero (both positive and negative)
                if (nAdjustedDisplacement < pdata->nLimitDown)
                    pdata->nLimitDown = nAdjustedDisplacement;
                
                if (nAdjustedDisplacement > pdata->nLimitUp)
                    pdata->nLimitUp = nAdjustedDisplacement;
                
                // store patch info
                if (pdata->nRipCnt < MHOOKS_MAX_RIPS)
                {
                    pdata->rips[pdata->nRipCnt].nDisplacement = pins->X86.Displacement;
                    pdata->rips[pdata->nRipCnt].dwOffset = dwRet + pins->PrefixCount + pins->OpcodeLength + pins->X86.HasModRM + pins->X86.HasIndexRegister;
                    pdata->rips[pdata->nRipCnt].opcode0 = pins->OpcodeBytes[0];
                    pdata->rips[pdata->nRipCnt].instructionLength = pins->Length;
                    
                    MachLogInfo("DisassembleAndSkip: RIP %d - displacement=%d, offset=%d", pdata->nRipCnt, pdata->rips[pdata->nRipCnt].nDisplacement, pdata->rips[pdata->nRipCnt].dwOffset);
                    
                    pdata->nRipCnt++;
                }
                else
                {
                    // no room for patch info, stop disassembly
                    break;
                }
            }
            
            dwRet += pins->Length;
            pLoc += pins->Length;
        } // while
        
        CloseDisassembler(&dis);
    }
    
    return dwRet;
}

// if IP-relative addressing has been detected, fix up the code so the offset points to the original location
static void FixupIPRelativeAddressing(char* pbNew, char* pbOriginal, MHOOKS_PATCHDATA* pdata)
{
    S64 diff = pbNew - pbOriginal;
    for (uint32_t i = 0; i < pdata->nRipCnt; i++)
    {
        // Special case: use a relative call 0x00000000 to compute the offset to some data (ie ptrace) -> replace call instruction
        // with a push to the real next address of the opcode
        if (pdata->rips[i].opcode0 == 0xE8 && pdata->rips[i].instructionLength == 5 && pdata->rips[i].nDisplacement == 0)
        {
            char* pushInst = pbNew + pdata->rips[i].dwOffset - 1 /* opcode */;
            intptr_t nextAddr = (intptr_t)pbOriginal + pdata->rips[i].dwOffset + 4 /* S32 */;
            
            pushInst[0] = 0x68;
            pushInst[1] = nextAddr & 0x000000ff;
            pushInst[2] = ((nextAddr & 0x0000ff00) >> 8);
            pushInst[3] = ((nextAddr & 0x00ff0000) >> 16);
            pushInst[4] = ((nextAddr & 0xff000000) >> 24);
            
            MachLogInfo("FixupRIP %d: special 0xE8 case - %p", i, nextAddr);
        }
        
        else
        {
            S32 dwNewDisplacement = (S32)(pdata->rips[i].nDisplacement - diff);
            
            MachLogInfo("Fixing up RIP instruction operand for code at 0x%p: old displacement: 0x%8.8x, new displacement: 0x%8.8x",
                       pbNew + pdata->rips[i].dwOffset, (S32)pdata->rips[i].nDisplacement, dwNewDisplacement);
            
            *(S32*)(pbNew + pdata->rips[i].dwOffset) = dwNewDisplacement;
        }
    }
}


// ================================Origin-specific changes==================================

/**************************
 *
 *    Constants
 *
 **************************/
#pragma mark    -
#pragma mark    (Constants)

#if defined(__ppc__) || defined(__POWERPC__)

long kIslandTemplate[] = {
    0x9001FFFC,    //    stw        r0,-4(SP)
    0x3C00DEAD,    //    lis        r0,0xDEAD
    0x6000BEEF,    //    ori        r0,r0,0xBEEF
    0x7C0903A6,    //    mtctr    r0
    0x8001FFFC,    //    lwz        r0,-4(SP)
    0x60000000,    //    nop        ; optionally replaced
    0x4E800420     //    bctr
};

#define kAddressHi            3
#define kAddressLo            5
#define kInstructionHi        10
#define kInstructionLo        11

#elif defined(__i386__)

#define kOriginalInstructionsSize 16

char kIslandTemplate[] = {
    // kOriginalInstructionsSize nop instructions so that we
    // should have enough space to host original instructions
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    // Now the real jump instruction
    0xE9, 0xEF, 0xBE, 0xAD, 0xDE
};

#define kInstructions    0
#define kJumpAddress    kInstructions + kOriginalInstructionsSize + 1
#elif defined(__x86_64__)

#define kOriginalInstructionsSize 32
#define kInstructions    0
#define kJumpAddress    kInstructions + kOriginalInstructionsSize + 6

char kIslandTemplate[] = {
    // kOriginalInstructionsSize nop instructions so that we
    // should have enough space to host original instructions
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
    // Now the real jump instruction
    0xFF, 0x25, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

#endif

#define    kAllocateHigh        1
#define    kAllocateNormal        0

/**************************
 *
 *    Data Types
 *
 **************************/
#pragma mark    -
#pragma mark    (Data Types)

typedef    struct    {
    char    instructions[sizeof(kIslandTemplate)];
    int        allocatedHigh;
}    BranchIsland;

/**************************
 *
 *    Funky Protos
 *
 **************************/
#pragma mark    -
#pragma mark    (Funky Protos)

mach_error_t
allocateBranchIsland(
                     BranchIsland    **island,
                     int                allocateHigh,
                     void *originalFunctionAddress);

mach_error_t
freeBranchIsland(
                 BranchIsland    *island );

#if defined(__ppc__) || defined(__POWERPC__)
mach_error_t
setBranchIslandTarget(
                      BranchIsland    *island,
                      const void        *branchTo,
                      long            instruction );
#endif

#if defined(__i386__) || defined(__x86_64__)
mach_error_t
setBranchIslandTarget_i386(
                           BranchIsland*        island,
                           const void*          branchTo,
                           char*                originalCode,
                           char*                instructions,
                           MHOOKS_PATCHDATA*    patchData);
void
atomic_mov64(
             uint64_t *targetAddress,
             uint64_t value );

static Boolean
eatKnownInstructions(
                     unsigned char      *code,
                     uint64_t           *newInstruction,
                     int                *howManyEaten,
                     char               *originalInstructions,
                     MHOOKS_PATCHDATA   *patchdata );
#endif

/*******************************************************************************
 *
 *    Interface
 *
 *******************************************************************************/
#pragma mark    -
#pragma mark    (Interface)

#if defined(__i386__) || defined(__x86_64__)
mach_error_t makeIslandExecutable(void *address) {
    mach_error_t err = err_none;
    vm_size_t pageSize;
    host_page_size( mach_host_self(), &pageSize );
    uintptr_t page = (uintptr_t)address & ~(uintptr_t)(pageSize-1);
    int e = err_none;
    e |= mprotect((void *)page, pageSize, PROT_EXEC | PROT_READ | PROT_WRITE);
    e |= msync((void *)page, pageSize, MS_INVALIDATE );
    if (e) {
        err = err_cannot_override;
    }
    return err;
}
#endif

mach_error_t
mach_override_ptr(
                  void *originalFunctionAddress,
                  const void *overrideFunctionAddress,
                  void **originalFunctionReentryIsland )
{
    assert( originalFunctionAddress );
    assert( overrideFunctionAddress );
    
    // this addresses overriding such functions as AudioOutputUnitStart()
    // test with modified DefaultOutputUnit project
#if defined(__x86_64__) || defined(__i386__)
    for(;;){
        if(*(unsigned char*)originalFunctionAddress==0xE9)      // jmp .+0x????????
            originalFunctionAddress=(void*)((char*)originalFunctionAddress+5+*(int32_t *)((char*)originalFunctionAddress+1));
#if defined(__x86_64__)
        else if(*(uint16_t*)originalFunctionAddress==0x25FF)    // jmp qword near [rip+0x????????]
            originalFunctionAddress=*(void**)((char*)originalFunctionAddress+6+*(int32_t *)((uint16_t*)originalFunctionAddress+1));
#elif defined(__i386__)
        else if(*(uint16_t*)originalFunctionAddress==0x25FF)    // jmp *0x????????
            originalFunctionAddress=**(void***)((uint16_t*)originalFunctionAddress+1);
#endif
        else break;
    }
#endif
    
    long    *originalFunctionPtr = (long*) originalFunctionAddress;
    mach_error_t    err = err_none;
    
#if defined(__ppc__) || defined(__POWERPC__)
    //    Ensure first instruction isn't 'mfctr'.
#define    kMFCTRMask            0xfc1fffff
#define    kMFCTRInstruction    0x7c0903a6
    
    long    originalInstruction = *originalFunctionPtr;
    if( !err && ((originalInstruction & kMFCTRMask) == kMFCTRInstruction) )
        err = err_cannot_override;
#elif defined(__i386__) || defined(__x86_64__)
    int eatenCount = 0;
    char originalInstructions[kOriginalInstructionsSize];
    uint64_t jumpRelativeInstruction = 0; // JMP
    MHOOKS_PATCHDATA patchData = { 0 };
    
    Boolean overridePossible = eatKnownInstructions ((unsigned char *)originalFunctionPtr, &jumpRelativeInstruction, &eatenCount, originalInstructions, &patchData);
    if (eatenCount > kOriginalInstructionsSize) {
        MachLogError("Override: Too many instructions eaten\n");

        overridePossible = false;
    }
    if (!overridePossible) err = err_cannot_override;
    
    if (err) MachLogError("err = %x %d\n", err, __LINE__);
    
#endif
    
    //    Make the original function implementation writable.
    if( !err ) {
        err = vm_protect( mach_task_self(),
                         (vm_address_t) originalFunctionPtr, 8, false,
                         (VM_PROT_ALL | VM_PROT_COPY) );
        if( err )
            err = vm_protect( mach_task_self(),
                             (vm_address_t) originalFunctionPtr, 8, false,
                             (VM_PROT_DEFAULT | VM_PROT_COPY) );
    }
    
    if (err) MachLogError("Failed to change original VM_PROT - err = %x %d\n", err, __LINE__);
    
    //    Allocate and target the escape island to the overriding function.
    BranchIsland    *escapeIsland = NULL;
    if( !err )
        err = allocateBranchIsland( &escapeIsland, kAllocateHigh, originalFunctionAddress );
    
    if (err) MachLogError("Failed to allocate branch island - err = %x %d\n", err, __LINE__);
    
    
#if defined(__ppc__) || defined(__POWERPC__)
    if( !err )
        err = setBranchIslandTarget( escapeIsland, overrideFunctionAddress, 0 );
    
    //    Build the branch absolute instruction to the escape island.
    long    branchAbsoluteInstruction = 0; // Set to 0 just to silence warning.
    if( !err ) {
        long escapeIslandAddress = ((long) escapeIsland) & 0x3FFFFFF;
        branchAbsoluteInstruction = 0x48000002 | escapeIslandAddress;
    }
#elif defined(__i386__) || defined(__x86_64__)
    
    if( !err )
        err = setBranchIslandTarget_i386( escapeIsland, overrideFunctionAddress, NULL, NULL, NULL );
    
    if (err) MachLogError("Failed to setup branch island - err = %x %d\n", err, __LINE__);

    // Build the jump relative instruction to the escape island
#endif
    
    
#if defined(__i386__) || defined(__x86_64__)
    if (!err) {
        uint32_t addressOffset = ((char*)escapeIsland - (char*)originalFunctionPtr - 5);
        addressOffset = OSSwapInt32(addressOffset);
        
        jumpRelativeInstruction |= 0xE900000000000000LL;
        jumpRelativeInstruction |= ((uint64_t)addressOffset & 0xffffffff) << 24;
        jumpRelativeInstruction = OSSwapInt64(jumpRelativeInstruction);
    }
#endif
    
    //    Optionally allocate & return the reentry island.
    BranchIsland    *reentryIsland = NULL;
    if( !err && originalFunctionReentryIsland ) {
        // If we are 64-bit + had RIP instructions, we need to allocate island close enough to the original function!
        int allocationType = kAllocateNormal;
#if defined(__x86_64__)
        if (patchData.nRipCnt > 0)
            allocationType = kAllocateHigh;
#endif
        err = allocateBranchIsland( &reentryIsland, allocationType, originalFunctionAddress);
        
        if (err) MachLogError("Failed to allocate re-entry island - err = %x %d\n", err, __LINE__);

        if( !err )
            *originalFunctionReentryIsland = reentryIsland;
    }
    
#if defined(__ppc__) || defined(__POWERPC__)
    //    Atomically:
    //    o If the reentry island was allocated:
    //        o Insert the original instruction into the reentry island.
    //        o Target the reentry island at the 2nd instruction of the
    //          original function.
    //    o Replace the original instruction with the branch absolute.
    if( !err ) {
        int escapeIslandEngaged = false;
        do {
            if( reentryIsland )
                err = setBranchIslandTarget( reentryIsland,
                                            (void*) (originalFunctionPtr+1), originalFunctionPtr, originalInstruction, &patchData );
            if( !err ) {
                escapeIslandEngaged = CompareAndSwap( originalInstruction,
                                                     branchAbsoluteInstruction,
                                                     (UInt32*)originalFunctionPtr );
                if( !escapeIslandEngaged ) {
                    //    Someone replaced the instruction out from under us,
                    //    re-read the instruction, make sure it's still not
                    //    'mfctr' and try again.
                    originalInstruction = *originalFunctionPtr;
                    if( (originalInstruction & kMFCTRMask) == kMFCTRInstruction)
                        err = err_cannot_override;
                }
            }
        } while( !err && !escapeIslandEngaged );
    }
#elif defined(__i386__) || defined(__x86_64__)
    // Atomically:
    //    o If the reentry island was allocated:
    //        o Insert the original instructions into the reentry island.
    //        o Target the reentry island at the first non-replaced
    //        instruction of the original function.
    //    o Replace the original first instructions with the jump relative.
    //
    // Note that on i386, we do not support someone else changing the code under our feet
    if ( !err ) {
        if( reentryIsland )
            err = setBranchIslandTarget_i386( reentryIsland,
                                             (void*) ((char *)originalFunctionPtr+eatenCount), originalFunctionAddress, originalInstructions, &patchData );
        
        if (err) MachLogError("Failed to setup re-entry island - err = %x %d\n", err, __LINE__);

        // try making islands executable before planting the jmp
#if defined(__x86_64__) || defined(__i386__)
        if( !err )
            err = makeIslandExecutable(escapeIsland);
        
        if (err) MachLogError("Failed to change branch island protection - err = %x %d\n", err, __LINE__);

        if( !err && reentryIsland )
            err = makeIslandExecutable(reentryIsland);
        
        if (err) MachLogError("Failed to change re-entry island protection - err = %x %d\n", err, __LINE__);
#endif
        if ( !err )
            atomic_mov64((uint64_t *)originalFunctionPtr, jumpRelativeInstruction);
    }
#endif
    
    //    Clean up on error.
    if( err ) {
        if( reentryIsland )
            freeBranchIsland( reentryIsland );
        if( escapeIsland )
            freeBranchIsland( escapeIsland );
    }
    
    return err;
}

// ================================Origin-specific changes==================================

// Reverse a previous method override.
mach_error_t mach_reverse_override_ptr(void* originalFunctionAddress, void* originalFunctionReentryIsland)
{
    // Only valid/tested for Intel processors
#if defined(__i386__) || defined(__x86_64__)
    
    if (!originalFunctionAddress || !originalFunctionReentryIsland)
        return err_invalid_inputs;
    
    // save last 3 bytes of first 64bits of codre we'll replace
    uint64_t currentFirst64BitsOfCode = *((uint64_t *)originalFunctionAddress);
    currentFirst64BitsOfCode = OSSwapInt64(currentFirst64BitsOfCode); // back to memory representation
    currentFirst64BitsOfCode &= 0x0000000000FFFFFFLL;
    
    // keep only last 3 instructions bytes, first 5 will be replaced back to original
    BranchIsland* island = (BranchIsland*)originalFunctionReentryIsland;
    uint64_t restoredInstructions = *((uint64_t*)(island->instructions + kInstructions));
    restoredInstructions = OSSwapInt64(restoredInstructions); // back to memory representation
    restoredInstructions &= 0xFFFFFFFFFF000000LL; // clear last 3 bytes
    restoredInstructions |= (currentFirst64BitsOfCode & 0x0000000000FFFFFFLL); // set last 3 bytes
    
    // Revert back original code
    restoredInstructions = OSSwapInt64(restoredInstructions);
    atomic_mov64((uint64_t *)originalFunctionAddress, restoredInstructions);
    
    return err_none;
    
#else
#error "Unsupported for non-Intel processors"
#endif
}

// ================================Origin-specific changes==================================

/*******************************************************************************
 *
 *    Implementation
 *
 *******************************************************************************/
#pragma mark    -
#pragma mark    (Implementation)

/***************************************************************************//**
                                                                              Implementation: Allocates memory for a branch island.
                                                                              
                                                                              @param    island            <-    The allocated island.
                                                                              @param    allocateHigh    ->    Whether to allocate the island at the end of the
                                                                              address space (for use with the branch absolute
                                                                              instruction).
                                                                              @result                    <-    mach_error_t
                                                                              
                                                                              ***************************************************************************/

mach_error_t
allocateBranchIsland(
                     BranchIsland    **island,
                     int                allocateHigh,
                     void *originalFunctionAddress)
{
    assert( island );
    
    mach_error_t    err = err_none;
    
#if defined(__x86_64__)
    if (true)
#else
    if( allocateHigh )
#endif
    {
        vm_size_t pageSize;
        err = host_page_size( mach_host_self(), &pageSize );
        if( !err ) {
            assert( sizeof( BranchIsland ) <= pageSize );
#if defined(__x86_64__)
            vm_address_t first = ((uint64_t)originalFunctionAddress & ~(uint64_t)(((uint64_t)1 << 31) - 1)) | ((uint64_t)1 << 31); // start in the middle of the page?
            vm_address_t last = 0x0;
#else
            vm_address_t first = 0xffc00000;
            vm_address_t last = 0xfffe0000;
#endif
            
            vm_address_t page = first;
            int allocated = 0;
            vm_map_t task_self = mach_task_self();
            
            while( !err && !allocated && page != last ) {
                
                err = vm_allocate( task_self, &page, pageSize, 0 );
                if( err == err_none )
                    allocated = 1;
                else if( err == KERN_NO_SPACE ) {
#if defined(__x86_64__)
                    page -= pageSize;
#else
                    page += pageSize;
#endif
                    err = err_none;
                }
            }
            if( allocated )
                *island = (BranchIsland*) page;
            else if( !allocated && !err )
                err = KERN_NO_SPACE;
        }
    } else {
        void *block = malloc( sizeof( BranchIsland ) );
        if( block )
            *island = (BranchIsland*)block;
        else
            err = KERN_NO_SPACE;
    }
    if( !err )
        (**island).allocatedHigh = allocateHigh;
    
    return err;
}

/***************************************************************************//**
                                                                              Implementation: Deallocates memory for a branch island.
                                                                              
                                                                              @param    island    ->    The island to deallocate.
                                                                              @result            <-    mach_error_t
                                                                              
                                                                              ***************************************************************************/

mach_error_t
freeBranchIsland(
                 BranchIsland    *island )
{
    assert( island );
    //assert( (*(long*)&island->instructions[0]) == kIslandTemplate[0] );
    assert( island->allocatedHigh );
    
    mach_error_t    err = err_none;
    
    if( island->allocatedHigh ) {
        vm_size_t pageSize;
        err = host_page_size( mach_host_self(), &pageSize );
        if( !err ) {
            assert( sizeof( BranchIsland ) <= pageSize );
            err = vm_deallocate(
                                mach_task_self(),
                                (vm_address_t) island, pageSize );
        }
    } else {
        free( island );
    }
    
    return err;
}

/***************************************************************************//**
                                                                              Implementation: Sets the branch island's target, with an optional
                                                                              instruction.
                                                                              
                                                                              @param    island        ->    The branch island to insert target into.
                                                                              @param    branchTo    ->    The address of the target.
                                                                              @param    instruction    ->    Optional instruction to execute prior to branch. Set
                                                                              to zero for nop.
                                                                              @result                <-    mach_error_t
                                                                              
                                                                              ***************************************************************************/
#if defined(__ppc__) || defined(__POWERPC__)
mach_error_t
setBranchIslandTarget(
                      BranchIsland    *island,
                      const void        *branchTo,
                      long            instruction )
{
    //    Copy over the template code.
    bcopy( kIslandTemplate, island->instructions, sizeof( kIslandTemplate ) );
    
    //    Fill in the address.
    ((short*)island->instructions)[kAddressLo] = ((long) branchTo) & 0x0000FFFF;
    ((short*)island->instructions)[kAddressHi]
    = (((long) branchTo) >> 16) & 0x0000FFFF;
    
    //    Fill in the (optional) instuction.
    if( instruction != 0 ) {
        ((short*)island->instructions)[kInstructionLo]
        = instruction & 0x0000FFFF;
        ((short*)island->instructions)[kInstructionHi]
        = (instruction >> 16) & 0x0000FFFF;
    }
    
    //MakeDataExecutable( island->instructions, sizeof( kIslandTemplate ) );
    msync( island->instructions, sizeof( kIslandTemplate ), MS_INVALIDATE );
    
    return err_none;
}
#endif

#if defined(__i386__)
mach_error_t
setBranchIslandTarget_i386(
                           BranchIsland*        island,
                           const void*          branchTo,
                           char*                originalCode,
                           char*                instructions,
                           MHOOKS_PATCHDATA*    patchData)
{
    
    //    Copy over the template code.
    bcopy( kIslandTemplate, island->instructions, sizeof( kIslandTemplate ) );
    
    // copy original instructions
    if (instructions) {
        bcopy (instructions, island->instructions + kInstructions, kOriginalInstructionsSize);
        if (patchData)
            FixupIPRelativeAddressing(island->instructions + kInstructions, originalCode, patchData);
    }
    
    // Fill in the address.
    int32_t addressOffset = (char *)branchTo - (island->instructions + kJumpAddress + 4);
    *((int32_t *)(island->instructions + kJumpAddress)) = addressOffset;
    
    msync( island->instructions, sizeof( kIslandTemplate ), MS_INVALIDATE );
    return err_none;
}

#elif defined(__x86_64__)
mach_error_t
setBranchIslandTarget_i386(
                           BranchIsland*        island,
                           const void*          branchTo,
                           char*                originalCode,
                           char*                instructions,
                           MHOOKS_PATCHDATA*    patchData)
{
    // Copy over the template code.
    bcopy( kIslandTemplate, island->instructions, sizeof( kIslandTemplate ) );
    
    // Copy original instructions.
    if (instructions) {
        bcopy (instructions, island->instructions, kOriginalInstructionsSize);
        if (patchData)
            FixupIPRelativeAddressing(island->instructions, originalCode, patchData);
    }
    
    //    Fill in the address.
    *((uint64_t *)(island->instructions + kJumpAddress)) = (uint64_t)branchTo;
    msync( island->instructions, sizeof( kIslandTemplate ), MS_INVALIDATE );
    
    return err_none;
}
#endif

#if defined(__i386__) || defined(__x86_64__)

#ifndef USE_DISASSEMBLER

// simplistic instruction matching
typedef struct {
    unsigned int length; // max 15
    unsigned char mask[15]; // sequence of bytes in memory order
    unsigned char constraint[15]; // sequence of bytes in memory order
}    AsmInstructionMatch;

#if defined(__i386__)
static AsmInstructionMatch possibleInstructions[] = {
    { 0x1, {0xFF}, {0x90} },                            // nop
    { 0x1, {0xFF}, {0x55} },                            // push %esp
    { 0x2, {0xFF, 0xFF}, {0x89, 0xE5} },                                // mov %esp,%ebp
    { 0x1, {0xFF}, {0x53} },                            // push %ebx
    { 0x3, {0xFF, 0xFF, 0x00}, {0x83, 0xEC, 0x00} },                            // sub 0x??, %esp
    { 0x6, {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00}, {0x81, 0xEC, 0x00, 0x00, 0x00, 0x00} },    // sub 0x??, %esp with 32bit immediate
    { 0x1, {0xFF}, {0x57} },                            // push %edi
    { 0x1, {0xFF}, {0x56} },                            // push %esi
    { 0x2, {0xFF, 0xFF}, {0x31, 0xC0} },                        // xor %eax, %eax
    { 0x3, {0xFF, 0x4F, 0x00}, {0x8B, 0x45, 0x00} },  // mov $imm(%ebp), %reg
    { 0x3, {0xFF, 0x4C, 0x00}, {0x8B, 0x40, 0x00} },  // mov $imm(%eax-%edx), %reg
    { 0x4, {0xFF, 0xFF, 0xFF, 0x00}, {0x8B, 0x4C, 0x24, 0x00} },  // mov $imm(%esp), %ecx
    
    // ================================Origin-specific changes==================================
    
    { 0x1, {0xF8}, {0x50} }, // push reg
    { 0x1, {0xF8}, {0x58} }, // pop reg
    { 0x2, {0xFF, 0x00}, {0xCD, 0x00} }, // int vec
    { 0x5, {0xF8, 0x00, 0x00, 0x00, 0x00}, {0xB8, 0x00, 0x00, 0x00, 0x00} }, // movl $imm, %reg (syscall -> movl 0x000c0030, %eax)
    { 0x5, {0xFF, 0x00, 0x00, 0x00, 0x00}, {0xE8, 0x00, 0x00, 0x00, 0x00} }, // call rel (shouldn't try to call original method though)
    
    // ================================Origin-specific changes==================================
    
    { 0x0 }
};
#elif defined(__x86_64__)
static AsmInstructionMatch possibleInstructions[] = {
    { 0x1, {0xFF}, {0x90} },                            // nop
    { 0x1, {0xF8}, {0x50} },                            // push %rX
    { 0x3, {0xFF, 0xFF, 0xFF}, {0x48, 0x89, 0xE5} },    // mov %rsp,%rbp
    { 0x3, {0xFF, 0xFF, 0xFF}, {0x48, 0x89, 0xF0} },    // mov %rsi, %rax
    { 0x4, {0xFF, 0xFF, 0xFF, 0x00}, {0x48, 0x83, 0xEC, 0x00} }, // sub 0x??, %rsp
    { 0x4, {0xFB, 0xFF, 0x00, 0x00}, {0x48, 0x89, 0x00, 0x00} }, // move onto rbp
    { 0x2, {0xFF, 0x00}, {0x41, 0x00} },                // push %rXX
    { 0x2, {0xFF, 0x00}, {0x85, 0x00} },                // test %rX,%rX
    { 0x5, {0xF8, 0x00, 0x00, 0x00, 0x00}, {0xB8, 0x00, 0x00, 0x00, 0x00} }, // mov $imm, %reg
    { 0x3, {0xFF, 0xFF, 0x00}, {0xFF, 0x77, 0x00} },    // pushq $imm(%rdi)
    
    // ================================Origin-specific changes==================================
    
    { 0x9, {0xFF, 0xFF, 0xFF, 0xC7, 0x3F, 0x00, 0x00, 0x00, 0x00}, {0x65, 0x48, 0x8B, 0x04, 0x25, 0x00, 0x00, 0x00, 0x00} }, // movq %gs:$imm32, %rX
    { 0x3, {0xFF, 0xFF, 0x00}, {0x48, 0x85, 0x00} },  // testq %rX, %rX
    
    // ================================Origin-specific changes==================================
    
    { 0x0 }
};
#endif

static Boolean codeMatchesInstruction(unsigned char *code, AsmInstructionMatch* instruction)
{
    Boolean match = true;
    
    size_t i;
    for (i=0; i<instruction->length; i++) {
        unsigned char mask = instruction->mask[i];
        unsigned char constraint = instruction->constraint[i];
        unsigned char codeValue = code[i];
        
        match = ((codeValue & mask) == constraint);
        if (!match) break;
    }
    
    return match;
}

#endif // !USE_DISASSEMBLER

#if defined(__i386__) || defined(__x86_64__)
static Boolean eatKnownInstructions(unsigned char* code, uint64_t* newInstruction, int* howManyEaten, char* originalInstructions, MHOOKS_PATCHDATA* patchData)
{
    Boolean allInstructionsKnown = true;
    int totalEaten = 0;
    unsigned char* ptr = code;
    int remainsToEat = 5; // a JMP instruction takes 5 bytes
    
    if (howManyEaten) *howManyEaten = 0;
    
    // ================================Origin-specific changes==================================
    
    MachLogInfo("ASM: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7] , ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14]);
    
    // ================================Origin-specific changes==================================
    

#ifndef USE_DISASSEMBLER // Use our limited tables of supported opcodes
    
#if defined(__i386__)
    int32_t instrOffset = 0;
    intptr_t addrValue = 0;
#endif
    
    while (remainsToEat > 0) {
        
        Boolean curInstructionKnown = false;
        
        // See if instruction matches one  we know
        AsmInstructionMatch* curInstr = possibleInstructions;
        do {
            if ((curInstructionKnown = codeMatchesInstruction(ptr, curInstr))) break;
            curInstr++;
        } while (curInstr->length > 0);
        
        // if all instruction matches failed, we don't know current instruction then, stop here
        if (!curInstructionKnown) {
            allInstructionsKnown = false;
            
            MachLogError("mach_override: some instructions unknown! Need to update mach_override.c\n");
            break;
        }
        
#if defined(__i386__)
        // Origin HACK: this is to help with overwritting methods that use a relative call 0x00000000 to compute the offset to some data (ie ptrace)
        if (curInstr->length == 5)
        {
            if (ptr[0] == 0xE8 && ptr[1] == 0x00 && ptr[2] == 0x00 && ptr[3] == 0x00 && ptr[4] == 0x00)
            {
                addrValue = (intptr_t)ptr + curInstr->length;
                instrOffset = totalEaten;
            }
        }
#endif
        // At this point, we've matched curInstr
        int eaten = curInstr->length;
        ptr += eaten;
        remainsToEat -= eaten;
        totalEaten += eaten;
    }
    
    if (howManyEaten) *howManyEaten = totalEaten;
    
    if (originalInstructions) {
        Boolean enoughSpaceForOriginalInstructions = (totalEaten < kOriginalInstructionsSize);
        
        if (enoughSpaceForOriginalInstructions) {
            memset(originalInstructions, 0x90 /* NOP */, kOriginalInstructionsSize); // fill instructions with NOP
            bcopy(code, originalInstructions, totalEaten);
            
            
        } else {
            // printf ("Not enough space in island to store original instructions. Adapt the island definition and kOriginalInstructionsSize\n");
            return false;
        }
    }
    
#else // Use disassembler from PC world
    
    // Grab the set of instructions we need to overwrite

    DWORD defaultInstSize = remainsToEat;
    
    // We want to grab/save as much of the original code as possible, as a 3rd party tool may hook on top of us, take more space than we do,
    // -> kaboom when we try to return to the original code from our hooked-in function (for example Fraps64 while hooking SwapChain->Present
    // (for example while playing Hardline)
    // Right now I'm using MHOOK_JUMPSIZE * 3 (15), but we could go up to MHOOKS_MAX_CODE_BYTES if necessary
    DWORD dwInstructionLength = DisassembleAndSkip(ptr, defaultInstSize, patchData);
    totalEaten += dwInstructionLength;
    
    if (howManyEaten) *howManyEaten = totalEaten;
    
    if (originalInstructions)
    {
        Boolean enoughSpaceForOriginalInstructions = (totalEaten < kOriginalInstructionsSize);
        
        if (enoughSpaceForOriginalInstructions)
        {
            memset(originalInstructions, 0x90 /* NOP */, kOriginalInstructionsSize); // fill instructions with NOP
            bcopy(code, originalInstructions, totalEaten);
        }
        
        else
        {
            MachLogError("Not enough space in island to store original instructions. Adapt the island definition and kOriginalInstructionsSize\n");
            return false;
        }
    }
    
#endif
    
    
    if (allInstructionsKnown) {
        // save last 3 bytes of first 64bits of codre we'll replace
        uint64_t currentFirst64BitsOfCode = *((uint64_t *)code);
        currentFirst64BitsOfCode = OSSwapInt64(currentFirst64BitsOfCode); // back to memory representation
        currentFirst64BitsOfCode &= 0x0000000000FFFFFFLL; 
        
        // keep only last 3 instructions bytes, first 5 will be replaced by JMP instr
        *newInstruction &= 0xFFFFFFFFFF000000LL; // clear last 3 bytes
        *newInstruction |= (currentFirst64BitsOfCode & 0x0000000000FFFFFFLL); // set last 3 bytes
    }
    
    return allInstructionsKnown;
}
#endif

#if defined(__i386__)
__asm(
      ".text;"
      ".align 2, 0x90;"
      "_atomic_mov64:;"
      "    pushl %ebp;"
      "    movl %esp, %ebp;"
      "    pushl %esi;"
      "    pushl %ebx;"
      "    pushl %ecx;"
      "    pushl %eax;"
      "    pushl %edx;"
      
      // atomic push of value to an address
      // we use cmpxchg8b, which compares content of an address with 
      // edx:eax. If they are equal, it atomically puts 64bit value 
      // ecx:ebx in address. 
      // We thus put contents of address in edx:eax to force ecx:ebx
      // in address
      "    mov        8(%ebp), %esi;"  // esi contains target address
      "    mov        12(%ebp), %ebx;"
      "    mov        16(%ebp), %ecx;" // ecx:ebx now contains value to put in target address
      "    mov        (%esi), %eax;"
      "    mov        4(%esi), %edx;"  // edx:eax now contains value currently contained in target address
      "    lock; cmpxchg8b    (%esi);" // atomic move.
      
      // restore registers
      "    popl %edx;"
      "    popl %eax;"
      "    popl %ecx;"
      "    popl %ebx;"
      "    popl %esi;"
      "    popl %ebp;"
      "    ret"
      );
#elif defined(__x86_64__)
void atomic_mov64(
                  uint64_t *targetAddress,
                  uint64_t value )
{
    *targetAddress = value;
}
#endif
#endif
