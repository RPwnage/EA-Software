///////////////////////////////////////////////////////////////////////////////
// EACallstack_PS2.cpp
//
// Copyright (c) 2003-2005 Electronic Arts Inc.
// Created by Paul Pedriana
//
// This implementation is based on previous work by:
//     Simon Everett
//     Vasyl Tsvirkunov
//     Avery Lee
//     Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// MIPS (PS2) callstack trace by Simon Everett (severett@ea.com), x85443
//
// The original PS2 callstack trace here was not correct.  It assumed that you could identify the 
// start of a function by looking for the return address to be written to the stack.  This may 
// not always happen, frequently in optimized code.
// In fact it is not possible to find the start of a C/C++ function in MIPS code.  It is however
// possible to find the end[*] by looking for the return (jr $ra).
//
// The code below correctly handles local frame allocations [alloca()] by restoring the 
// stack pointer from the frame pointer (when used).  There appears to be a bug in the SN
// provided gcc 2.95.3 - cc1,cc1plus 2.95.3 SN 1.64 (ee), where an unusually large stack is
// generated for no aprarent reason.  In this case, the stack pointer is not used directly
// but rather indirected through $t4/$t5.  This is handled correctly.
//
// The code below will also handle MetroWerks generated code (up to 3.5), but may need
// masssaging to know when to stop the trace.
//
// The code will also handle QW or DW register stores to the stack frame (custom gcc extension
// by SN, -fopt-stack ).  However the code generation by the compiler is sometimes incorrect
// and I would warn strongly against using it (-fopt-stack), since the bugs are hard to find.
//
// Note:  This callstack code will fail if not called from the primary thread in a multithreaded
// environment, and also if called from an interrupt context.  Contact me if you need support for
// this.
//
// [*] Most of the time.  There is a function tail optimization where instead of returning
// to the caller, a non-leaf function jumps to a leaf function (j), and the leaf function
// then returns (jr $ra) to the caller's caller directly when it returns.  This can be hard
// to detect, and rarely happens.  No attempt is made to find this case below.
//
// If this callstack fails, please contact me (severett@ea.com) so that I can look at your
// generated code.  Thank you.
///////////////////////////////////////////////////////////////////////////////


#include <EACallstack/EACallstack.h>
#include <stddef.h>


extern "C" int main( int, char** );


//--------------------------------------------------------------------------------------------------------------------------------
// Walk the code, looking for the end of a function, and (optionally) a stack frame
//
// The end of a function is found by looking for the return, jr $ra

#define INSTR_OFFSET(_i)    (int16_t)((_i) & 0x0000ffff)

//             31-29   28-26   25-21   20-16    15-0                    (1)
//             31-29   28-26   25-0                                     (2)
//             31-29   28-26   25-21   20-6             5-3     2-0     (3)
//             31-29   28-26   25-21   20-16 15-11 10-6 5-3     2-0     (4)


//  j           000     010             offset                          (2)     j offset
//  addiu       001     001      rs     rt      immediate               (1)     addiu rt, rs, imm
//  ori(li)     001     101      rs     rt      immediate               (1)     ori rt, rs, imm
//  lq          011     110     base    rt      offset                  (1)     lq rt, offset(base)
//  lw          100     011     base    rt      offset                  (1)     lw rt, offset(base)
//  ld          110     111     base    rt      offset                  (1)     ld rt, offset(base)
//  sd          111     111     base    rt      offset                  (1)     sd rt, offset(base)

#define OPCODE_MASK     0xfc000000
#define OPCODE_SPECIAL_MASK 0x000007ff

#define OPCODE_SPECIAL  0x00000000

#define OPCODE_J        0x08000000
#define OPCODE_ADDIU    0x24000000
#define OPCODE_ORI      0x34000000
#define OPCODE_LQ       0x78000000
#define OPCODE_LW       0x8c000000
#define OPCODE_LD       0xdc000000
#define OPCODE_SD       0xfc000000

//  jr          000     000      rs   000000000000000   001     000     (3)     jr rs
//  addu        000     000      rs     rt   rd   00000 100     001     (4)     addu rd, rs, rt
//  daddu       000     000      rs     rt  rd    00000 101     101     (4)     daddu rd, rs, rt

#define OPCODE_SPECIAL_ADDU     0x21
#define OPCODE_SPECIAL_DADDU    0x2d
#define OPCODE_SPECIAL_JR       0x8

// addiu $sp, $sp, i
#define INSTR_ADDIU_SP_SP_OFFSET            0x27bd0000
#define INSTR_BITMASK_ADDIU_SP_SP_OFFSET    0xffff0000

// lw $ra, n($sp)
#define INSTR_LW_RA_OFFSET_SP               0x8fbf0000
#define INSTR_BITMASK_LW_RA_OFFSET_SP       0xffff0000

// ld $ra, n($sp)
#define INSTR_LD_RA_OFFSET_SP               0xdfbf0000
#define INSTR_BITMASK_LD_RA_OFFSET_SP       0xffff0000

