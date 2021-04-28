///////////////////////////////////////////////////////////////////////////////
// arm_unwind.cpp
//
// Copyright (c) 2011 Electronic Arts Inc.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Portions of this code come from the WIND library:
// 
// ARM Stack Unwinder, Michael.McTernan.2001@cs.bris.ac.uk
//
// This program is PUBLIC DOMAIN.
// This means that there is no copyright and anyone is able to take a copy
// for free and use it as they wish, with or without modifications, and in
// any context, commercially or otherwise. The only limitation is that I
// don't guarantee that the software is fit for any purpose or accept any
// liability for it's use or misuse - this software is without warranty.
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Key ARM information
//
// The ARM architecture reference manual:
//     http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0406b/index.html
//     http://infocenter.arm.com/help/topic/com.arm.doc.ihi0042d/IHI0042D_aapcs.pdf
//
// The ARM-THUMB Procedure Call Standard (I think this doc is obsolete, but still useful):
//     http://infocenter.arm.com/help/topic/com.arm.doc.espc0002/ATPCS.pdf
//     
// Other related links:
//     http://www.shervinemami.info/armAssembly.html
//     http://www.ethernut.de/en/documents/arm-inline-asm.html
//     http://cplusadd.blogspot.com/2008/11/frame-pointers-and-function-call.html
//     http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0041c/ch09s02s02.html
//     http://www.arm.com/products/processors/technologies/vector-floating-point.php
//     http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.qrc0001m/index.html
//     http://infocenter.arm.com/help/topic/com.arm.doc.ihi0042d/IHI0042D_aapcs.pdf
//
// Apple/iOS docs:
//     http://developer.apple.com/library/ios/#documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARMv6FunctionCallingConventions.html
//
// DWARF unwind information:
//     http://www.mikeash.com/pyblog/friday-qa-2012-04-27-plcrashreporter-and-unwinding-the-stack-with-dwarf.html
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Approaches to ARM stack unwinding
//
// Debugger information (e.g. DWARF).
//    - Works best but requires debug information that might not be available
//      at runtime. 
//    - Is rather complicated to implement, though open source versions exist.
// C++ exception handler information.
//    - May not be present or complete, as it's designed for exception unwinding.
//    - Is somewhat complicated to implement, though there is some obscure example code out there.
// Code markup 
//    - Manual markup is tedious and limited. Requires cooperation of all code authors.
//    - Somewhat automatic markup: GCC -finstrument-functions, http://gcc.activeventure.org/Code-Gen-Options.html
//      Downside is that only one entity in an application can use this.
//    - Slows down the application and increases its size.
// Read stack WORDs until one of them looks like code. 
//    - Error-prone.
// Execution simulation.
//    - Lookup function entry in .map file and find the function prologue.
//      The stack-moves-once rule allows you to undo the prologue.
//    - Simulate instructions till you hit an exit.
// Get the compiler to implement a consistent frame pointer scheme.
//    - Allows for fairly easy unwinding.
//    - Increases code size slightly and typically isn't available in optimized code.
//    - GCC -fno-omit-frame-pointer, http://gcc.gnu.org/onlinedocs/gcc-4.4.3/gcc/Optimize-Options.html
//      http://www.oschina.net/code/explore/linux-2.6.36/arch/arm/kernel/return_address.c
//      http://www.oschina.net/code/explore/linux-2.6.36/arch/arm/kernel/stacktrace.c
//    - Apple/iOS does this as part of the ABI, at least for debug builds, with r7 as a frame pointer:
//      http://developer.apple.com/library/ios/#documentation/Xcode/Conceptual/iPhoneOSABIReference/Articles/ARMv6FunctionCallingConventions.html
// Read instructions backward and find the function prologue (push { ... lr }) and reverse what it does.
//    - Not guaranteed to be reliable.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// The stack-moves-once condition
// Throughout the body of a routine that obeys the stack-moves-once condition, 
// the value of SP must be identical to the value of SP at the end of the entry 
// sequence. Strictly, this condition only needs to hold at points where control 
// might pass—whether via a chain of calls or via a run-time trap—to an unwinding 
// agent. 
// This standard allows any complying routine to pass control to an unwinding 
// agent so, in general, the condition must hold at all calls to routines that 
// are publicly visible or call publicly visible routines. In practice, we often 
// use the stronger form of the condition in which code in a routine body does 
// not alter SP.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// -- SP, the Stack Pointer -- 
// Register R13 (SP) is used as a pointer to the active stack.
// In Thumb code, most instructions cannot access SP. The only instructions 
// that can access SP are those designed to use SP as a stack pointer.
// The use of SP for any purpose other than as a stack pointer is deprecated
// and not likely to be found in code generated by modern compilers.
//
// -- LR, the Link Register --
// Register r14 (LR) is used to store the return address from a subroutine. 
// At other times, LR can be used for other purposes. When a BL or BLX  
// instruction performs a subroutine call, LR is set to the subroutine 
// return address. To perform a subroutine return, copy LR back to the 
// program counter. This is typically done in one of two ways, after  
// entering the subroutine with a BL or BLX instruction:
//     - Return with a BX LR instruction.
//     - On subroutine entry, store LR to the stack with an 
//       instruction of the form:
//          PUSH { <registers>, LR }
//       and use a matching instruction to return:
//          POP { <registers>, PC }
//
// -- PC, the Program Counter --
// Register R15 (PC) is the program counter:
//     - When executing an ARM instruction, PC reads as the address 
//       of the current instruction plus 8 bytes (two ARM instruction words).
//     - When executing a Thumb instruction, PC reads as the address 
//       of the current instruction plus 4 bytes (two Thumb instruction words).
// Writing an address to PC causes a branch to that address.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Register conventions
// 
// Use registers r0-r3 to pass parameter values into routines, and to pass 
// result values out. You can refer to r0-r3 as a1-a4 to make this usage 
// apparent. Between subroutine calls you can use r0-r3 for any purpose.
// A called routine does not have to restore r0-r3 before returning. A calling 
// routine must preserve the contents of r0-r3 if it needs them again.
//
// Use registers r4-r11 to hold the values of a routine's local variables. 
// You can refer to them as v1-v8 to make this usage apparent. In Thumb state, 
// in most instructions you can only use registers r4-r7 for local variables.
// A called routine must restore the values of these registers before returning, 
// if it has used them.
//
// Register r10 is alternatively called the Stack Limit register. When the compiler
// has stack limit checking enabled, this register is supposed to always hold the 
// stack limit currently in effect. This allows for checking against stack bounds.
//
// Register r12 is the intra-call scratch register, ip. It is used in this role 
// in procedure linkage veneers, for example interworking veneers. Between procedure 
// calls you can use it for any purpose. A called routine does not need to restore 
// r12 before returning.
//
// Register r13 is the stack pointer, sp. You must not use it for any other purpose.
// The value held in sp on exit from a called routine must be the same as it was on
// entry. Additionally, the function is not allowed to modify SP once it sets it
// for itself in the function prologue. (Question: how does alloca() C function work then?)
// 
// Register r14 is the link register, lr. If you save the return address, you 
// can use r14 for other purposes between calls. Upon function entry it is typically
// pushed onto the stack, and popped off the stack and into the PC opon function exit.
// 
// Register r15 is the program counter, pc. It cannot be used for any other purpose.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Portions of this code come from the WIND library, which has passed EA Legal
// approval for use in EA code, both internal and shipping.
// 
// ARM Stack Unwinder, Michael.McTernan.2001@cs.bris.ac.uk
//
// This program is PUBLIC DOMAIN.
// This means that there is no copyright and anyone is able to take a copy
// for free and use it as they wish, with or without modifications, and in
// any context, commercially or otherwise. The only limitation is that I
// don't guarantee that the software is fit for any purpose or accept any
// liability for it's use or misuse - this software is without warranty.
///////////////////////////////////////////////////////////////////////////////


#include "arm_unwind.h"
#include <EACallstack/EACallstack.h>
#include <EACallstack/EADasm.h>
#include <EAAssert/eaassert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void      UnwInvalidateRegisterFile(RegData* regFile, bool bPreserveFramePointer);
void      UnwInitState       (UnwState* state, const UnwindCallbacks* cb, void* rptData, uint32_t fp, uint32_t sp, uint32_t lr, uint32_t pc);
UnwResult UnwStartArm        (UnwState* state);
UnwResult UnwStartThumb      (UnwState* state);
Bool      UnwReportRetAddr   (UnwState* state, uint32_t addr);
Bool      UnwMemWriteRegister(UnwState* state, uint32_t addr, const RegData* reg);
Bool      UnwMemReadRegister (UnwState* state, uint32_t addr, RegData* reg);
bool      RandBool();



#if defined(UNW_DEBUG)
    #define UnwPrintd(pFormat, ...) state->cb->printf(pFormat, __VA_ARGS__)
#else
    #define UnwPrintd(pFormat, ...) (void)0
#endif



#if defined(UNW_DEBUG)
    static void UnwPrintARMInstr(UnwState* state, uint32_t sp, uint32_t pc, uint32_t* pInstr)
    {
        using namespace EA::Dasm;

        DisassemblerARM dARM;
        DasmData        dd;
        char            buffer[64];
        char            buffer2[512];

        dd.mOptionFlags = kOFHex | kOFHelp | kOFARM;
        dARM.Dasm(pInstr, pInstr + 1, dd, 0, pc);
        snprintf(buffer, sizeof(buffer), "%s %s", dd.mOperation, dd.mOperands); // Printed into its own buffer because we want it to have its own field size below.
        snprintf(buffer2, sizeof(buffer2), "ARM sp: %08x pc: %08x: %-12s %-36s | %s\n", sp, pc, dd.mInstruction, buffer, dd.mHelp);
        state->cb->printf("%s", buffer2);
    } 

    static void UnwPrintThumbInstr(UnwState* state, uint32_t sp, uint32_t pc, uint16_t* pInstr)
    {
        using namespace EA::Dasm;

        DisassemblerARM dARM;
        DasmData        dd;
        char            buffer[64];
        char            buffer2[512];

        dd.mOptionFlags = kOFHex | kOFHelp | kOFThumb;
        dARM.Dasm(pInstr, pInstr + 2, dd, 0, pc);
        snprintf(buffer, sizeof(buffer), "%s %s", dd.mOperation, dd.mOperands); // Printed into its own buffer because we want it to have its own field size below.
        snprintf(buffer2, sizeof(buffer2), "Thumb sp: %08x pc: %08x: %-12s %-36s | %s\n", sp, pc, dd.mInstruction, buffer, dd.mHelp);
        state->cb->printf("%s", buffer2);
    } 
#else
    #define UnwPrintARMInstr(a,b,c,d)   (void)0
    #define UnwPrintThumbInstr(a,b,c,d) (void)0
#endif


static inline bool ThumbInstructionIs32Bit(uint16_t in16)
{
    return ((in16 & 0xe000) == 0xe000) && ((in16 & 0x1800) != 0x0000);  // If the top three bits are set, and the next two are anything but zero... it's a 32 bit instruction.
}



