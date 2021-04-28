///////////////////////////////////////////////////////////////////////////////
// Context.h
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Defines a processor context for various platforms. A processor context
// is the state of all registers and any other pertinent data that defines
// the context of a running thread.
///////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_CONTEXT_H
#define EACALLSTACK_CONTEXT_H

#include <eathread/internal/config.h>

// The implementation of the callstack fetch code and callstack context code 
// has been moved to EAThread past version 1.17.02.
// TODO: Remove old callstack implementation code by June, 2014.
#if EATHREAD_VERSION_N > 11702

    #include <eathread/eathread_callstack_context.h>

    namespace EA
    {
        namespace Callstack
        {
            using ::EA::Thread::VMXRegister;
            using ::EA::Thread::ContextXenon;
            using ::EA::Thread::ContextPS3;
            using ::EA::Thread::ContextSPU;
            using ::EA::Thread::ContextPowerPC32;
            using ::EA::Thread::ContextPowerPC64;
            using ::EA::Thread::ContextX86;
            using ::EA::Thread::ContextX86_64;
            using ::EA::Thread::ContextARM;
            using ::EA::Thread::Context;
            using ::EA::Thread::SPURegister;
            using ::EA::Thread::CallstackContextX86;
            using ::EA::Thread::CallstackContextX86_64;
            using ::EA::Thread::CallstackContextPowerPC;
            using ::EA::Thread::CallstackContextARM;
            using ::EA::Thread::CallstackContextMIPS;
            using ::EA::Thread::CallstackContextSPU;
            using ::EA::Thread::CallstackContext;

            enum CallstackContextType
            {
                CALLSTACK_CONTEXT_UNKNOWN = 0,
                CALLSTACK_CONTEXT_POWERPC,
                CALLSTACK_CONTEXT_X86,
                CALLSTACK_CONTEXT_X86_64,
                CALLSTACK_CONTEXT_ARM,
                CALLSTACK_CONTEXT_MIPS,
                CALLSTACK_CONTEXT_SPU,
                NUMBER_OF_CALLSTACK_CONTEXT_TYPES
            };

        }
    }

