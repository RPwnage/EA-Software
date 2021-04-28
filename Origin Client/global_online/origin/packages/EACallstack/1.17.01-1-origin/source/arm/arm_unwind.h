///////////////////////////////////////////////////////////////////////////////
// arm_unwind.h
//
// Copyright (c) 2011 Electronic Arts Inc.
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


#ifndef EACALLSTACK_ARM_UNWIND_H
#define EACALLSTACK_ARM_UNWIND_H


#include <EASTL/fixed_map.h>


// Forward declarations
struct UnwState;
struct UnwindCallbacks;
struct RegData;
struct MemData;


// To do: Replace this with C/C++ bool.
enum Bool
{
    FALSE,
    TRUE
};


// If this define is set, additional information will be produced while
// unwinding the stack to allow debug of the unwind module itself.
#if EACALLSTACK_DEBUG_DETAIL_ENABLED
    #define UNW_DEBUG 1
#endif


// Possible results for UnwindStart to return.
enum UnwResult
{
    UNWIND_SUCCESS = 0,       // Unwinding was successful and complete.
    UNWIND_EXHAUSTED,         // More than UNW_MAX_INSTR_COUNT instructions were interpreted.
    UNWIND_TRUNCATED,         
    UNWIND_INCONSISTENT,      // Read data was found to be inconsistent.
    UNWIND_UNSUPPORTED,       // Unsupported instruction or data found.
    UNWIND_FAILURE,           // General failure.
    UNWIND_ILLEGAL_INSTR,     // Illegal instruction.
    UNWIND_RESET,             // Unwinding hit the reset vector.
    UNWIND_IREAD_W_FAIL,      // Failed read for an instruction word.
    UNWIND_IREAD_H_FAIL,      // Failed read for an instruction half-word.
    UNWIND_IREAD_B_FAIL,      // Failed read for an instruction byte.
    UNWIND_DREAD_W_FAIL,      // Failed read for a data word.
    UNWIND_DREAD_H_FAIL,      // Failed read for a data half-word.
    UNWIND_DREAD_B_FAIL,      // Failed read for a data byte.
    UNWIND_DWRITE_W_FAIL      // Failed write for a data word.
};


// Type for function pointer for result callback.
// The function is passed two parameters, the first is a void * pointer,
// and the second is the return address of the function.  The bottom bit
// of the passed address indicates the execution mode; if it is set,
// the execution mode at the return address is Thumb, otherwise it is ARM.
//
// The return value of this function determines whether unwinding should
// continue or not.  If TRUE is returned, unwinding will continue and the
// report function maybe called again in future.  If FALSE is returned,
// unwinding will stop with UnwindStart() returning UNWIND_TRUNCATED.
typedef Bool (*UnwindReportFunc)(void* data, uint32_t address);


// Structure that holds memory callback function pointers.
// To consider: Replace with a C++ interface.
struct UnwindCallbacks
{
    // Report an unwind result.
    UnwindReportFunc report;

    // Read a 32 bit word from memory.
    // The memory address to be read is passed as address, and
    // *val is expected to be populated with the read value.
    // If the address cannot or should not be read, FALSE can be
    // returned to indicate that unwinding should stop. If TRUE
    // is returned, *val is assumed to be valid and unwinding
    // will continue.
    Bool (*readW)(uint32_t address, uint32_t *val);

    // Read a 16 bit half-word from memory.
    // This function has the same usage as for readW, but is expected
    // to read only a 16 bit value.
    Bool (*readH)(uint32_t address, uint16_t *val);

    // Read a byte from memory.
    // This function has the same usage as for readW, but is expected
    // to read only an 8 bit value.
    Bool (*readB)(uint32_t address, uint8_t  *val);

    // Validates a candidate PC (program counter)
    // Before setting a new PC value, it's validated with this function.
    Bool (*validatePC)(uint32_t pc);

    // Print a formatted line for debug.
    int (*printf)(const char *format, ...);
};

extern UnwindCallbacks cliCallbacks;



// Start unwinding the current stack.
// This will unwind the stack starting at the PC value supplied to in the
// link register (i.e. not a normal register) and the stack pointer value supplied.
UnwResult UnwindStart(uint32_t fp, uint32_t sp, uint32_t lr, uint32_t pc, const UnwindCallbacks* cb, void* data);


// The maximum number of instructions to interpet in a function.
// Unwinding will be unconditionally stopped and UNWIND_EXHAUSTED returned
// if more than this number of instructions are interpreted in a single
// function without unwinding a stack frame.  This prevents infinite loops
// or corrupted program memory from preventing unwinding from progressing.
#define UNW_MAX_INSTR_COUNT 10000


enum RegValOrigin
{
    REG_VAL_INVALID       = 0x00,
    REG_VAL_FROM_STACK    = 0x01,
    REG_VAL_FROM_MEMORY   = 0x02,
    REG_VAL_FROM_CONST    = 0x04,
    REG_VAL_ARITHMETIC    = 0x80,
    REG_VAL_FROM_ANY_MASK = 0x7f // Probably should instead be (REG_VAL_FROM_STACK | REG_VAL_FROM_MEMORY | REG_VAL_FROM_CONST)
};

#define M_IsOriginValid(o) (((o) & REG_VAL_FROM_ANY_MASK) ? TRUE : FALSE)
#define M_Origin2Str(o)    ((o) ? "VALID" : "INVALID")