#include <stdio.h>
#include <stdarg.h>
#include <string.h>

    // Printf wrapper.
    // This is used such that alternative outputs for any output can be selected
    // by modification of this wrapper function.
    int UnwPrintf(const char* format, ...)
    {
        va_list args;
        va_start(args, format);

        #if defined(EA_PLATFORM_ANDROID_DISABLED)
            // The android log writer insists on writing a newline for every sprintf call.
            char        bufferTemp[96];
            static char buffer[512];
            static int  bufferPos = 0;

            int count = EA::StdC::Vsnprintf(bufferTemp, sizeof(bufferTemp), format, args);

            strcpy(buffer + bufferPos, bufferTemp);
            bufferPos += count;

            for(int i = 0; i < bufferPos; ++i)
            {
                if((buffer[i] == '\n') || (i >= 256))
                {
                    buffer[i] = 0;
                    EA::StdC::Printf("%s", buffer);
                    memmove(buffer, buffer + (i + 1), (size_t)(bufferPos - (i + 1)));
                    bufferPos -= (i + 1);
                    i -= (i + 1);
                }
            }

            int result = count;
        #else
            int result = EA::StdC::Vprintf(format, args);
        #endif

        va_end(args);
        return result;
    }



void UnwInitializeRegisterFile(RegData* regFile)
{
    for(int i = 0; i < 16; i++)
    {
        regFile[i].v = 0x00000000;
        regFile[i].o = REG_VAL_INVALID;
    }
}


// Invalidate all general purpose registers.
// We want to avoid calling this function to the extent possible, as it results in 
// registers getting cleared that may have useful data in them.
void UnwInvalidateRegisterFile(RegData* regFile, bool bPreserveFramePointer)
{
    bool bThumb = (regFile[15].v & 0x01) != 0;

    for(int i = 0; i < 13; i++) // Don't invalidate r13 (sp), r14 (lr), r15 (pc).
    {
        if(bPreserveFramePointer)
        {
            if(bThumb && (i == 7)) // If Thumb code and i refers to the frame pointer register...
            {
                const uint32_t absDifference = (regFile[13].v > regFile[7].v) ? (regFile[13].v - regFile[7].v) : (regFile[7].v - regFile[13].v);

                if(absDifference < 16384)   // If the fp seems to be valid...
                    continue;               // Don't invalidate this register.
            }
            else if(!bThumb && (i == 11)) // If ARM code and i refers to the frame pointer register...
            {
                const uint32_t absDifference = (regFile[13].v > regFile[11].v) ? (regFile[11].v - regFile[7].v) : (regFile[7].v - regFile[11].v);

                if(absDifference < 16384)   // If the fp seems to be valid...
                    continue;               // Don't invalidate this register.
            }
        }

        regFile[i].o = REG_VAL_INVALID;
    }
}


// Initialise the data used for unwinding.
void UnwInitState(UnwState*              state,   // Pointer to structure to fill.
                  const UnwindCallbacks* cb,      // Callbacks.
                  void*                  rptData, // Data to pass to report function.
                  uint32_t fp, uint32_t sp, uint32_t lr, uint32_t pc) // register context at which to start unwinding.
{
    UnwInitializeRegisterFile(state->regData);

    // Store the pointer to the callbacks
    state->cb = cb;
    state->reportData = rptData;

    if(pc & 0x01) // If Thumb code... fp refers to r7, else it refers to r11.
    {
        state->regData[ 7].v = fp;
        state->regData[ 7].o = REG_VAL_FROM_CONST;
    }
    else
    {
        state->regData[11].v = fp;
        state->regData[11].o = REG_VAL_FROM_CONST;
    }

    state->regData[13].v = sp;
    state->regData[13].o = REG_VAL_FROM_CONST;

    state->regData[14].v = lr;
    state->regData[14].o = REG_VAL_FROM_CONST;

    state->regData[15].v = pc;
    state->regData[15].o = REG_VAL_FROM_CONST;

    // Invalidate all memory addresses
    memset(state->memData.used, 0, sizeof(state->memData.used));
}


// Call the report function to indicate some return address.
// This returns the value of the report function, which if TRUE
// indicates that unwinding may continue.
Bool UnwReportRetAddr(UnwState* state, uint32_t addr)
{
    // Cast away const from reportData.
    // The const is only to prevent the unw module modifying the data.
    return state->cb->report((void*)state->reportData, addr);
}


// Write a value from a register to memory.
// This will store some register and meta data onto the virtual stack.
//
//      \param state [in/out]  The unwinding state.
//      \param wAddr [in]  The address at which to write the data.
//      \param reg   [in]  The register to store.
//      \return TRUE if the write was successful, FALSE otherwise.
//
Bool UnwMemWriteRegister(UnwState* state, uint32_t addr, RegData* reg)
{
    return UnwMemHashWrite(&state->memData, addr, reg->v, M_IsOriginValid(reg->o));
}


// Read into a register from memory.
// This will read a register from memory, and setup the meta data.
// If the register has been previously written to memory using
// UnwMemWriteRegister, the local hash will be used to return the
// value while respecting whether the data was valid or not.  If the
// register was previously written and was invalid at that point,
// REG_VAL_INVALID will be returned in *reg.
//
// \param state [in]  The unwinding state.
// \param addr  [in]  The address to read.
// \param reg   [out] The result, containing the data value and the origin
//                     which will be REG_VAL_FROM_MEMORY, or REG_VAL_INVALID.
// \return TRUE if the address could be read and *reg has been filled in.
//         FALSE is the data could not be read.
//
Bool UnwMemReadRegister(UnwState* state, uint32_t addr, RegData* reg)
{
    Bool tracked;
    Bool returnValue;

    #if defined(UNW_DEBUG)
        EA::StdC::Printf("      UnwMemReadRegister addr: 0x%08", addr);
    #endif

    // Check if the value can be found in the hash
    if(UnwMemHashRead(&state->memData, addr, &reg->v, &tracked))
    {
        reg->o = tracked ? REG_VAL_FROM_MEMORY : REG_VAL_INVALID;
        returnValue = TRUE;

        #if defined(UNW_DEBUG)
            EA::StdC::Printf(" value: 0x%08x (from hash).", reg->v);
        #endif
    }
    // Not in the hash, so read from real memory
    else if(state->cb->readW(addr, &reg->v))
    {
        reg->o = REG_VAL_FROM_MEMORY;
        returnValue = TRUE;

        #if defined(UNW_DEBUG)
            EA::StdC::Printf(" value: 0x%08x (from real memory).", reg->v);
        #endif
    }
    else
    {
        // Not in the hash, and failed to read from memory
        #if defined(UNW_DEBUG)
            EA::StdC::Printf(". Read failure.");
        #endif
        returnValue = FALSE;
    }

    #if defined(UNW_DEBUG)
        EA::StdC::Printf("\n");
    #endif

    return returnValue;
}



// Check if some instruction is a data-processing instruction.
// Decodes the passed instruction, checks if it is a data-processing and
// verifies that the parameters and operation really indicate a data-
// processing instruction.  This is needed because some parts of the
// instruction space under this instruction can be extended or represent
// other operations such as MRS, MSR.
//
// \param[in] inst  The instruction word.
// \retval TRUE  Further decoding of the instruction indicates that this is
//                a valid data-processing instruction.
// \retval FALSE This is not a data-processing instruction,

static Bool isDataProc(uint32_t instr)
{
    uint8_t opcode = (instr & 0x01e00000) >> 21;
    Bool    S      = (instr & 0x00100000) ? TRUE : FALSE;

    if((instr & 0xfc000000) != 0xe0000000)
        return FALSE;
    else if(!S && (opcode >= 8) && (opcode <= 11))        
        return FALSE; // TST, TEQ, CMP and CMN all require S to be set
    else
        return TRUE;
}