#else

    #include <EABase/eabase.h>
    #include <EACallstack/internal/Config.h>

    #ifdef _MSC_VER
        #pragma warning(push)
        #pragma warning(disable: 4201)
    #endif



    namespace EA
    {
    namespace Callstack
    {


    union VMXRegister
    {
        uint8_t  mByte    [16 / sizeof(uint8_t )];
        uint16_t mHalfword[16 / sizeof(uint16_t)];
        uint32_t mWord    [16 / sizeof(uint32_t)];
        uint64_t mDword   [16 / sizeof(uint64_t)];  // Some VMX implementations don't support 64 bit integers.
        float    mFloat   [16 / sizeof(float)];
        double   mDouble  [16 / sizeof(double)];    // Some VMX implementations don't support 64 bit doubles.
    };


    /// ContextXenon
    ///
    /// This is a binary equivalent of the CONTEXT struct in the Xenon WinNT.h header.
    /// It exists here so that it can be referenced via compilation on a platform
    /// other than Xenon.
    ///
    struct ContextXenon
    {
        uint32_t mContextFlags;

        uint32_t mMsr;       // Machine status register
        uint32_t mIar;       // Instruction address register
        uint32_t mLr;        // Link register
        uint64_t mCtr;       // Count register
        uint64_t mGpr[32];   // General registers 0..31
        uint32_t mCr;        // Condition register
        uint32_t mXer;       // Fixed point exception register

        double   mFpscr;     // Floating point status/control reg
        double   mFpr[32];   // Floating registers 0..31

        uint32_t mUserModeControl;   // User mode control state
        uint32_t mFill;

        VMXRegister mVscr;       // Vector status/control register
        VMXRegister mVr[128];    // Vector registers 0..127

    }; // ContextXenon

#ifdef EA_PLATFORM_XENON
        struct Context : public ContextXenon
        {
            // Empty
        };
#endif



    /// ContextPS3
    ///
    /// This is equivalent to the PS3 sys_dbg_ppu_thread_context_t struct.
    ///
    struct ContextPS3
    {
        uint64_t      mGpr[32];      // General Purpose Register GPR[0-31]
        uint32_t      mCr;           // Condition Register
        uint64_t      mXer;          // Fixed Point Exception Register
        uint64_t      mLr;           // Link Register
        uint64_t      mCtr;          // Control Register
        uint64_t      mIar;          // Program Counter
        double        mFpr[32];      // Floating Point Register FPR[0-31]
        uint32_t      mFpscr;        // Floating Point Status and Control Register
        VMXRegister   mVr[32];       // Vector Register VR[0-31]
        VMXRegister   mVscr;         // Vector Status and Control Register

    }; // ContextPowerPC

#ifdef EA_PLATFORM_PS3
        struct Context : public ContextPS3
        {
            // Empty
        };
#endif





    /// SPURegister
    union SPURegister
    {
        uint8_t  mByte    [16 / sizeof(uint8_t )];
        uint16_t mHalfword[16 / sizeof(uint16_t)];
        uint32_t mWord    [16 / sizeof(uint32_t)];
        uint64_t mDword   [16 / sizeof(uint64_t)];
        float    mFloat   [16 / sizeof(float)];
        double   mDouble  [16 / sizeof(double)];
    };


    /// ContextSPU
    ///
    /// Context for an IBM/Sony SPU processor.
    ///
    struct ContextSPU
    {
        SPURegister mGpr[128];      // GPR[0-127]
        uint32_t    mNpc;           // Next Program Counter
        uint32_t    mFpscr;         // Floating point status control register
        uint32_t    mSrr0;          // SRR0
        uint32_t    mSPUStatus;     // SPU STATUS
        uint64_t    mSPUCfg;        // SPU CFG
        uint32_t    mMBStat;        // MB_stat
        uint32_t    mPPUMb;         // PPU Mail Box
        uint32_t    mSPUMb[4];      // SPU Mail Box
        uint32_t    mDecrementer;   // Decrementer
        uint64_t    mMfcCqSr[96];   // MFC Command Queue Save/Restore Register
    };

#ifdef EA_PLATFORM_PS3_SPU
        struct Context : public ContextSPU
        {
            // Empty
        };
#endif



/// ContextRevolution
///
/// Context for the Revolution/Wii/Cafe platform.
/// This is just like the Nintendo OSContext struct but contains some missing members.
///
struct ContextRevolution
{
    uint32_t mGpr[32];    // General registers 0..31
    uint32_t mCr;         // Condition register
    uint32_t mLr;         // Link register
    uint32_t mCtr;        // Count register low
    uint32_t mXer;        // Fixed point exception register
    double   mFpr[32];    // Floating registers 0..31
    uint32_t mFpscrPad;   // Unused
    uint32_t mFpscr;      // Floating Point Status and Control Register
    uint32_t mSrr0;       // SRR0
    uint32_t mSrr1;       // SRR1
    uint16_t mMode;       // Context mode. See Revolution/OSContext.h. #define OS_CONTEXT_MODE_FPU 0x01u   #define OS_CONTEXT_MODE_PSFP 0x02u
    uint16_t mState;      // Context state. See Revolution/OSContext.h #define OS_CONTEXT_STATE_FPSAVED 0x01u #define OS_CONTEXT_STATE_EXC 0x02u
    uint32_t mGqr[8];     // Gekko-specific registers.
    uint32_t mPsfPad;     // Gekko-specific registers.
    double   mPsf[32];    // Gekko-specific registers.

    #if defined(EA_PLATFORM_CAFE) // See KernelOSContext.h
    u32      mRrror;                // per-thread OS error.
    u16      mSpinLockCount;        // the number of spinlock that this thread is holding.
    u16      mSpinLockCount_pad;    // padding
    #endif

    // These aren't included in the Nintendo OSContext struct.
    uint32_t mIar;        // Instruction address register
    uint32_t mMsr;        // Machine status register
    uint32_t mCtrHigh;    // Count register high

    }; // ContextRevolution

#if defined(EA_PLATFORM_REVOLUTION) || defined(EA_PLATFORM_CAFE)
        struct Context : public ContextRevolution
        {
            // Empty
        };
#endif



    /// ContextGameCube
    ///
    /// Context for the GameCube platform.
    /// This is just like the Nintendo OSContext struct but contains some missing members.
    ///
    struct ContextGameCube
    {
        uint32_t mGpr[32];    // General registers 0..31
        uint32_t mCr;         // Condition register
        uint32_t mLr;         // Link register
        uint32_t mCtr;        // Count register low
        uint32_t mXer;        // Fixed point exception register
        double   mFpr[32];    // Floating registers 0..31
        uint32_t mFpscrPad;   // Unused
        uint32_t mFpscr;      // Floating Point Status and Control Register
        uint32_t mSrr0;       // SRR0
        uint32_t mSrr1;       // SRR1
        uint16_t mMode;       // Context mode. See Revolution/OSContext.h. #define OS_CONTEXT_MODE_FPU 0x01u   #define OS_CONTEXT_MODE_PSFP 0x02u
        uint16_t mState;      // Context state. See Revolution/OSContext.h #define OS_CONTEXT_STATE_FPSAVED 0x01u #define OS_CONTEXT_STATE_EXC 0x02u
        uint32_t mGqr;        // Gekko-specific registers.
        uint32_t mPsfPad;     // Gekko-specific registers.
        double   mPsf[32];    // Gekko-specific registers.

        // These aren't included in the Nintendo OSContext struct.
        uint32_t mIar;        // Instruction address register
        uint32_t mMsr;        // Machine status register
        uint32_t mCtrHigh;    // Count register high
    };

#ifdef EA_PLATFORM_GAMECUBE
        struct Context : public ContextGameCube
        {
            // Empty
        };
#endif



    /// ContextPowerPC32
    ///
    /// This is a generic 32 bit PowerPC with VMX context.
    ///
    struct ContextPowerPC32
    {
        uint32_t    mGpr[32];    // General registers 0..31
        uint32_t    mCr;         // Condition register
        uint32_t    mXer;        // Fixed point exception register
        uint32_t    mLr;         // Link register
        uint32_t    mCtr;        // Count register low
        uint32_t    mCtrHigh;    // Count register high
        uint32_t    mIar;        // Instruction address register
        uint32_t    mMsr;        // Machine status register
        double      mFpr[32];    // Floating registers 0..31
        double      mFpscr;      // Floating point status/control reg
        VMXRegister mVr[32];     // Vector registers 0..127
        VMXRegister mVscr;       // Vector status/control register

    }; // ContextPowerPC32

#if defined(EA_PROCESSOR_POWERPC) && defined(EA_PLATFORM_OSX)
        struct Context : public ContextPowerPC32
        {
            // Empty
        };
#endif



    /// ContextPowerPC64
    ///
    /// This is a generic 64 bit PowerPC with VMX context.
    ///
    struct ContextPowerPC64
    {
        uint64_t    mGpr[32];    // General registers 0..31
        uint64_t    mCr;         // Condition register
        uint64_t    mXer;        // Fixed point exception register
        uint64_t    mLr;         // Link register
        uint64_t    mCtr;        // Count register
        uint64_t    mIar;        // Instruction address register
        uint64_t    mMsr;        // Machine status register
        double      mFpr[32];    // Floating registers 0..31
        double      mFpscr;      // Floating point status/control reg
        VMXRegister mVr[32];     // Vector registers 0..127
        VMXRegister mVscr;       // Vector status/control register

    }; // ContextPowerPC64




    /// ContextX86
    ///
    /// Generic Intel x86 context.
    /// This is a duplicate of the CONTEXT structure defined by Microsoft in WinNT.h.
    ///
    struct ContextX86
    {
        uint32_t   ContextFlags;    // Used by Windows to indicate which of the following sections are valid.

        uint32_t   Dr0;
        uint32_t   Dr1;
        uint32_t   Dr2;
        uint32_t   Dr3;
        uint32_t   Dr6;
        uint32_t   Dr7;

        // FLOATING_SAVE_AREA
        uint32_t   Control;
        uint32_t   Status;
        uint32_t   Tag;
        uint32_t   ErrorOffset;
        uint32_t   ErrorSelector;
        uint32_t   DataOffset;
        uint32_t   DataSelector;
        uint8_t    RegisterArea[80];
        uint32_t   Cr0NpxState;

        uint32_t   SegGs;
        uint32_t   SegFs;
        uint32_t   SegEs;
        uint32_t   SegDs;

        uint32_t   Edi;
        uint32_t   Esi;
        uint32_t   Ebx;
        uint32_t   Edx;
        uint32_t   Ecx;
        uint32_t   Eax;

        uint32_t   Ebp;
        uint32_t   Eip;
        uint32_t   SegCs;
        uint32_t   EFlags;
        uint32_t   Esp;
        uint32_t   SegSs;

        uint8_t    ExtendedRegisters[512];

    }; // ContextX86

#ifdef EA_PROCESSOR_X86 // Win32, Linux, OSX.
        struct Context : public ContextX86
        {
            // Empty
        };
#endif



    /// ContextX86_64
    ///
    /// Generic Intel x86-64 context.
    /// This is a duplicate of the CONTEXT structure defined 
    /// by Microsoft in WinNT.h in VC8 and later.
    ///
    EA_PREFIX_ALIGN(16)
    struct M128A_
    {
        uint64_t Low;
        int64_t  High;
    }EA_POSTFIX_ALIGN(16);

    struct XMM_SAVE_AREA32_
    {
        uint16_t  ControlWord;
        uint16_t  StatusWord;
        uint8_t   TagWord;
        uint8_t   Reserved1;
        uint16_t  ErrorOpcode;
        uint32_t  ErrorOffset;
        uint16_t  ErrorSelector;
        uint16_t  Reserved2;
        uint32_t  DataOffset;
        uint16_t  DataSelector;
        uint16_t  Reserved3;
        uint32_t  MxCsr;
        uint32_t  MxCsr_Mask;
        M128A_    FloatRegisters[8];
        M128A_    XmmRegisters[16];
        uint8_t   Reserved4[96];
    };

    EA_PREFIX_ALIGN(16) struct ContextX86_64
    {
        uint64_t P1Home;
        uint64_t P2Home;
        uint64_t P3Home;
        uint64_t P4Home;
        uint64_t P5Home;
        uint64_t P6Home;

        uint32_t ContextFlags;    // Used by Windows to indicate which of the following sections are valid.
        uint32_t MxCsr;

        uint16_t SegCs;
        uint16_t SegDs;
        uint16_t SegEs;
        uint16_t SegFs;
        uint16_t SegGs;
        uint16_t SegSs;
        uint32_t EFlags;

        uint64_t Dr0;
        uint64_t Dr1;
        uint64_t Dr2;
        uint64_t Dr3;
        uint64_t Dr6;
        uint64_t Dr7;

        uint64_t Rax;
        uint64_t Rcx;
        uint64_t Rdx;
        uint64_t Rbx;
        uint64_t Rsp;
        uint64_t Rbp;
        uint64_t Rsi;
        uint64_t Rdi;
        uint64_t R8;
        uint64_t R9;
        uint64_t R10;
        uint64_t R11;
        uint64_t R12;
        uint64_t R13;
        uint64_t R14;
        uint64_t R15;

        uint64_t Rip;

        union {
            XMM_SAVE_AREA32_ FltSave;

            struct {
                M128A_ Header[2];
                M128A_ Legacy[8];
                M128A_ Xmm0;
                M128A_ Xmm1;
                M128A_ Xmm2;
                M128A_ Xmm3;
                M128A_ Xmm4;
                M128A_ Xmm5;
                M128A_ Xmm6;
                M128A_ Xmm7;
                M128A_ Xmm8;
                M128A_ Xmm9;
                M128A_ Xmm10;
                M128A_ Xmm11;
                M128A_ Xmm12;
                M128A_ Xmm13;
                M128A_ Xmm14;
                M128A_ Xmm15;
            } DUMMYSTRUCTNAME;
        } DUMMYUNIONNAME;

        M128A_   VectorRegister[26];
        uint64_t VectorControl;

        uint64_t DebugControl;
        uint64_t LastBranchToRip;
        uint64_t LastBranchFromRip;
        uint64_t LastExceptionToRip;
        uint64_t LastExceptionFromRip;

    }; // ContextX86_64

#ifdef EA_PROCESSOR_X86_64
        struct Context : public ContextX86_64
        {
            // Empty
        };
#endif




    union DoubleFloat
    {
        double   d64;
        float    f32[2];
        uint64_t u64;
        uint32_t u32[2];
    };


    /// ContextARM
    ///
    /// Generic ARM processor context.
    /// There are many variations of ARM processors, so one context can't 
    /// address them all. We assume an ARM 7 with VFPv3 here, which is the
    /// latest we use as of 2010.
    /// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0041c/ch09s02s02.html
    /// http://www.arm.com/products/processors/technologies/vector-floating-point.php
    ///
    /// mGpr[0]   Volatile register. Argument1, return value.
    /// mGpr[1]   Volatile register. Argument2, Second 32-bits if double/int Return Value
    /// mGpr[2]   Volatile register. Argument3.
    /// mGpr[3]   Volatile register. Argument4. Further arguments are put on the stack.
    /// mGpr[4]   Permanent register.
    /// mGpr[5]   Permanent register.
    /// mGpr[6]   Permanent register.
    /// mGpr[7]   Permanent register. Thumb instruction set frame pointer.
    /// mGpr[8]   Permanent register.
    /// mGpr[9]   Permanent register. Has platform-specific uses. On iOS it's reserved for the OS.
    /// mGpr[10]  Permanent register. SL (Stack limit, in some uses)
    /// mGpr[11]  Permanent register. ARM instruction set frame pointer, except for Apple/iOS where it's general purpose.
    /// mGpr[12]  Permanent register. IP (scratch register/new-sb in inter-link-unit calls)
    /// mGpr[13]  Permanent register. SP (Stack pointer)
    /// mGpr[14]  Permanent register. LR (Link register)
    /// mGpr[15]  Permanent register. PC (Program Counter)

    struct ContextARM
    {
        uint32_t    mGpr[16];           // General registers.
        uint32_t    mCpsr;              // Current program status register.
        uint32_t    mSpsr;              // Saved program status register.
        uint32_t    mFpscr;             // Floating point status condition register.
        DoubleFloat mDoubleFloat[32];   // If these are present, the device will have either 16 (VFPv3-D16) or 32 (VFPv3-D32) registers.

    }; // ContextARM

#ifdef EA_PROCESSOR_ARM
        struct Context : public ContextARM
        {
            // Empty
        };
#endif



    } // namespace Callstack

    } // namespace EA



    #ifdef _MSC_VER
        #pragma warning(pop)
    #endif

#endif

#endif // Header include guard