// lq $ra, n($sp)
#define INSTR_LQ_RA_OFFSET_SP               0x7bbf0000
#define INSTR_BITMASK_LQ_RA_OFFSET_SP       0xffff0000

// lq $fp, n($sp)
#define INSTR_LQ_FP_OFFSET_SP               0x7bbe0000
#define INSTR_BITMASK_LQ_FP_OFFSET_SP       0xffff0000

// jr $ra
#define INSTR_JR_RA                         0x03e00008
#define INSTR_BITMASK_JR_RA                 0xffffffff

// j n
#define INSTR_J                             0x08000000
#define INSTR_BITMASK_J                     0xfc000000

// sd $ra, n($sp)
#define INSTR_SD_RA_OFFSET_SP               0xffbf0000
#define INSTR_BITMASK_SD_RA_OFFSET_SP       0xffff0000

// daddu $sp, $fp
#define INSTR_DADDU_SP_FP                   0x03c0e82d
#define INSTR_BITMASK_DADDU_SP_FP           0xffffffff

// jal
#define INSTR_JAL                           0x0c000000
#define INSTR_BITMASK_JAL                   0xfc000000

//
// I've only ever seen this optimized code in 3 functions where the local
// stackframe is large (~ 32KB).
// I strongly suspect this is due a compiler bug generating a stack frame which is
// much too large. (gcc 2.95.3, cc1,cc1plus 2.95.3 SN 1.64 (ee))
//
// The unusual function epilog would look something like this:
//
//  li      $t4, 0x8ae0
//  addu    $t5, $t4, $sp
//  ld      $ra, 0xff70( $t5 )
//  jr      $ra
//  addu    $sp, $sp, $t4
//

// li $t4, i
#define INSTR_ORI_T4_ZERO_OFFSET            0x340c0000
#define INSTR_BITMASK_ORI_T4_ZERO_OFFSET    0xffff0000

// addu $t5, $t4, $sp
#define INSTR_ADDU_T5_T4_SP                 0x019d6821
#define INSTR_BITMASK_ADDU_T5_T4_SP         0xffffffff

// ld $ra, n($t5)
#define INSTR_LD_RA_OFFSET_T5               0xddbf0000
#define INSTR_BITMASK_LD_RA_OFFSET_T5       0xffff0000

// addu $sp, $sp, $t4
#define INSTR_ADDU_SP_SP_T4                 0x03ace821
#define INSTR_BITMASK_ADDU_SP_SP_T4         0xffffffff

namespace
{
    struct CallstackParams
    {
        uint32_t* pc;          ///< Program counter
        uint32_t* sp;          ///< Stack pointer
        uint32_t* fp;          ///< Frame pointer
        uint32_t* ra;          ///< Return address

        uint32_t* t4;          ///< Temp register
        uint32_t* t5;          ///< Temp register
    };

    void Leaf() {}
}