UnwResult UnwStartArm(UnwState* state)
{
    Bool     found   = FALSE;
    uint32_t sbottom = (uint32_t) EA::Callstack::GetStackBottom();

    EA_COMPILETIME_ASSERT(sizeof(void*) == 4); // Currently we have only 32 bit support. As of this writing 64 bit ARM doesn't exist (though ARM is working on it).

    #if defined(UNW_DEBUG)
        EA::StdC::Printf("\nUnwStartArm begin loop\n");
    #endif

    state->ResetBranchStrategy();

    do
    {
        uint32_t instr;

        // [13] is the stack pointer, [15] is the program counter.
        if((sbottom != 0) && (state->regData[13].v >= sbottom)) // If the stack pointer has gone past the end (ARM callstacks grow downward)...
            return UNWIND_SUCCESS;

        // Attempt to read the instruction
        if(!state->cb->readW(state->regData[15].v, &instr))
            return UNWIND_IREAD_W_FAIL;

        UnwPrintARMInstr(state, state->regData[13].v, state->regData[15].v, &instr); // [13] is the stack pointer, [15] is the program counter.

        // Check that the PC is still on Arm alignment
        if(state->regData[15].v & 0x3)
        {
            UnwPrintd("\nError: PC misalignment. Returning UNWIND_INCONSISTENT, line %d.\n", __LINE__);
            return UNWIND_INCONSISTENT;
        }

        // Check that the SP and PC have not been invalidated
        if(!M_IsOriginValid(state->regData[13].o) || !M_IsOriginValid(state->regData[15].o))
        {
            UnwPrintd("\nError: PC or SP invalidated. Returning UNWIND_INCONSISTENT, line %d.\n", __LINE__);
            return UNWIND_INCONSISTENT;
        }

        // Branch and Exchange (BX)
        //  This is tested prior to data processing to prevent
        //  mis-interpretation as an invalid TEQ instruction.
        if((instr & 0xfffffff0) == 0xe12fff10)
        {
            uint8_t rn = instr & 0xf;

            UnwPrintd("BX r%d\t ; r%d (%08x %s)\n", rn, rn, state->regData[rn].v, M_Origin2Str(state->regData[rn].o));

            if(!M_IsOriginValid(state->regData[rn].o))
            {
                UnwPrintd("\nUnwind failure: BX to untracked register. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                return UNWIND_FAILURE;
            }

            // Set the new PC value
            state->regData[15].v = state->regData[rn].v;

            // Check if the return value is from the stack
            if(state->regData[rn].o == REG_VAL_FROM_STACK)
            {
                // Report the return address
                if(!UnwReportRetAddr(state, state->regData[rn].v))
                    return UNWIND_TRUNCATED; // The output stack address array ran out of entries.

                // Reset because we are now going to be starting anew reading the parent functions's instructions.
                state->ResetInstructionCount();
            }

            // Determine the return mode
            if(state->regData[rn].v & 0x1)
            {
                // Branching to THUMB
                return UnwStartThumb(state);
            }
            else
            {
                // Branch to ARM
                // Account for the auto-increment which isn't needed
                state->regData[15].v -= 4;
            }
        }

        // Branch
        else if((instr & 0xff000000) == 0xea000000)
        {
            int32_t offset = (instr & 0x00ffffff);

            // Shift value
            offset = offset << 2;

            // Sign extend if needed
            if(offset & 0x02000000)
                offset |= 0xfc000000;

            UnwPrintd("B %d\n", offset);

            // Adjust PC
            state->regData[15].v += offset;

            // Account for pre-fetch, where normally the PC is 8 bytes
            //  ahead of the instruction just executed.
            
            state->regData[15].v += 4;

        }

        // MRS
        else if((instr & 0xffbf0fff) == 0xe10f0000)
        {
            #if defined(UNW_DEBUG)
            Bool R = (instr & 0x00400000) ? TRUE : FALSE;
            #endif
            uint8_t    rd = (instr & 0x0000f000) >> 12;

            UnwPrintd("MRS r%d,%s\t; r%d invalidated", rd, R ? "SPSR" : "CPSR", rd);

            // Status registers untracked
            state->regData[rd].o = REG_VAL_INVALID;
        }
        // MSR
        else if((instr & 0xffb0f000) == 0xe120f000)
        {
            #if defined(UNW_DEBUG)
            Bool R  = (instr & 0x00400000) ? TRUE : FALSE;
            UnwPrintd("MSR %s_?, ???", R ? "SPSR" : "CPSR");
            #endif

            // Status registers untracked.
            //  Potentially this could change processor mode and switch
            //  banked registers r8-r14. Most likely is that r13 (sp) will
            //  be banked.  However, invalidating r13 will stop unwinding
            //  when potentially this write is being used to disable/enable
            //  interrupts (a common case). Therefore no invalidation is
            //  performed.
            
        }
        // Data processing
        else if(isDataProc(instr))
        {
            Bool      I        = (instr & 0x02000000) ? TRUE : FALSE;
            uint8_t   opcode   = (instr & 0x01e00000) >> 21;
            #if defined(UNW_DEBUG)
            Bool      S        = (instr & 0x00100000) ? TRUE : FALSE;
            #endif
            uint8_t   rn       = (instr & 0x000f0000) >> 16;
            uint8_t   rd       = (instr & 0x0000f000) >> 12;
            uint16_t  operand2 = (instr & 0x00000fff);
            uint32_t  op2val;
            int       op2origin;

            switch(opcode)
            {
                case  0: UnwPrintd("AND%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  1: UnwPrintd("EOR%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  2: UnwPrintd("SUB%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  3: UnwPrintd("RSB%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  4: UnwPrintd("ADD%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  5: UnwPrintd("ADC%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  6: UnwPrintd("SBC%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  7: UnwPrintd("RSC%s r%d,r%d,", S ? "S" : "", rd, rn); break;
                case  8: UnwPrintd("TST%s r%d,",     S ? "S" : "", rn);     break;
                case  9: UnwPrintd("TEQ%s r%d,",     S ? "S" : "", rn);     break;
                case 10: UnwPrintd("CMP%s r%d,",     S ? "S" : "", rn);     break;
                case 11: UnwPrintd("CMN%s r%d,",     S ? "S" : "", rn);     break;
                case 12: UnwPrintd("ORR%s r%d,",     S ? "S" : "", rn);     break;
                case 13: UnwPrintd("MOV%s r%d,",     S ? "S" : "", rd);     break;
                case 14: UnwPrintd("BIC%s r%d,r%d",  S ? "S" : "", rd, rn); break;
                case 15: UnwPrintd("MVN%s r%d,",     S ? "S" : "", rd);     break;
            }

            // Decode operand 2
            if(I)
            {
                uint8_t shiftDist  = (operand2 & 0x0f00) >> 8;
                uint8_t shiftConst = (operand2 & 0x00ff);

                // rotate const right by 2 * shiftDist
                shiftDist *= 2;
                op2val    = (shiftConst >> shiftDist) |
                            (shiftConst << (32 - shiftDist));
                op2origin = REG_VAL_FROM_CONST;

                UnwPrintd("#0x%08x", op2val);
            }
            else
            {
                // Register and shift
                uint8_t  rm        = (operand2 & 0x000f);
                uint8_t  regShift  = (operand2 & 0x0010) ? TRUE : FALSE;
                uint8_t  shiftType = (operand2 & 0x0060) >> 5;
                uint32_t shiftDist;
                #if defined(UNW_DEBUG)
                const char* const shiftMnu[4] = { "LSL", "LSR", "ASR", "ROR" };
                #endif
                UnwPrintd("r%d ", rm);

                // Get the shift distance
                if(regShift)
                {
                    uint8_t rs = (operand2 & 0x0f00) >> 8;

                    if(operand2 & 0x00800)
                    {
                        UnwPrintd("\nError: Bit should be zero. Returning UNWIND_ILLEGAL_INSTR, line %d.\n", __LINE__);
                        return UNWIND_ILLEGAL_INSTR;
                    }
                    else if(rs == 15)
                    {
                        UnwPrintd("\nError: Cannot use R15 with register shift. Returning UNWIND_ILLEGAL_INSTR, line %d.\n", __LINE__);
                        return UNWIND_ILLEGAL_INSTR;
                    }

                    // Get shift distance
                    shiftDist = state->regData[rs].v;
                    op2origin = state->regData[rs].o;

                    UnwPrintd("%s r%d\t; r%d (%08x %s), r%d (%08x %s)", shiftMnu[shiftType], rs, rm, state->regData[rm].v, M_Origin2Str(state->regData[rm].o), rs, state->regData[rs].v, M_Origin2Str(state->regData[rs].o));
                }
                else
                {
                    shiftDist = (operand2 & 0x0f80) >> 7;
                    op2origin = REG_VAL_FROM_CONST;

                    if(shiftDist)
                        UnwPrintd("%s #%d", shiftMnu[shiftType], shiftDist);

                    UnwPrintd("\t; r%d (%08x %s)", rm, state->regData[rm].v, M_Origin2Str(state->regData[rm].o));
                }

                // Apply the shift type to the source register
                switch(shiftType)
                {
                    case 0: // logical left
                        op2val = state->regData[rm].v << shiftDist;
                        break;

                    case 1: // logical right
                        if(!regShift && shiftDist == 0)
                            shiftDist = 32;
                        op2val = state->regData[rm].v >> shiftDist;
                        break;

                    case 2: // arithmetic right
                        if(!regShift && shiftDist == 0)
                            shiftDist = 32;

                        if(state->regData[rm].v & 0x80000000)
                        {
                            // Register shifts maybe greater than 32
                            if(shiftDist >= 32)
                                op2val = 0xffffffff;
                            else
                            {
                                op2val = state->regData[rm].v >> shiftDist;
                                op2val |= 0xffffffff << (32 - shiftDist);
                            }
                        }
                        else
                            op2val = state->regData[rm].v >> shiftDist;
                        break;

                    case 3: // rotate right
                        if(!regShift && shiftDist == 0)
                        {
                            // Rotate right with extend.
                            //  This uses the carry bit and so always has an
                            //  untracked result.
                            op2origin = REG_VAL_INVALID;
                            op2val    = 0;
                        }
                        else
                        {
                            // Limit shift distance to 0-31 incase of register shift
                            shiftDist &= 0x1f;

                            op2val = (state->regData[rm].v >> shiftDist) |
                                     (state->regData[rm].v << (32 - shiftDist));
                        }
                        break;

                    default:
                        UnwPrintd("\nError: Invalid shift type: %d. Returning UNWIND_FAILURE, line %d\n", shiftType, __LINE__);
                        return UNWIND_FAILURE;
                }

                // Decide the data origin
                if(M_IsOriginValid(op2origin) &&
                   M_IsOriginValid(state->regData[rm].o))
                {
                    op2origin = state->regData[rm].o;
                    op2origin |= REG_VAL_ARITHMETIC;
                }
                else
                    op2origin = REG_VAL_INVALID;
            }

            // Propagate register validity
            switch(opcode)
            {
                case  0: // AND: Rd := Op1 AND Op2
                case  1: // EOR: Rd := Op1 EOR Op2
                case  2: // SUB: Rd := Op1 - Op2
                case  3: // RSB: Rd := Op2 - Op1
                case  4: // ADD: Rd := Op1 + Op2
                case 12: // ORR: Rd := Op1 OR Op2
                case 14: // BIC: Rd := Op1 AND NOT Op2
                    if(!M_IsOriginValid(state->regData[rn].o) ||
                       !M_IsOriginValid(op2origin))
                    {
                        state->regData[rd].o = REG_VAL_INVALID;
                    }
                    else
                    {
                        state->regData[rd].o = state->regData[rn].o;
                        state->regData[rd].o |= op2origin;
                    }
                    break;
                case  5: // ADC: Rd:= Op1 + Op2 + C
                case  6: // SBC: Rd:= Op1 - Op2 + C
                case  7: // RSC: Rd:= Op2 - Op1 + C
                    // CPSR is not tracked
                    state->regData[rd].o = REG_VAL_INVALID;
                    break;

                case  8: // TST: set condition codes on Op1 AND Op2
                case  9: // TEQ: set condition codes on Op1 EOR Op2
                case 10: // CMP: set condition codes on Op1 - Op2
                case 11: // CMN: set condition codes on Op1 + Op2
                    break;


                case 13: // MOV: Rd:= Op2
                case 15: // MVN: Rd:= NOT Op2
                    state->regData[rd].o = op2origin;
                    break;
            }

            // Account for pre-fetch by temporarily adjusting PC
            if(rn == 15)
            {
                // If the shift amount is specified in the instruction,
                //  the PC will be 8 bytes ahead. If a register is used
                //  to specify the shift amount the PC will be 12 bytes
                //  ahead.
                
                if(!I && (operand2 & 0x0010))
                    state->regData[rn].v += 12;
                else
                    state->regData[rn].v += 8;
            }

            // Compute values
            switch(opcode)
            {
                case  0: // AND: Rd := Op1 AND Op2
                    state->regData[rd].v = state->regData[rn].v & op2val;
                    break;

                case  1: // EOR: Rd := Op1 EOR Op2
                    state->regData[rd].v = state->regData[rn].v ^ op2val;
                    break;

                case  2: // SUB: Rd:= Op1 - Op2
                    state->regData[rd].v = state->regData[rn].v - op2val;
                    break;
                case  3: // RSB: Rd:= Op2 - Op1
                    state->regData[rd].v = op2val - state->regData[rn].v;
                    break;

                case  4: // ADD: Rd:= Op1 + Op2
                    state->regData[rd].v = state->regData[rn].v + op2val;
                    break;

                case  5: // ADC: Rd:= Op1 + Op2 + C
                case  6: // SBC: Rd:= Op1 - Op2 + C
                case  7: // RSC: Rd:= Op2 - Op1 + C
                case  8: // TST: set condition codes on Op1 AND Op2
                case  9: // TEQ: set condition codes on Op1 EOR Op2
                case 10: // CMP: set condition codes on Op1 - Op2
                case 11: // CMN: set condition codes on Op1 + Op2
                    UnwPrintd("%s", "\t; ????");
                    break;

                case 12: // ORR: Rd:= Op1 OR Op2
                    state->regData[rd].v = state->regData[rn].v | op2val;
                    break;

                case 13: // MOV: Rd:= Op2
                    state->regData[rd].v = op2val;
                    break;

                case 14: // BIC: Rd:= Op1 AND NOT Op2
                    state->regData[rd].v = state->regData[rn].v & (~op2val);
                    break;

                case 15: // MVN: Rd:= NOT Op2
                    state->regData[rd].v = ~op2val;
                    break;
            }

            // Remove the prefetch offset from the PC
            if(rd != 15 && rn == 15)
            {
                if(!I && (operand2 & 0x0010))
                    state->regData[rn].v -= 12;
                else
                    state->regData[rn].v -= 8;
            }

        }

        // LDR Load Register (immediate)
        // STR Store Register (immediate)
        else if((instr & 0xfe0f0000) == 0xe40d0000)  // LDR pc,[sp],#0x14
        {
            Bool     P         = (instr & 0x01000000) ? TRUE : FALSE;
            Bool     U         = (instr & 0x00800000) ? TRUE : FALSE;
            Bool     W         = (instr & 0x00200000) ? TRUE : FALSE;
            Bool     L         = (instr & 0x00100000) ? TRUE : FALSE;
            uint16_t baseReg   = (instr & 0x000f0000) >> 16;
            uint16_t destReg   = (instr & 0x0000f000) >> 12;
            uint32_t imm12     = (instr & 0x00000fff);
            bool     index     = (P == TRUE);
            bool     add       = (U == TRUE);
            bool     wback     = ((P == FALSE) || (W == TRUE));

            if(L) // if LDR (as opposed to STR)...
            {
                if(wback && (baseReg == destReg))
                {
                    UnwPrintd("\nError: Invalid LDR usage. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                    return UNWIND_FAILURE;
                }
                else
                {
                    uint32_t base_addr   = state->regData[baseReg].v;
                    uint32_t offset_addr = add ? base_addr + imm12 : base_addr - imm12;
                    uint32_t addr        = index ? offset_addr : base_addr;

                    if(wback)
                        state->regData[baseReg].v = offset_addr;

                    if(destReg == 15) // if it's PC
                    {
                        if(!UnwMemReadRegister(state, addr, &state->regData[15]))
                        {
                            UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                            return UNWIND_DREAD_W_FAIL;
                        }

                        if(!M_IsOriginValid(state->regData[15].o))
                        {
                            // Return address is not valid
                            UnwPrintd("PC popped with invalid address. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                            return UNWIND_FAILURE;
                        }
                        else
                        {
                            // Store the return address
                            if(!UnwReportRetAddr(state, state->regData[15].v))
                                return UNWIND_TRUNCATED; // The output stack address array ran out of entries.

                            if(!state->cb->validatePC(state->regData[15].v))
                            {
                                UnwPrintd("  (appears to be invalid). Returning UNWIND_FAILURE, line %d\n", state->regData[15].v, __LINE__);
                                return UNWIND_FAILURE;
                            }

                            // Reset because we are now going to be starting anew reading the parent functions's instructions.
                            state->ResetInstructionCount();

                            // Determine the return mode
                            if(state->regData[15].v & 0x1)
                            {
                                // Branching to THUMB
                                return UnwStartThumb(state);
                            }
                            else
                            {
                                // Branch to ARM

                                // Account for the auto-increment which isn't needed
                                state->regData[15].v -= 4;
                            }
                        }
                    }
                }
            }
            else // else STR
            {
            }
        }

        // Vector Store or Load Multiple
        // FLDM/VLDM  FSTM/VSTM (the V version is the newer version of the F version)
        // Encoding T1 / A1 VFPv2, VFPv3, Advanced SIMD
        else if((instr & 0xfe000b00) == 0xec000b00)
        {
            Bool     P         = (instr & 0x01000000) ? TRUE : FALSE;
            Bool     U         = (instr & 0x00800000) ? TRUE : FALSE;
          //Bool     D         = (instr & 0x00400000) ? TRUE : FALSE;
            Bool     W         = (instr & 0x00200000) ? TRUE : FALSE;
            Bool     L         = (instr & 0x00100000) ? TRUE : FALSE;
            uint16_t baseReg   = (instr & 0x000f0000) >> 16;
            uint32_t Vd        = (instr & 0x0000f000) >> 12; // the starting floating point or vector register number
            uint32_t imm8      = (instr & 0x000000ff);
            uint32_t regs      = imm8 / 2; // number of vector registers starting from Vd
            uint32_t addr      = state->regData[baseReg].v;
            uint32_t regLength = 8;
          //Bool     addrValid = M_IsOriginValid(state->regData[baseReg].o);

            // currenlty we only handle load (pop) instruction cuz it modifies the SP
            if(L) // Load
            {
                if(!P && !U && !W)
                {
                   // instructions for 64-bit transfers between ARM core (regular) and extension (vector) registers 
                }
                else if(P && !W)
                {
                    // VLDR instruction
                }
                else if(P == U && W) // behavior undefined
                {
                    UnwPrintd("\nInvalid vector instruction. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                    return UNWIND_FAILURE;
                }
                else if(baseReg == 15 && W)
                {
                    UnwPrintd("\nInvalid vector instruction. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                    return UNWIND_FAILURE;
                }
                else if(regs == 0 || regs > 16 || (Vd + regs) > 32)
                {
                    UnwPrintd("\nInvalid vector instruction. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                    return UNWIND_FAILURE;
                }
                else if(imm8 % 2 != 0) // FLDMX instruction
                {
                    //Use of FLDMX and FSTMX is deprecated from ARMv6, except for disassembly purposes, and reassembly of disassembled code.
                    UnwPrintd("\nInvalid FLDMX/FSTMX. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                    return UNWIND_FAILURE;
                }
                else if(!P && U && W && baseReg == 13) // FLDMIAD/VLDMIAD or VPOP PUW = 011 (IA with !) IA = Increment After. we only care when Write-Back bit is set cuz it modifies the SP
                    state->regData[baseReg].v = addr + regLength * regs;
            }
        }

        // Block Data Transfer
        //  LDM, STM
        else if((instr & 0xfe000000) == 0xe8000000)
        {
            Bool     P         = (instr & 0x01000000) ? TRUE : FALSE;
            Bool     U         = (instr & 0x00800000) ? TRUE : FALSE;
            Bool     S         = (instr & 0x00400000) ? TRUE : FALSE;
            Bool     W         = (instr & 0x00200000) ? TRUE : FALSE;
            Bool     L         = (instr & 0x00100000) ? TRUE : FALSE;
            uint16_t baseReg   = (instr & 0x000f0000) >> 16;
            uint16_t regList   = (instr & 0x0000ffff);
            uint32_t addr      = state->regData[baseReg].v;
            Bool     addrValid = M_IsOriginValid(state->regData[baseReg].o);
            int8_t   r;

            #if defined(UNW_DEBUG)
                bool bPushPop = (baseReg == 13);

                // Display the instruction
                if(L)
                {
                    UnwPrintd("%s%c%c r%d%s, {reglist}%s\n",
                               bPushPop ? "POP" : "LDM",
                               P ? 'E' : 'F',
                               U ? 'D' : 'A',
                               baseReg,
                               W ? "!" : "",
                               S ? "^" : "");
                }
                else
                {
                    UnwPrintd("%s%c%c r%d%s, {reglist}%s\n",
                               bPushPop ? "PUSH" : "STM",
                               !P ? 'E' : 'F',
                               !U ? 'D' : 'A',
                               baseReg,
                               W ? "!" : "",
                               S ? "^" : "");
                }
            #endif

            // S indicates that banked registers (untracked) are used, unless
            //  this is a load including the PC when the S-bit indicates that
            //  that CPSR is loaded from SPSR (also untracked, but ignored).
            
            if(S && (!L || (regList & (0x01 << 15)) == 0))
            {
                UnwPrintd("\nError:S-bit set requiring banked registers. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                return UNWIND_FAILURE;
            }
            else if(baseReg == 15)
            {
                UnwPrintd("\nError: r15 used as base register. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                return UNWIND_FAILURE;
            }
            else if(regList == 0)
            {
                UnwPrintd("\nError: Register list empty. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                return UNWIND_FAILURE;
            }

            // Check if ascending or descending.
            //  Registers are loaded/stored in order of address.
            //  i.e. r0 is at the lowest address, r15 at the highest.
            r = U ? 0 : 15;

            do
            {
                // Check if the register is to be transferred
                if(regList & (0x01 << r))
                {
                    if(P) 
                        addr += U ? 4 : -4;

                    if(L) // If pop
                    {
                        if(addrValid)
                        {
                            if(!UnwMemReadRegister(state, addr, &state->regData[r]))
                            {
                                UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                                return UNWIND_DREAD_W_FAIL;
                            }

                            // Update the origin if read via the stack pointer
                            if(M_IsOriginValid(state->regData[r].o) && baseReg == 13)
                                state->regData[r].o = REG_VAL_FROM_STACK;

                            UnwPrintd(" R%d = 0x%08x\t; r%d (%08x %s)\n", r, state->regData[r].v, r, state->regData[r].v, M_Origin2Str(state->regData[r].o));
                        }
                        else
                        {
                            // Invalidate the register as the base reg was invalid
                            state->regData[r].o = REG_VAL_INVALID;

                            UnwPrintd(" R%d = ???\n", r);
                        }
                    }
                    else // Else push
                    {
                        if(r == 14) // If the code is pushing the LR register...
                        {
                            // We have a problem because it looks like we have fallen into prolgue code for another function.
                            // During stack unwinding we should jsut about never run into the case of executing a function prologue.
                            // To do: Back out of this unwind attempt.
                            UnwPrintd("\nFell into a function prologue. Returning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                            return UNWIND_DWRITE_W_FAIL;
                        }

                        if(addrValid)
                        {
                            // [13] is the stack pointer.
                            if(!UnwMemWriteRegister(state, state->regData[13].v, &state->regData[r]))
                            {
                                UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                                return UNWIND_DWRITE_W_FAIL;
                            }
                        }

                        UnwPrintd(" R%d = 0x%08x\n", r);
                    }

                    if(!P) 
                        addr += U ? 4 : -4;
                }

                // Check the next register
                r += U ? 1 : -1;
            }
            while((r >= 0) && (r <= 15));

            // Check the writeback bit
            if(W) 
                state->regData[baseReg].v = addr;

            // Check if the PC was loaded
            if(L && (regList & (0x01 << 15))) // If there was a pop into the PC...
            {
                if(!M_IsOriginValid(state->regData[15].o))
                {
                    UnwPrintd("PC popped with invalid address. Returning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                    return UNWIND_FAILURE;
                }
                else
                {
                    // Store the return address
                    if(!UnwReportRetAddr(state, state->regData[15].v))
                        return UNWIND_TRUNCATED; // The output stack address array ran out of entries.

                    if(!state->cb->validatePC(state->regData[15].v))
                    {
                        UnwPrintd("  (appears to be invalid). Returning UNWIND_FAILURE, line %d.\n", state->regData[15].v, __LINE__);
                        return UNWIND_FAILURE;
                    }

                    // Reset because we are now going to be starting anew reading the parent functions's instructions.
                    state->ResetInstructionCount();

                    if(state->regData[15].v & 0x1)      // If the destination is Thumb code...
                        return UnwStartThumb(state);    // Branching to THUMB
                    else
                        state->regData[15].v -= 4;      // Branch to ARM;  Account for the auto-increment which isn't needed
                }
            }
        }
        else
        {
            UnwPrintd("Skipped or unknown instruction: %08x:", instr);

            // Unknown/undecoded.  May alter some register, so invalidate file.
            // To do: handle this instruction rather than just invalidate the register file.
            //        We probably don't need to simulate the instruction; just need to know 
            //        what register or memory it might write to and invalidate it.
            UnwInvalidateRegisterFile(state->regData, true); // true=preserve frame pointer. Chances are very slim that whatever instruction this was, it didn't modify the frame pointer, at least if the frame pointer is being used as such.
        }

        UnwPrintd("%s", "\n");

        // Should never hit the reset vector
        if(state->regData[15].v == 0) 
        {
            UnwPrintd("Returning UNWIND_RESET, line %d\n", __LINE__);
            return UNWIND_RESET;
        }

        // Check next address
        state->regData[15].v += 4;

        // Garbage collect the memory hash (used only for the stack)
        UnwMemHashGC(state);

        if(++state->instructionCount == UNW_MAX_INSTR_COUNT) 
        {
            UnwPrintd("Returning UNWIND_EXHAUSTED, line %d\n", __LINE__);
            return UNWIND_EXHAUSTED;
        }
    }
    while(!found);

    UnwPrintd("Returning UNWIND_SUCCESS, line %d\n", __LINE__);
    return UNWIND_SUCCESS;
}



// Sign extend an 11 bit value.
// This function simply inspects bit 11 of the input value, and if
// set, the top 5 bits are set to give a 2's compliment signed value.
//     value   The value to sign extend.
//     return The signed-11 bit value stored in a 16bit data type.
static int16_t SignExtend11(uint16_t value)
{
    if(value & 0x0400)
       value |= 0xf800;

    return value;
}

static int16_t SignExtend8(uint16_t value)
{
    if(value & 0x0080)
       value |= 0xff00;

    return value;
}


UnwResult UnwStartThumb(UnwState* state)
{
    Bool     found   = FALSE;
    uint32_t sbottom = (uint32_t)EA::Callstack::GetStackBottom(); 

    EA_COMPILETIME_ASSERT(sizeof(void*) == 4);  // Currently we have only 32 bit support. As of this writing 64 bit ARM doesn't exist (though ARM is working on it).

    #if defined(UNW_DEBUG)
        UnwPrintd("%s", "\nUnwStartThumb begin loop\n");
    #endif

    state->ResetBranchStrategy();

    do
    {
        uint16_t instr[1]; // Some Thumb-2 instructions are 32 bit (16 bit pair).
        uint32_t currentInstrAddr = state->regData[15].v;
        uint32_t currentInstrBranchDest = 0; // This may get altered below if the current instruction is a branch.

        // [13] is the stack pointer, [15] is the program counter.
        if((sbottom != 0) && (state->regData[13].v >= sbottom))  // If the stack pointer has gone past the end (ARM callstacks grow downward)...
            return UNWIND_SUCCESS;

        // Attempt to read the instruction
        if(!state->cb->readH(state->regData[15].v & (~0x1), &instr[0]))
        {
            UnwPrintd("Returning UNWIND_IREAD_H_FAIL, line %d\n", __LINE__);
            return UNWIND_IREAD_H_FAIL;
        }

        if(!state->cb->readH((state->regData[15].v & (~0x1)) + 2, &instr[1]))
        {
            UnwPrintd("Returning UNWIND_IREAD_H_FAIL, line %d\n", __LINE__);
            return UNWIND_IREAD_H_FAIL;
        }

        UnwPrintThumbInstr(state, state->regData[13].v, state->regData[15].v, instr);

        // Check that the PC is still on Thumb alignment.
        if(!(state->regData[15].v & 0x1))
        {
            UnwPrintd("\nError: PC misalignment. Returning UNWIND_INCONSISTENT, line %d.\n", __LINE__);
            return UNWIND_INCONSISTENT;
        }

        // Check that the SP and PC have not been invalidated.
        if(!M_IsOriginValid(state->regData[13].o) || !M_IsOriginValid(state->regData[15].o))
        {
            UnwPrintd("\nError: PC or SP invalidated. Returning UNWIND_INCONSISTENT, line %d.\n", __LINE__);
            return UNWIND_INCONSISTENT;
        }

        // Format 1: Move shifted register
        //   LSL Rd, Rs, #Offset5
        //   LSR Rd, Rs, #Offset5
        //   ASR Rd, Rs, #Offset5
        if((instr[0] & 0xe000) == 0x0000 && (instr[0] & 0x1800) != 0x1800)
        {
            Bool signExtend;
            uint8_t    op      = (instr[0] & 0x1800) >> 11;
            uint8_t    offset5 = (instr[0] & 0x07c0) >>  6;
            uint8_t    rs      = (instr[0] & 0x0038) >>  3;
            uint8_t    rd      = (instr[0] & 0x0007);

            switch(op)
            {
                case 0: // LSL
                    UnwPrintd("LSL r%d, r%d, #%d\t; r%d (%08x %s)", rd, rs, offset5, rs, state->regData[rs].v, M_Origin2Str(state->regData[rs].o));
                    state->regData[rd].v = state->regData[rs].v << offset5;
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;

                case 1: // LSR
                    UnwPrintd("LSR r%d, r%d, #%d\t; r%d (%08x %s)", rd, rs, offset5, rs, state->regData[rs].v, M_Origin2Str(state->regData[rs].o));
                    state->regData[rd].v = state->regData[rs].v >> offset5;
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;

                case 2: // ASR
                    UnwPrintd("ASL r%d, r%d, #%d\t; r%d (%08x %s)", rd, rs, offset5, rs, state->regData[rs].v, M_Origin2Str(state->regData[rs].o));

                    signExtend = (state->regData[rs].v & 0x8000) ? TRUE : FALSE;
                    state->regData[rd].v = state->regData[rs].v >> offset5;
                    if(signExtend)
                        state->regData[rd].v |= 0xffffffff << (32 - offset5);
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;
            }
        }

        // Format 2: add/subtract
        //   ADD Rd, Rs, Rn
        //   ADD Rd, Rs, #Offset3
        //   SUB Rd, Rs, Rn
        //   SUB Rd, Rs, #Offset3
        else if((instr[0] & 0xf800) == 0x1800)
        {
            Bool    I  = (instr[0] & 0x0400) ? TRUE : FALSE;   // Direct (constant) or indirect (through register). 
            Bool    op = (instr[0] & 0x0200) ? TRUE : FALSE;   // Subraction or addition.
            uint8_t rn = (instr[0] & 0x01c0) >> 6;             // Register arg
            uint8_t rs = (instr[0] & 0x0038) >> 3;             // Register source
            uint8_t rd = (instr[0] & 0x0007);                  // Register destination

            // Print decoding
            UnwPrintd("%s r%d, r%d, %c%d", op ? "SUB" : "ADD", rd, rs, I ? '#' : 'r', rn);
            UnwPrintd("\t;r%d (%08x %s), r%d (%08x %s)", rd, state->regData[rd].v, M_Origin2Str(state->regData[rd].o), rs, state->regData[rs].v, M_Origin2Str(state->regData[rs].o));
            if(!I)
            {
                UnwPrintd(", r%d (%08x %s)", rn, state->regData[rn].v, M_Origin2Str(state->regData[rn].o));

                // Perform calculation
                if(op)
                    state->regData[rd].v = state->regData[rs].v - state->regData[rn].v;
                else
                    state->regData[rd].v = state->regData[rs].v + state->regData[rn].v;

                // Propagate the origin
                if(M_IsOriginValid(state->regData[rs].v) &&
                   M_IsOriginValid(state->regData[rn].v))
                {
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                }
                else
                    state->regData[rd].o = REG_VAL_INVALID;
            }
            else
            {
                // Perform calculation
                if(op)
                    state->regData[rd].v = state->regData[rs].v - rn;
                else
                    state->regData[rd].v = state->regData[rs].v + rn;

                // Propagate the origin
                state->regData[rd].o = state->regData[rs].o;
                state->regData[rd].o |= REG_VAL_ARITHMETIC;
            }
        }

        // Format 3: move/compare/add/subtract immediate
        //  MOV Rd, #Offset8
        //  CMP Rd, #Offset8
        //  ADD Rd, #Offset8
        //  SUB Rd, #Offset8
        else if((instr[0] & 0xe000) == 0x2000)
        {
            uint8_t op      = (instr[0] & 0x1800) >> 11;
            uint8_t rd      = (instr[0] & 0x0700) >>  8;
            uint8_t offset8 = (instr[0] & 0x00ff);

            switch(op)
            {
                case 0: // MOV
                    UnwPrintd("MOV r%d, #0x%02x", rd, offset8);
                    state->regData[rd].v = offset8;
                    state->regData[rd].o = REG_VAL_FROM_CONST;
                    break;

                case 1: // CMP
                    // Irrelevant to unwinding
                    UnwPrintd("%s", "CMP ???");
                    break;

                case 2: // ADD
                    UnwPrintd("ADD r%d, #0x%02x\t; r%d (%08x %s)", rd, offset8, rd, state->regData[rd].v, M_Origin2Str(state->regData[rd].o));
                    state->regData[rd].v += offset8;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;

                case 3: // SUB
                    UnwPrintd("SUB r%d, #0x%02x\t; r%d (%08x %s)", rd, offset8, rd, state->regData[rd].v, M_Origin2Str(state->regData[rd].o));
                    state->regData[rd].v += offset8;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;
            }
        }

        // Format 4: ALU operations
        //  AND Rd, Rs
        //  EOR Rd, Rs
        //  LSL Rd, Rs
        //  LSR Rd, Rs
        //  ASR Rd, Rs
        //  ADC Rd, Rs
        //  SBC Rd, Rs
        //  ROR Rd, Rs
        //  TST Rd, Rs
        //  NEG Rd, Rs
        //  CMP Rd, Rs
        //  CMN Rd, Rs
        //  ORR Rd, Rs
        //  MUL Rd, Rs
        //  BIC Rd, Rs
        //  MVN Rd, Rs
        else if((instr[0] & 0xfc00) == 0x4000)
        {
            uint8_t op = (instr[0] & 0x03c0) >> 6;
            uint8_t rs = (instr[0] & 0x0038) >> 3;
            uint8_t rd = (instr[0] & 0x0007);

            #if defined(UNW_DEBUG)
              static const char* const mnu[16] =
              {
                "AND", "EOR", "LSL", "LSR",
                "ASR", "ADC", "SBC", "ROR",
                "TST", "NEG", "CMP", "CMN",
                "ORR", "MUL", "BIC", "MVN"
              };
            #endif

            // Print the mnemonic and registers
            switch(op)
            {
                case  0: // AND
                case  1: // EOR
                case  2: // LSL
                case  3: // LSR
                case  4: // ASR
                case  7: // ROR
                case  9: // NEG
                case 12: // ORR
                case 13: // MUL
                case 15: // MVN
                    UnwPrintd("%s r%d ,r%d\t; r%d (%08x %s), r%d (%08x %s)", mnu[op], rd, rs, rd, state->regData[rd].v, M_Origin2Str(state->regData[rd].o), rs, state->regData[rs].v, M_Origin2Str(state->regData[rs].o));
                    break;

                case 5: // ADC
                case 6: // SBC
                    UnwPrintd("%s r%d, r%d", mnu[op], rd, rs);
                    break;

                case  8: // TST
                case 10: // CMP
                case 11: // CMN
                    // Irrelevant to unwinding
                    UnwPrintd("%s ???", mnu[op]);
                    break;

                case 14: // BIC
                    UnwPrintd("r%d ,r%d\t; r%d (%08x %s)", rd, rs, rs, state->regData[rs].v, M_Origin2Str(state->regData[rs].o));
                    state->regData[rd].v &= !state->regData[rs].v;
                    break;
            }

            // Perform operation
            switch(op)
            {
                case 0: // AND
                    state->regData[rd].v &= state->regData[rs].v;
                    break;

                case 1: // EOR
                    state->regData[rd].v ^= state->regData[rs].v;
                    break;

                case 2: // LSL
                    state->regData[rd].v <<= state->regData[rs].v;
                    break;

                case 3: // LSR
                    state->regData[rd].v >>= state->regData[rs].v;
                    break;

                case 4: // ASR
                    if(state->regData[rd].v & 0x80000000)
                    {
                        state->regData[rd].v >>= state->regData[rs].v;
                        state->regData[rd].v |= 0xffffffff << (32 - state->regData[rs].v);
                    }
                    else
                        state->regData[rd].v >>= state->regData[rs].v;

                    break;

                case  5: // ADC
                case  6: // SBC
                case  8: // TST
                case 10: // CMP
                case 11: // CMN
                    break;

                case 7: // ROR
                    state->regData[rd].v = (state->regData[rd].v >>       state->regData[rs].v) |
                                           (state->regData[rd].v << (32 - state->regData[rs].v));
                    break;

                case 9: // NEG
                    state->regData[rd].v = -state->regData[rs].v;
                    break;

                case 12: // ORR
                    state->regData[rd].v |= state->regData[rs].v;
                    break;

                case 13: // MUL
                    state->regData[rd].v *= state->regData[rs].v;
                    break;

                case 14: // BIC
                    state->regData[rd].v &= !state->regData[rs].v;
                    break;

                case 15: // MVN
                    state->regData[rd].v = !state->regData[rs].v;
                    break;
            }

            // Propagate data origins
            switch(op)
            {
                case  0: // AND
                case  1: // EOR
                case  2: // LSL
                case  3: // LSR
                case  4: // ASR
                case  7: // ROR
                case 12: // ORR
                case 13: // MUL
                case 14: // BIC
                    if(M_IsOriginValid(state->regData[rs].o) && M_IsOriginValid(state->regData[rs].o))
                    {
                        state->regData[rd].o = state->regData[rs].o;
                        state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    }
                    else
                        state->regData[rd].o = REG_VAL_INVALID;
                    break;

                case  5: // ADC
                case  6: // SBC
                    // C-bit not tracked
                    state->regData[rd].o = REG_VAL_INVALID;
                    break;

                case  8: // TST
                case 10: // CMP
                case 11: // CMN
                    // Nothing propagated
                    break;

                case  9: // NEG
                case 15: // MVN
                    state->regData[rd].o = state->regData[rs].o;
                    state->regData[rd].o |= REG_VAL_ARITHMETIC;
                    break;
            }
        }

        // Format 5: Hi register operations/branch exchange
        //   ADD Rd, Rs
        //   ADD Rd, Hs
        //   ADD Hd, Rs
        //   ADD Hd, Hs
        //   CMP ...
        //   MOV ...
        //   BX/BLX
        else if((instr[0] & 0xfc00) == 0x4400)
        {
            if((instr[0] >> 7) != 0x008f) // have to avoid BLX
            {
                uint8_t op  = (instr[0] & 0x0300) >> 8;
                Bool h1     = (instr[0] & 0x0080) ? TRUE: FALSE;
                Bool h2     = (instr[0] & 0x0040) ? TRUE: FALSE;
                uint8_t rhs = (instr[0] & 0x0038) >> 3;
                uint8_t rhd = (instr[0] & 0x0007);

                // Adjust the register numbers
                if(h2) 
                    rhs += 8;
                if(h1)
                    rhd += 8;

                switch(op)
                {
                    case 0: // ADD
                        UnwPrintd("ADD r%d, r%d\t; r%d (%08x %s)", rhd, rhs, rhs, state->regData[rhs].v, M_Origin2Str(state->regData[rhs].o));
                        state->regData[rhd].v += state->regData[rhs].v;
                        state->regData[rhd].o =  state->regData[rhs].o;
                        state->regData[rhd].o |= REG_VAL_ARITHMETIC;
                        break;

                    case 1: // CMP
                        // Irrelevant to unwinding
                        UnwPrintd("%s", "CMP ???");
                        break;

                    case 2: // MOV
                        UnwPrintd("MOV r%d, r%d\t; r%d (%08x %s), r%d (%08x %s)", rhd, rhs, rhd, state->regData[rhd].v, M_Origin2Str(state->regData[rhd].o), rhs, state->regData[rhs].v, M_Origin2Str(state->regData[rhs].o));
                        state->regData[rhd].v = state->regData[rhs].v;
                        state->regData[rhd].o = state->regData[rhs].o;
                        break;

                    case 3: // BX / BLX
                    {
                        UnwPrintd("%s r%d\t; r%d (%08x %s)\n", (h1 ? "BLX" : "BX"), rhs, rhs, state->regData[rhs].v, M_Origin2Str(state->regData[rhs].o));

                        // Only follow BX if the data was from the stack. In practice, BX is usually (always?) executed with an address from the stack.
                        if(state->regData[rhs].o == REG_VAL_FROM_STACK)
                        {
                            // Reset because we are now going to be starting anew reading the parent functions's instructions.
                            state->ResetInstructionCount();

                            // Report the return address, including mode bit
                            if(!UnwReportRetAddr(state, state->regData[rhs].v))
                                return UNWIND_TRUNCATED; // The output stack address array ran out of entries.

                            // Update the PC
                            currentInstrBranchDest = state->regData[rhs].v;
                            state->regData[15].v   = state->regData[rhs].v;

                            // Note by Paul P: Do we need to detect that we are using BLX instead of BX and recognize that the 
                            //                 we need to copy the next instruction's address (with bit 0 set) into LR?

                            // Determine the new mode.
                            // Note by Paul P: I don't think it's possible that we could ever branch to Thumb code here, 
                            // because the X in BX/BLX specifically means switch mode (to ARM).
                            if(state->regData[rhs].v & 0x1)
                            {
                                // Branching to THUMB
                                // Account for the auto-increment below which isn't needed.
                                state->regData[15].v -= 2;
                            }
                            else
                            {
                                // Branch to ARM
                                UnwPrintd("\n Branching to ARM code. Calling UnwStartArm recursively, line %d.\n", __LINE__);
                                return UnwStartArm(state);
                            }
                        }
                        else
                        {
                            UnwPrintd("\nError: BX to invalid register: r%d (%08x %s). Returning UNWIND_FAILURE, line %d.\n", rhs, state->regData[rhs].v, M_Origin2Str(state->regData[rhs].o), __LINE__);
                            return UNWIND_FAILURE;
                        }
                    }
                }
            }
        }

        // LDR/STR (register)
        // A6.2.4 Load/store single data item
        //else if()
        //{
        //}

        // LDR/STR (immeiate)
        // A6.2.4 Load/store single data item
        //   STR<c> <Rt>, [<Rn>{,#<imm5>}] -- A7.7.158 STR (immediate)
        //   LDR<c> <Rt>, [<Rn>{,#<imm5>}] -- A7.7.42  LDR (immediate)
        //   STRB<c> <Rt>,[<Rn>,#<imm5>]   -- A7.7.160 STRB (immediate)
        //   LDRB<c> <Rt>,[<Rn>{,#<imm5>}] -- A7.7.45  LDRB (immediate)
        //   STRH<c> <Rt>,[<Rn>{,#<imm5>}] -- A7.7.167 STRH (immediate)
        //   LDRH<c> <Rt>,[<Rn>{,#<imm5>}] -- A7.7.54 LDRH (immediate)
        //   STR<c> <Rt>,[SP,#<imm8>] -- A7.7.158 STR (immediate)
        //   LDR<c> <Rt>,[SP{,#<imm8>}] -- A7.7.42 LDR (immediate)
        else if(((instr[0] & 0xf000) == 0x6000) || // If top four bits are 0110...
                ((instr[0] & 0xf000) == 0x7000) || // If top four bits are 0111...
                ((instr[0] & 0xf000) == 0x8000) || // If top four bits are 1000...
                ((instr[0] & 0xf000) == 0x9000))   // If top four bits are 1001...
        {
            bool        bLoad     = ((instr[0] & 0x0800) != 0);
            int         Rt        = (instr[0] & 0x0007);         // Register in the range of 0-7
            int         Rn        = ((instr[0] >> 3) & 0x0007);  // Register in the range of 0-7
            int         imm       = ((instr[0] >> 6) & 0x001f);
            Bool        bRnValid  = M_IsOriginValid(state->regData[Rn].o);
            int         readSize;
            const char* pName;

            if(((instr[0] & 0xf000) == 0x6000)) // If LDR word
            {
                readSize = 4;
                pName    = bLoad ? "LDR" : "STR";
            }
            else if((instr[0] & 0xf000) == 0x7000)   
            {
                readSize = 1;
                pName    = bLoad ? "LDRB" : "STRB";
            }
            else if((instr[0] & 0xf000) == 0x8000)
            {
                readSize = 2;
                pName    = bLoad ? "LDRH" : "STRH";
            }
            else
            {
                // This is a special case of an SP-relative load. We rewrite a couple of the values.
                Rn       = 13;
                imm      = (instr[0] & 0x00ff);
                readSize = 4;
                bRnValid = M_IsOriginValid(state->regData[Rn].o);
                pName    = bLoad ? "LDR" : "STR";
            }

            UnwPrintd("%s r%d [r%d, #%d]\t; r%d (%08x %s), r%d (%08x %s)\n", pName, Rt, Rn, (imm << 2), Rt, state->regData[Rt].v, M_Origin2Str(state->regData[Rt].o), Rn, state->regData[Rn].v, M_Origin2Str(state->regData[Rn].o));

            if(bLoad)
            {
                if(bRnValid)
                {
                    if(readSize == 4) // If LDR word
                    {
                        uint32_t address = (state->regData[Rn].v + (imm << 2));

                        if(!UnwMemReadRegister(state, address, &state->regData[Rt]))
                        {
                            state->regData[Rt].o = REG_VAL_INVALID;

                            //UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                            //return UNWIND_DREAD_W_FAIL;
                        }
                    }
                    else // Else we currently assume that byte and half words aren't going to be important. You just don't see such instructions being related to stack and call maintenance.
                        state->regData[Rt].o = REG_VAL_INVALID;
                }
                else
                    state->regData[Rt].o = REG_VAL_INVALID;
            }
            else
            {
                if(bRnValid)
                {
                    if(((instr[0] & 0xf000) == 0x6000)) // If STR word
                    {
                        uint32_t address = (state->regData[Rn].v + (imm << 2));

                        if(!UnwMemWriteRegister(state, address, &state->regData[Rt]))
                        {
                            //Since the memory storage failed, we are kinda stuck and maybe should quit or restart with a different strategy.
                            //UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                            //return UNWIND_DREAD_W_FAIL;
                        }
                    }
                    // Else we currently assume that byte and half words aren't going to be important. You just don't see such instructions being related to stack and call maintenance.
                }
                else
                {
                    //UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                    //return UNWIND_DREAD_W_FAIL;
                }
            }
        }

        // Format 9: PC-relative load
        //   LDR Rd,[PC, #imm] -- A7.7.43 LDR (literal)     
        else if((instr[0] & 0xf800) == 0x4800)
        {
            uint8_t  rd      = (instr[0] & 0x0700) >> 8;
            uint8_t  word8   = (instr[0] & 0x00ff);
            uint32_t address = (state->regData[15].v & (~0x3)) + 4 + (word8 << 2); // Compute load address, adding a word to account for prefetch. ARM processors always are reading 4 bytes ahead of the currently executing instruction.

            UnwPrintd("LDR r%d, 0x%08x", rd, address);

            if(!UnwMemReadRegister(state, address, &state->regData[rd]))
            {
                UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                return UNWIND_DREAD_W_FAIL;
            }
        }

        // Format 13: add offset to Stack Pointer
        //   ADD sp,#+imm
        //   ADD sp,#-imm
        else if((instr[0] & 0xff00) == 0xB000)
        {
            uint16_t value = (instr[0] & 0x7f) * 4; // Use int16 in case the value gets truncated when it's too large for int8.

            // Check the negative bit
            if((instr[0] & 0x80) != 0)
            {
                UnwPrintd("SUB sp,#0x%04x", value);
                state->regData[13].v -= value;
            }
            else
            {
                UnwPrintd("ADD sp,#0x%04x", value);
                state->regData[13].v += value;
            }

            UnwPrintd("\t;r13 (%08x %s)", state->regData[13].v, M_Origin2Str(state->regData[13].o));
            
        }

        // Format 14: push/pop registers
        //   PUSH {Rlist}
        //   PUSH {Rlist, LR}
        //   POP {Rlist}
        //   POP {Rlist, PC}
        else if((instr[0] & 0xf600) == 0xb400)
        {
            Bool    L        = (instr[0] & 0x0800) ? TRUE : FALSE;
            Bool    R        = (instr[0] & 0x0100) ? TRUE : FALSE;
            uint8_t rList    = (instr[0] & 0x00ff);
            Bool    bSPValid = M_IsOriginValid(state->regData[13].o);

            if(L) // If POP...
            {
                // Load from memory: POP
                UnwPrintd("POP {Rlist%s} (SP %08x %s)\n", R ? ", PC" : "", state->regData[13].v, bSPValid ? "VALID" : "INVALID");

                if(bSPValid == FALSE)
                {
                    // We are screwed. Somehow the SP register became invalid, probably due to   
                    // being written by a REG_VAL_INVALID register or memory.
                    // To do: Switch to a more drastic mode of unwinding this frame.
                    UnwPrintd("\nPop attempt into invalid SP. Returning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                    return UNWIND_DREAD_W_FAIL;
                }

                for(unsigned r = 0; r < 8; r++)
                {
                    if(rList & (0x01 << r))
                    {
                        // Read the word
                        if(!UnwMemReadRegister(state, state->regData[13].v, &state->regData[r]))
                        {
                            UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                            return UNWIND_DREAD_W_FAIL;
                        }

                        // Alter the origin to be from the stack if it was valid.
                        if(M_IsOriginValid(state->regData[r].o))
                            state->regData[r].o = REG_VAL_FROM_STACK;

                        state->regData[13].v += 4;

                        UnwPrintd("  r%d = 0x%08x\n", r, state->regData[r].v);
                    }
                }

                if(R) // If the PC is to be popped (and thus we are returning from the subroutine) ...
                {
                    // Get the return address.
                    if(!UnwMemReadRegister(state, state->regData[13].v, &state->regData[15]))
                    {
                        UnwPrintd("\nReturning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                        return UNWIND_DREAD_W_FAIL;
                    }

                    // Alter the origin to be from the stack if it was valid.
                    if(!M_IsOriginValid(state->regData[15].o))
                    {
                        // Return address is not valid.
                        UnwPrintd("\nPC popped with invalid address. Returning UNWIND_FAILURE, line %d.\n", __LINE__);
                        return UNWIND_FAILURE;
                    }
                    else
                    {
                        if((state->regData[15].v & 0x1) == 0) // If the return address appears to be ARM code instead of Thumb code...
                        {
                            // You can return from Thumb to ARM via POP:
                            // http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0471c/Bajbagia.html
                            // "You can use BX, BLX, LDR, LDM, and POP instructions to perform the processor state changes."

                            // Store the return address.
                            if(!UnwReportRetAddr(state, state->regData[15].v))
                            {
                                UnwPrintd("\nReturning UNWIND_TRUNCATED, line %d.\n", __LINE__);
                                return UNWIND_TRUNCATED; // The output stack address array ran out of entries.
                            }

                            state->regData[13].v += 4; // Adjust the stack pointer to reflect the removal of return address.

                            UnwPrintd("Calling UnwStartArm recursively, line %d.\n", __LINE__);
                            return UnwStartArm(state);
                        }

                        currentInstrBranchDest = state->regData[15].v;

                        // Store the return address
                        if(!UnwReportRetAddr(state, state->regData[15].v))
                        {
                            UnwPrintd("Returning UNWIND_TRUNCATED, line %d.\n", __LINE__);
                            return UNWIND_TRUNCATED; // The output stack address array ran out of entries.
                        }

                        if(!state->cb->validatePC(state->regData[15].v))
                        {
                            UnwPrintd("  (appears to be invalid). Returning UNWIND_FAILURE, line %d\n", state->regData[15].v, __LINE__);
                            return UNWIND_FAILURE;
                        }

                        // Reset because we are now going to be starting anew reading the parent functions's instructions
                        state->ResetInstructionCount();

                        // Update the sp
                        state->regData[13].v += 4;

                        // Compensate for the instruction pointer increment that occurs at the end of this main loop.
                        state->regData[15].v -= 2;
                    }
                }
            }
            else // Else PUSH
            {
                // Store to memory: PUSH
                UnwPrintd("PUSH {Rlist%s}", R ? ", LR" : "");

                if(bSPValid == FALSE)
                {
                    // We are screwed. Somehow the SP register became invalid, probably due to   
                    // being written by a REG_VAL_INVALID register or memory.
                    // To do: Switch to a more drastic mode of unwinding this frame.
                    UnwPrintd("\nPop attempt into invalid SP. Returning UNWIND_DREAD_W_FAIL, line %d.\n", __LINE__);
                    return UNWIND_DREAD_W_FAIL;
                }

                // Check if the LR is to be pushed
                if(R)
                {
                    UnwPrintd("\n  lr = 0x%08x\t; (%08x %s)", state->regData[14].v, state->regData[14].v, M_Origin2Str(state->regData[14].o));

                    state->regData[13].v -= 4;

                    // Write the register value to our emulated memory.
                    if(!UnwMemWriteRegister(state, state->regData[13].v, &state->regData[14]))
                    {
                        UnwPrintd("Returning UNWIND_DWRITE_W_FAIL, line %d.\n", __LINE__);
                        return UNWIND_DWRITE_W_FAIL;
                    }
                }

                for(int r = 7; r >= 0; r--)
                {
                    if(rList & (0x1 << r))
                    {
                        UnwPrintd("\n  r%d = 0x%08x\t; (%08x %s)", r, state->regData[r].v, state->regData[r].v, M_Origin2Str(state->regData[r].o));

                        state->regData[13].v -= 4;

                        if(!UnwMemWriteRegister(state, state->regData[13].v, &state->regData[r]))
                        {
                            UnwPrintd("Returning UNWIND_DWRITE_W_FAIL, line %d\n", __LINE__);
                            return UNWIND_DWRITE_W_FAIL;
                        }
                    }
                }
            }
        }

        // Format 16: conditional branch
        //   B<c> <label>
        //   Architecture manual section A8.6.16
        //   BEQ label 
        //   BNE label 
        //   BCS label 
        //   BCC label 
        //   BMI label 
        //   BPL label 
        //   BVS label 
        //   BVC label 
        //   BHI label 
        //   BLS label
        else if((instr[0] & 0xf000) == 0xd000) // if bit pattern == 1101 xxxx xxxx xxxx
        {
            int16_t branchValue      = (2 * SignExtend8(instr[0] & 0xff));
            bool    shouldTakeBranch = false; // By default we ignore conditional branches, but if our stategy is to take them then do so.

            if(state->branchStrategy == kBSForwardConditionalBranches)
                shouldTakeBranch = (currentInstrBranchDest > state->regData[15].v); // True if the branch is forward.
            else if(state->branchStrategy == kBSRandomConditionalBranches)
                shouldTakeBranch = RandBool();

            UnwPrintd("B<c> %d (we %s it)\n", branchValue, shouldTakeBranch ? "take" : "ignore");
            currentInstrBranchDest = state->regData[15].v + 4 + branchValue;

            if(shouldTakeBranch)
            {
                UnwPrintd(" New PC=0x%08x", currentInstrBranchDest);  // Display PC of next instruction.
                state->regData[15].v += 2;  // The other +2 will occur below.
                state->regData[15].v += branchValue;
            }
        }

        // Format 18: unconditional branch
        //   B label
        else if((instr[0] & 0xf800) == 0xe000) // if bit pattern == 1110 0xxx xxxx xxxx
        {
            // Branch distance is twice that specified in the instruction, because the specified value is in instructions, not bytes.
            int16_t branchValue = (2 * SignExtend11(instr[0] & 0x07ff));
            bool    shouldTakeBranch = true; // By default we ignore unconditional branches, but if our stategy is to ignore them then do so.

            UnwPrintd("B %d\n", branchValue);

            // We have a potentially major problem here if the branchValue is negative.
            // The way this code works is it attempts to simulate instructions until it
            // gets to the instructions at the end of the subroutine. If we always execute
            // unconditional branches to backwards in the code, we may never get to 
            // the bottom of the function, depending on what code is above. We have
            // two primary ways to work around this: 1) Ignore the backward branch and 
            // just start executing the next instructions. 2) Have conditional branches
            // execute randomly instead of always the false condition. Both of these have
            // a chance for catastrophic failure (moving the PC into invalid code).
            // One thing we can try detect that we are in an infinite loop and if so 
            // switch to more drastic solutions such as the above two. Solution #2 is 
            // less risky. If we implement solution #1 we might want to clear register values
            // upon doing it and cross our fingers.
            //     if(we've been here N times already)
            //     {
            //         Enable conditional branch randomization and reset t counter to back to a high number.
            //     }
            //     if(the above didn't seem to work after N more times)
            //     {
            //         If the previous instruction is a conditional forward branch, 
            //         just take that branch instead of proceeding to the next instruction. 
            //         It turns out this is a common code generation pattern.
            //         Else set branchValue to -2, so we just proceed to the next instruction.
            //         Maybe clear registers before proceeding to the next instruction.
            //     }
            //     Don't forget to clear this state on each new call frame.
            //     Need to clear the counter every time we take 

            // This is the count of the number of times we've been at this code address. Used for infinite loop detection.
            uint32_t count = state->IncrementAddressCount(state->regData[15].v);

            if(branchValue < 0)
            {
                if(state->branchStrategy == kBSIgnoreConditionalBranches) 
                {
                    // kBSIgnoreConditionalBranches is the initial, most conservative strategy. 

                    if(count > 16) // If we appear to be in an infinite loop...
                    {
                        // If we are here then the kBSIgnoreConditionalBranches strategy isn't working
                        // for us with this call frame. If it turns out that the instruction 
                        // prior to this one is a conditionally forward branch, we take it.
                        // Otherwise we switch to a new branch strategy.

                        if((state->prevAddr == (state->regData[15].v - 2)) &&  // If the previously executed instruction was a  conditional forward 
                           (state->prevBranchDest > state->regData[15].v))     // branch... take it. That will probably take us out of this infinite loop.
                        {
                            currentInstrAddr       = state->prevAddr;               // We pretend we are back on the previous instruction.
                            currentInstrBranchDest = state->prevBranchDest;         //
                            UnwPrintd(" New PC=0x%08x", currentInstrBranchDest);   // Display PC of next instruction.

                            state->regData[15].v = (state->prevBranchDest - 2); // -2 to offset the +2 below.

                            shouldTakeBranch = false; // We already 'branched' directly above.
                        }
                        else
                            state->branchStrategy = kBSIgnoreConditionalBranches;

                        state->ResetInstructionCount();
                    }
                }
                //else if(state->branchStrategy == kBSForwardConditionalBranches)
                //{
                //   Nothing to do here since the current instruction is an unconditional branch.
                //   This strategy is handled by the conditional branch handling code above.
                //}
                //else if(state->branchStrategy == kBSRandomConditionalBranches)
                //{
                //   Nothing to do here since the current instruction is an unconditional branch.
                //   This strategy is handled by the conditional branch handling code above.
                //}
                else if(state->branchStrategy == kBSIgnoreBackwardUnconditionalBranches)
                {
                    if(count > 16) // If we appear to be in an infinite loop... we've tried the above strategies already and they didn't work. So we try to just break out of this loop.
                    {
                        // Do nothing but continue to the next instruction (hoping it's actually an instruction and not data).
                        UnwInvalidateRegisterFile(state->regData, true);
                        shouldTakeBranch = false;
                        state->ResetInstructionCount();
                    }
                }
            }

            if(shouldTakeBranch)
            {
                // Need to advance by a 32 bit word to account for instruction pipeline pre-fetch. Actually advance by 
                // a half word here, allowing the normal address advance below to account for the other half word.
                currentInstrBranchDest = state->regData[15].v + 4 + branchValue;
                UnwPrintd(" New PC=0x%08x", currentInstrBranchDest);  // Display PC of next instruction.

                state->regData[15].v += (branchValue + 2);  // The other +2 will occur below.
            }
        }

        else if(ThumbInstructionIs32Bit(instr[0]))
        {
            // Skip decoding this for the time being, as it's always used for function calls in our case.

            // Long branch (usually a function call)
            //if((instr[0] & 0xf000) == 0xf000) // if bit pattern == 1111 xxxx xxxx xxxx
            //if((instr[0] & 0x800) == 0) // If bit 11 == 0
            //{
                // BL
                // I1 = NOT(J1 EOR S); I2 = NOT(J2 EOR S); imm32 = SignExtend(S:I1:I2:imm10:imm11:’0’, 32);
                // BLX
                // I1 = NOT(J1 EOR S); I2 = NOT(J2 EOR S); imm32 = SignExtend(S:I1:I2:imm10H:imm10L:’00’, 32);
                // state->regData[15].v += ((instr[0] & 0x07ff) << 12); // Offset field (low 11 bits) shifted left by 12.
                // state->regData[15].v += 2;
                // pc += (instr[0] & 0x07ff) << 12) + 2
                // Possibly update 'currentInstrBranchDest'
            //}

            // The called function could modify registers 0-3, but it's our code's responsibility
            // to save and restore those registers, and so our simulation code would already be 
            // doing that. The called function must return registers r4-r11 to their state upon 
            // returning. So again we don't need to do anything here about register preservation.

            // Skip the second part of this 32 bit instruction.
            state->regData[15].v += 2;
        }

        // Skipped instruction
        else
        {
            UnwPrintd("Skipped or unknown instruction: %04x:", instr[0]);

            // Unknown/undecoded.  May alter some register, so invalidate file.
            // To do: handle this instruction rather than just invalidate the register file.
            //        We probably don't need to simulate the instruction; just need to know 
            //        what register or memory it might write to and invalidate it.
            UnwInvalidateRegisterFile(state->regData, true); // true=preserve frame pointer. Chances are very slim that whatever instruction this was, it didn't modify the frame pointer, at least if the frame pointer is being used as such.
        }

        UnwPrintd("%s", "\n");

        // Should never hit the reset vector.
        if(state->regData[15].v == 0) 
        {
            UnwPrintd("PC == 0. Returning UNWIND_RESET, line %d.\n", __LINE__);
            return UNWIND_RESET;
        }

        // Denote the previous instruction and its branch dest (if it was a branch instruction).
        state->prevAddr       = currentInstrAddr;
        state->prevBranchDest = currentInstrBranchDest;

        // Move the instruction pointer to the next Thumb instruction address.
        state->regData[15].v += 2;

        // Garbage collect the memory hash (used only for the stack).
        UnwMemHashGC(state);

        if(++state->instructionCount == UNW_MAX_INSTR_COUNT)
        {
            UnwPrintd("Returning UNWIND_EXHAUSTED, line %d\n", __LINE__);
            return UNWIND_EXHAUSTED;
        }
    }
    while(!found);

    UnwPrintd("Returning UNWIND_SUCCESS, line %d\n", __LINE__);
    return UNWIND_SUCCESS;
}




EA_NO_INLINE
UnwResult UnwindStart(uint32_t fp, uint32_t sp, uint32_t lr, uint32_t pc, const UnwindCallbacks* cb, void* data)
{
    UnwState  stateObject;
    UnwState* state = &stateObject;  // Create a pointer called 'state' in order to be consistent and let the UnwPrintd macro usage below comple.

    UnwInitState(state, cb, data, fp, sp, lr, pc);
    UnwPrintd("UnwindStart fp: 0x%08x sp: 0x%08x lr: 0x%08x pc: 0x%08x\n", fp, sp, lr, pc);

    // Check the Thumb bit. Instruction addresses that have bit 1 set are thumb instructions.
    if(pc & 0x1)
        return UnwStartThumb(state);
    else
        return UnwStartArm(state); 
}




#define M_IsIdxUsed(a, v) (((a)[v >> 3] &   (1 << (v & 0x7))) ? TRUE : FALSE)
#define M_SetIdxUsed(a, v) ((a)[v >> 3] |=  (1 << (v & 0x7)))
#define M_ClrIdxUsed(a, v) ((a)[v >> 3] &= ~(1 << (v & 0x7)))

// Search the memory hash to see if an entry is stored in the hash already.
// This will search the hash and either return the index where the item is
// stored, or -1 if the item was not found.
static int16_t memHashIndex(MemData* memData, uint32_t addr)
{
    const uint16_t v = addr % MEM_HASH_SIZE;
    uint16_t       s = v;

    do
    {
        // Check if the element is occupied
        if(M_IsIdxUsed(memData->used, s))
        {
            // Check if it is occupied with the sought data
            if(memData->a[s] == addr)
                return s;
        }
        else
        {
            // Item is free, this is where the item should be stored
            return s;
        }

        // Search the next entry
        s++;
        if(s > MEM_HASH_SIZE)
            s = 0;
    }
    while(s != v);

    // Search failed, hash is full and the address not stored
    return -1;
}


Bool UnwMemHashRead(MemData*  memData,
                    uint32_t  addr,
                    uint32_t* data,
                    Bool*     tracked)
{
    int16_t i = memHashIndex(memData, addr);

    if((i >= 0) && M_IsIdxUsed(memData->used, i) && (memData->a[i] == addr))
    {
        *data    = memData->v[i];
        *tracked = M_IsIdxUsed(memData->tracked, i);
        return TRUE;
    }

    return FALSE; // Address not found in the hash
}


Bool UnwMemHashWrite(MemData* memData,
                     uint32_t addr,
                     uint32_t val,
                     Bool     valValid)
{
    int16_t i = memHashIndex(memData, addr);

    if(i < 0)
        return FALSE; // Hash full

    // Store the item
    memData->a[i] = addr;
    M_SetIdxUsed(memData->used, i);

    if(valValid)
    {
        memData->v[i] = val;
        M_SetIdxUsed(memData->tracked, i);
    }
    else
    {
        #if defined(UNW_DEBUG)
            memData->v[i] = 0xdeadbeef;
        #endif

        M_ClrIdxUsed(memData->tracked, i);
    }

    return TRUE;
}


// Clear all memory entries that refer to memory below the current SP. 
// This is useful only if the data we store was written to stack memory 
// as opposed to heap memory.
void UnwMemHashGC(UnwState* state)
{
    uint32_t minValidAddr = state->regData[13].v; // SP (stack pointer) value
    MemData* memData = &state->memData;

    for(int t = 0; t < MEM_HASH_SIZE; t++)
    {
        if(M_IsIdxUsed(memData->used, t) && (memData->a[t] < minValidAddr))
        {
            UnwPrintd("MemHashGC: Free elem %d, addr 0x%08x\n", t, memData->a[t]);

            M_ClrIdxUsed(memData->used, t);
        }
    }
}


bool RandBool()
{
    return (rand() & 0x80) != 0; // Pick some bit to use.
}




static Bool CliReport(void* data, uint32_t address);
static Bool CliReadW(uint32_t a, uint32_t* v);
static Bool CliReadH(uint32_t a, uint16_t* v);
static Bool CliReadB(uint32_t a, uint8_t* v);
static Bool CliValidatePC(uint32_t pc);


// Table of function pointers for passing to the unwinder
UnwindCallbacks cliCallbacks =
{
    CliReport,
    CliReadW,
    CliReadH,
    CliReadB,
    CliValidatePC,
    UnwPrintf
};



////////////////////////////////////////////////////////////////////////////////
// CliReport
//
// Parameters:   data    - Pointer to data passed to UnwindStart()
//               address - The return address of a stack frame.
//
// Returns:      TRUE if unwinding should continue, otherwise FALSE to
//                 indicate that unwinding should stop.
//
// Description:  This function is called from the unwinder each time a stack
//                 frame has been unwound.  The LSB of address indicates if
//                 the processor is in ARM mode (LSB clear) or Thumb (LSB
//                 set).
//
////////////////////////////////////////////////////////////////////////////////
static Bool CliReport(void* data, uint32_t address)
{
    CliStack* s = (CliStack*)data;

    s->address[s->frameCount++] = address;

    #if defined(UNW_DEBUG)
        EA::StdC::Printf("\nNew entry: 0x%08x\n", address);
        EA::StdC::Printf("Current entries:");
        for(int i = 0; i < s->frameCount; i++)
            EA::StdC::Printf(" 0x%08x", s->address[i]);
        EA::StdC::Printf("\n");
    #endif

    return (s->frameCount < (sizeof(s->address) / sizeof(s->address[0]))) ? TRUE : FALSE;
}

static Bool CliReadW(uint32_t a, uint32_t* v)
{
    *v = *(uint32_t*)a;
    return TRUE;
}

static Bool CliReadH(uint32_t a, uint16_t* v)
{
    *v = *(uint16_t*)a;
    return TRUE;
}

static Bool CliReadB(uint32_t a, uint8_t* v)
{
    *v = *(uint8_t*)a;
    return TRUE;
}

static Bool CliValidatePC(uint32_t /*pc*/)
{
    return TRUE;

    // Enable this code once we feel confident in it and can test it:

    // This is a primitive version until we can make this code mature.
    // To consider: Use .map files as a way to help validate this.
    // To do: We should instead just see if the pc is within ~20 MB of 
    // the current pc. Also, if we know that this pc is supposed to be 
    // a function return address, we can verify that the instruction before 
    // it (be it Thumb or ARM) is a BL or BLX function call instruction.
    //
    // void* pcThisCode_;
    // EAGetInstructionPointer(pcThisCode_);
    // uint32_t pcThisCode = (uint32_t)(uintptr_t)pcThisCode_;
    // 
    // const uint32_t top12BitsThisCode = (pcThisCode & 0xfff00000);
    // const uint32_t top12BitsPCInput  = (pc & 0xfff00000);
    // 
    // if(top12BitsThisCode == top12BitsPCInput)
    //     return TRUE;
    // else
    //     return FALSE;
}

Bool CliInvalidateW(uint32_t a)
{
    *(uint32_t*)a = 0xdeadbeef;
    return TRUE;
}