// Type for tracking information about a register.
// This stores the register value, as well as other data that helps unwinding.
struct RegData
{
    uint32_t v;      // The value held in the register.
    int32_t  o;      // This is an OR of RegValOrigin values. The origin of the register value. This is used to track how the value in the register was loaded.

    RegData() : v(0), o(0){}
};


// Structure used to track reads and writes to memory.
// This structure is used as a hash to store a small number of writes to memory.
// To consider: Change this code to use a C++ bitset and array. Or maybe a hash_map<addr, info>.

#define MEM_HASH_SIZE 53 // The size of the hash used to track reads and writes to memory. This should be a prime value for efficiency.

struct MemData
{
    uint32_t a[MEM_HASH_SIZE];                  // Address at which v[n] represents.
    uint32_t v[MEM_HASH_SIZE];                  // Memory contents.
    uint8_t  used[(MEM_HASH_SIZE + 7) / 8];     // Indicates whether the data in v[n] and a[n] is occupied. Each bit represents one hash value.
    uint8_t  tracked[(MEM_HASH_SIZE + 7) / 8];  // Indicates whether the data in v[n] is valid. This allows a[n] to be set, but for v[n] to be marked as invalid. Specifically this is needed for when an untracked register value is written to memory.

    MemData()
    {
        memset(this, 0, sizeof(MemData));
    }
};

void UnwMemHashGC(UnwState* state);
Bool UnwMemHashRead(MemData* memData, uint32_t addr, uint32_t* data, Bool* tracked);
Bool UnwMemHashWrite(MemData* memData, uint32_t addr, uint32_t val, Bool valValid);


// Used to keep track of how often a given instruction has been encountered,
// for the purpose of detecting infinite loops.
typedef eastl::fixed_map<uint32_t, uint32_t, 32> AddressCountMap; // Instruction address and the count of times it's been encountered.



enum BranchStrategy
{
    kBSIgnoreConditionalBranches,           // Initial most conservative strategy: Ignore conditional branches, heed unconditional branches.
    kBSForwardConditionalBranches,          // Don't ignore conditional branches, choose whichever destination is more forward.
    kBSRandomConditionalBranches,           // Don't ignore conditional branches, we randomly choose which branch to take.
    kBSIgnoreBackwardUnconditionalBranches, // Just skip to the next instruction after a backward unconditional branch. This is a bit risky since the next bytes might not even be instructions.
    kBSCount
};


// Structure that is used to keep track of unwinding meta-data.
// This data is passed between all the unwinding functions.
// The register values and meta-data.
//     regData[0]   Volatile register. Argument1, return value.
//     regData[1]   Volatile register. Argument2, Second 32-bits if double/int Return Value
//     regData[2]   Volatile register. Argument3.
//     regData[3]   Volatile register. Argument4. Further arguments are put on the stack.
//     regData[4]   Permanent register.
//     regData[5]   Permanent register.
//     regData[6]   Permanent register.
//     regData[7]   Permanent register. Thumb instruction set frame pointer.
//     regData[8]   Permanent register.
//     regData[9]   Permanent register. Has platform-specific uses. On iOS it's reserved for the OS.
//     regData[10]  Permanent register. SL (Stack limit, in some uses)
//     regData[11]  Permanent register. ARM instruction set frame pointer, except for Apple/iOS where it's general purpose.
//     regData[12]  Permanent register. IP (Intra-procedure-call scratch register)
//     regData[13]  Permanent register. SP (Stack pointer)
//     regData[14]  Permanent register. LR (Link register)
//     regData[15]  Permanent register. PC (Program Counter)

struct UnwState
{
    RegData                 regData[16];            // Other register (e.g. floating point) are irrelevant.
    MemData                 memData;                // Memory tracking data.
    const UnwindCallbacks*  cb;                     // Pointer to the callback functions.
    const void*             reportData;             // Pointer to pass to the report function.
    int                     instructionCount;       // Up to UNW_MAX_INSTR_COUNT
    AddressCountMap         addressCountMap;        // Keeps track of how often we've executed certain instructions, for the purpose of avoiding infinite loops.
    BranchStrategy          branchStrategy;         // Tells how we deal with conditional and unconditional branches.
    uint32_t                prevAddr;               // Previous instruction's address. Useful for looking back at while on the next instruction. Usually this is one instruction previous to the current, but not if the previous instruction was a branch.
    uint32_t                prevBranchDest;         // If the previous instruction was a branch, this is the branch address. Else 0.

    UnwState()
    {
        // regData
        // memData
        cb = NULL;
        reportData = NULL;
        instructionCount = 0;
        // addressCountArray
        branchStrategy = kBSIgnoreConditionalBranches;
        prevAddr = 0;
        prevBranchDest = 0;
    }

    void ResetBranchStrategy()
    {
        branchStrategy = kBSIgnoreConditionalBranches;
        addressCountMap.clear();
        instructionCount = 0;
    }

    void ResetInstructionCount()
    {
        instructionCount = 0;
    }

    uint32_t IncrementAddressCount(uint32_t addr)
    {
        return ++addressCountMap[addr];
    }
};


// Example structure for holding unwind results.
struct CliStack
{
    uint16_t frameCount;
    uint32_t address[64];
};




#endif // Header include guard