namespace EA
{
namespace Callstack
{


///////////////////////////////////////////////////////////////////////////////
// InitCallstack
//
EACALLSTACK_API void InitCallstack()
{
    // Nothing needed.
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownCallstack
//
EACALLSTACK_API void ShutdownCallstack()
{
    // Nothing needed.
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstack
//
EACALLSTACK_API size_t GetCallstack(void* pReturnAddressArray[], size_t nReturnAddressArrayLength, const CallstackContext* pContext)
{
    size_t finalDepth = 0;

    if( 0 != pReturnAddressArray )
    {
        // Get the flow registers pc,sp,fp,ra
        CallstackParams params;

        params.t4 = params.t5 = 0;

        if(pContext)
        {
            params.pc = (uint32_t*)pContext->mPC;
            params.ra = (uint32_t*)pContext->mRA;
            params.sp = (uint32_t*)pContext->mSP;
            params.fp = (uint32_t*)pContext->mFP;
        }
        else
        {
            asm __volatile__
            (
                "
                    .set noreorder

                    sw      $31, 0(%1)
                    sw      $30, 0(%3)

                    jalr    %4
                    sw      $29, 0(%2)

                    sw      $31, 0(%0)
                    lw      $31, 0(%1)

                    .set reorder
                " :
                : "r" (&params.pc), "r" (&params.ra), "r" (&params.sp), "r" (&params.fp), "r" (Leaf)
                : "$31", "memory"
            );
        }

        for( ; finalDepth < nReturnAddressArrayLength; ++finalDepth )
        {
            uint32_t instr, returnToCaller = 0;
            for ( instr = *params.pc; !returnToCaller; instr = *(++params.pc) )
            {
                uint32_t offset = INSTR_OFFSET( instr ) >> 2;

                // opcode encoded in bits 31-29 and 28-26
                switch( instr & OPCODE_MASK )
                {
                    default:
                        break;

                    case OPCODE_SPECIAL:
                        switch( instr & (INSTR_BITMASK_ADDU_T5_T4_SP | INSTR_BITMASK_DADDU_SP_FP ) )
                        {
                            case INSTR_JR_RA:
                                returnToCaller = 1;
                                break;

                            case INSTR_ADDU_T5_T4_SP:
                                params.t5 = params.t4 + reinterpret_cast<uint32_t>(params.sp);
                                break;

                            case INSTR_DADDU_SP_FP:
                                params.sp = params.fp;
                                break;
                        }
                        break;

                    case OPCODE_ADDIU:
                        if ((instr & INSTR_BITMASK_ADDIU_SP_SP_OFFSET ) == INSTR_ADDIU_SP_SP_OFFSET )
                        {
                            params.sp += offset;
                        }
                        break;

                    case OPCODE_ORI:
                        if((instr & INSTR_BITMASK_ORI_T4_ZERO_OFFSET ) == INSTR_ORI_T4_ZERO_OFFSET )
                        {
                            params.t4 = reinterpret_cast<uint32_t*>(offset);
                        }
                        break;

                    case OPCODE_LQ:
                        switch( instr & (INSTR_BITMASK_LQ_RA_OFFSET_SP | INSTR_BITMASK_LQ_FP_OFFSET_SP) )
                        {
                            case INSTR_LQ_RA_OFFSET_SP:
                            params.ra = reinterpret_cast<uint32_t*>(*(params.sp + offset));
                                break;
                            case INSTR_LQ_FP_OFFSET_SP:
                            params.fp = reinterpret_cast<uint32_t*>(*(params.sp + offset));
                                break;
                        }
                        break;

                    case OPCODE_LW:
                        if((instr & INSTR_BITMASK_LW_RA_OFFSET_SP ) == INSTR_LW_RA_OFFSET_SP )
                        {
                            params.ra = reinterpret_cast<uint32_t*>(*(params.sp + offset));
                        }
                        break;

                    case OPCODE_LD:
                        switch( instr & (INSTR_BITMASK_LD_RA_OFFSET_SP | INSTR_BITMASK_LD_RA_OFFSET_T5) )
                        {
                            case INSTR_LD_RA_OFFSET_SP:
                            params.ra = reinterpret_cast<uint32_t*>(*(params.sp + offset));
                                break;
                            case INSTR_LD_RA_OFFSET_T5:
                                params.ra = reinterpret_cast<uint32_t*>(*(params.t5 + offset));
                                break;
                        }
                        break;

                    case OPCODE_SD:
                        if( (instr & INSTR_BITMASK_SD_RA_OFFSET_SP ) == INSTR_SD_RA_OFFSET_SP )
                        {
                            *(params.sp + offset ) = reinterpret_cast<uint32_t>(params.ra);
                        }
                        break;
                }
            }

            // Handle the instruction in the delay slot
            params.pc = params.ra;
            if ((instr & INSTR_BITMASK_ADDIU_SP_SP_OFFSET ) == INSTR_ADDIU_SP_SP_OFFSET )
            {
                // The stack pointer is adjusted in the delay slot
                // This often happens in optimized code
                params.sp += INSTR_OFFSET( instr ) >> 2;
            }
            else if( (instr & INSTR_BITMASK_ADDU_SP_SP_T4 ) == INSTR_ADDU_SP_SP_T4 )
            {
                params.sp += reinterpret_cast<uint32_t>(params.t4);
            }
            else if( (instr & INSTR_BITMASK_LD_RA_OFFSET_SP) == INSTR_LD_RA_OFFSET_SP )
            {
                // The return address for the parent function is loaded from the stack in the delay slot
                // This rarely happens, and usually only in optimized code
                params.ra = reinterpret_cast<uint32_t*>(*(params.sp + (INSTR_OFFSET( instr ) >> 2)));
            }

            uint32_t ep = reinterpret_cast<uint32_t>(params.pc);
            pReturnAddressArray[finalDepth] = reinterpret_cast<void*>(ep);

            // Lower 1MB is reserved for OS
            if( ep < 0x100000 )
            {
                // This is the end. Something bad has happened, this is not in the
                // application address space.
                break;
            }
            else
            {
                // Was the function call a call to main?  If so, terminate.
                uint32_t instr = *(params.pc - 2);

                if( ( ( instr & INSTR_BITMASK_JAL ) == INSTR_JAL ) || ( ( instr & INSTR_BITMASK_J ) == INSTR_J ) )
                {
                    uint32_t mainAddr = reinterpret_cast<uint32_t>(main);
                    instr = *(params.pc - 2);
                    // INSTR_J and INSTR_JAL have same format
                    uint32_t addr = ((instr & ~(INSTR_BITMASK_JAL)) << 2) | (0xf0000000 & reinterpret_cast<uint32_t>((params.pc - 2)));

                    if( mainAddr == addr )
                    {
                        // In the crt0, just after calling main()
                        break;
                    }
                }
            }
        }
    }

    return finalDepth;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContext
//
EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId)
{
    // To do.
    (void)context;
    (void)threadId;
    return false;
}


///////////////////////////////////////////////////////////////////////////////
// GetCallstackContextSysThreadId
//
EACALLSTACK_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t threadId)
{
    return GetCallstackContext(context, threadId);
}


} // namespace Callstack
} // namespace EA





