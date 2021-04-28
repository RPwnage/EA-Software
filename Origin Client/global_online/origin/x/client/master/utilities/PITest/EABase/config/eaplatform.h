/*-----------------------------------------------------------------------------
 * config/eaplatform.h
 *
 * Copyright (c) 2002 - 2005 Electronic Arts Inc. All rights reserved.
 * Maintained by Paul Pedriana, Maxis
 *
 *-----------------------------------------------------------------------------
 * Currently supported platform indentification defines include:
 *    EA_PLATFORM_PLAYSTATION2
 *    EA_PLATFORM_PLAYSTATION2_IOP
 *    EA_PLATFORM_PS3
 *    EA_PLATFORM_PS3_PPU
 *    EA_PLATFORM_PS3_SPU
 *    EA_PLATFORM_PSP
 *    EA_PLATFORM_PSP2
 *    EA_PLATFORM_CAFE (a.k.a. Wii U)
 *    EA_PLATFORM_REVOLUTION (a.k.a. Wii)
 *    EA_PLATFORM_GAMECUBE
 *    EA_PLATFORM_DS
 *    EA_PLATFORM_CTR
 *    EA_PLATFORM_XBOX
 *    EA_PLATFORM_XENON (a.k.a. XBox 360)
 *    EA_PLATFORM_ANDROID
 *    EA_PLATFORM_BADA
 *    EA_PLATFORM_BADA_SIMULATOR
 *    EA_PLATFORM_APPLE
 *    EA_PLATFORM_IPHONE
 *    EA_PLATFORM_IPHONE_SIMULATOR
 *    EA_PLATFORM_PALM
 *    EA_PLATFORM_PLAYBOOK
 *    EA_PLATFORM_AIRPLAY
 *    EA_PLATFORM_OSX
 *    EA_PLATFORM_LINUX
 *    EA_PLATFORM_FREEBOX
 *    EA_PLATFORM_SAMSUNG_TV
 *    EA_PLATFORM_WINDOWS
 *    EA_PLATFORM_WIN32
 *    EA_PLATFORM_WIN64
 *    EA_PLATFORM_WINCE
 *    EA_PLATFORM_WINDOWS_PHONE
 *    EA_PLATFORM_WINRT
 *    EA_PLATFORM_SUN
 *    EA_PLATFORM_LRB (Larrabee)
 *    EA_PLATFORM_POSIX     (pseudo-platform; may be defined along with another platform like EA_PLATFORM_LINUX, EA_PLATFORM_UNIX, EA_PLATFORM_QNX)
 *    EA_PLATFORM_UNIX      (pseudo-platform; may be defined along with another platform like EA_PLATFORM_LINUX)
 *    EA_PLATFORM_CYGWIN    (pseudo-platform; may be defined along with another platform like EA_PLATFORM_LINUX)
 *    EA_PLATFORM_MINGW     (pseudo-platform; may be defined along with another platform like EA_PLATFORM_WINDOWS)
 *    EA_PLATFORM_MICROSOFT (pseudo-platform; may be defined along with another platform like EA_PLATFORM_WINDOWS)
 *
 *    EA_ABI_ARM_LINUX      (a.k.a. "eabi". for all platforms that use the CodeSourcery GNU/Linux toolchain, like Android/Palm Pre/Bada/Airplay)
 *    EA_ABI_ARM_APPLE      (similar to eabi but not identical)
 *    EA_ABI_ARM_WINCE      (similar to eabi but not identical)
 *
 * Other definitions emanated from this file inclue:
 *    EA_PLATFORM_NAME = <string>
 *    EA_PLATFORM_DESCRIPTION = <string>
 *    EA_PROCESSOR_XXX
 *    EA_MISALIGNED_SUPPORT_LEVEL=0|1|2
 *    EA_SYSTEM_LITTLE_ENDIAN | EA_SYSTEM_BIG_ENDIAN
 *    EA_ASM_STYLE_ATT | EA_ASM_STYLE_INTEL | EA_ASM_STYLE_MOTOROLA
 *    EA_PLATFORM_PTR_SIZE = <integer size in bytes>
 *    EA_PLATFORM_WORD_SIZE = <integer size in bytes>
 *
 *---------------------------------------------------------------------------*/

/*
    EA_PLATFORM_MOBILE
    EA_PLATFORM_MOBILE is a peer to EA_PLATORM_DESKTOP and EA_PLATFORM_CONSOLE. Their definition is qualitative rather
    than quantitative, and refers to the general (usually weaker) capabilities of the machine. Mobile devices have a
    similar set of weaknesses that are useful to generally categorize. The primary motivation is to avoid code that
    tests for multiple mobile platforms on a line and needs to be updated every time we get a new one.
    For example, mobile platforms tend to have weaker ARM processors, don't have full multiple processor support,
    are hand-held, don't have mice (though may have touch screens or basic cursor controls), have writable solid
    state permanent storage. Production user code shouldn't have too many expecations about the meaning of this define.

    EA_PLATFORM_DESKTOP
    This is similar to EA_PLATFORM_MOBILE in its qualitative nature and refers to platforms that are powerful.
    For example, they nearly always have virtual memory, mapped memory, hundreds of GB of writable disk sto rage,
    TCP/IP network connections, mice, keyboards, 512+ MB of RAM, multiprocessing, multiple display support.
    Production user code shouldn't have too many expecations about the meaning of this define.

    EA_PLATFORM_CONSOLE
    This is similar to EA_PLATFORM_MOBILE in its qualitative nature and refers to platforms that are consoles.
    This means platforms that are connected to TVs, are fairly powerful (especially graphics-wise), are tightly
    controlled by vendors, tend not to have mapped memory, tend to have TCP/IP, don't have multiple process support
    though they might have multiple CPUs, support TV output only. Production user code shouldn't have too many
    expecations about the meaning of this define.

*/


#ifndef INCLUDED_eaplatform_H
#define INCLUDED_eaplatform_H


// Cygwin
// This is a pseudo-platform which will be defined along with EA_PLATFORM_LINUX when
// using the Cygwin build environment.
#if defined(__CYGWIN__)
    #define EA_PLATFORM_CYGWIN 1
    #define EA_PLATFORM_DESKTOP 1
#endif

// MinGW
// This is a pseudo-platform which will be defined along with EA_PLATFORM_WINDOWS when
// using the MinGW Windows build environment.
#if defined(__MINGW32__) || defined(__MINGW64__)
    #define EA_PLATFORM_MINGW 1
    #define EA_PLATFORM_DESKTOP 1
#endif

// PlayStation 2
// __R5900 is defined by the GCC compiler and is only defined for PS2 processors.
// __MWERKS__ and __MIPS__ are defined by the Metrowerks compiler.
#if defined(EA_PLATFORM_PLAYSTATION2) || defined(EA_PLATFORM_PS2) || defined(__R5900) || defined(__R5900__)
    #undef  EA_PLATFORM_PLAYSTATION2
    #define EA_PLATFORM_PLAYSTATION2 1
    #define EA_PLATFORM_NAME "PS2"
    #define EA_PROCESSOR_R5900 1
    #define EA_PROCESSOR_MIPS 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "PS2 on R5900"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_SONY 1

// PlayStation 2 IOP (IO Processor)
// _R3000 and/or R3000 is defined by PS1 and PS2 IP Processor compilers.
// The GNU PSP compiler also predefines _R3000, so we check that __psp__
// is not also defined.
#elif defined(EA_PLATFORM_PLAYSTATION2_IOP) || ((defined(_R3000) || defined(R3000)) && !defined(__psp__))
    #undef  EA_PLATFORM_PLAYSTATION2_IOP
    #define EA_PLATFORM_PLAYSTATION2_IOP 1
    #define EA_PLATFORM_NAME "PS2 IOP"
    #define EA_PROCESSOR_R3000 1
    #define EA_PROCESSOR_MIPS 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "PS2 IOP on R3000"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_SONY 1

// PlayStation 3 PPU (Primary Processing Unit)
#elif defined(EA_PLATFORM_PS3_PPU) || defined(EA_PLATFORM_PS3) || defined(__PU__) || defined(__PPU__)
    #undef  EA_PLATFORM_PS3_PPU
    #define EA_PLATFORM_PS3_PPU 1
    #undef  EA_PLATFORM_PS3
    #define EA_PLATFORM_PS3 1
    #define EA_PLATFORM_NAME "PS3"
    #define EA_PROCESSOR_POWERPC 1
    #define EA_PROCESSOR_POWERPC_64 1
    #define EA_SYSTEM_BIG_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "PS3 on PowerPC"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_SONY 1

// PlayStation 3 SPU (Synergistic Processing Unit)
#elif defined(EA_PLATFORM_PS3_SPU) || defined(__SPU__)
    #undef  EA_PLATFORM_PS3_SPU
    #define EA_PLATFORM_PS3_SPU 1
    #define EA_PLATFORM_NAME "PS3 SPU"
    #define EA_PROCESSOR_SPU 1
    #define EA_SYSTEM_BIG_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "PS3 SPU on SPU"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_SONY 1

// PlayStation Portable (a.k.a. PSP)
// __psp__ is defined by the GCC compiler.
// _R4000 is defined by the SN compiler.
#elif defined(EA_PLATFORM_PSP) || (defined(__GNUC__) && defined(__psp__)) || (defined(__SNC__) && defined(_R4000))
    #undef  EA_PLATFORM_PSP
    #define EA_PLATFORM_PSP 1
    #define EA_PLATFORM_NAME "PSP"
    #define EA_PROCESSOR_MIPS32 1
    #define EA_PROCESSOR_MIPS 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "PSP on MIPS32"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_SONY 1

// PlayStation Vita (a.k.a. PSP2)
#elif defined(EA_PLATFORM_PSP2) || (defined(__SNC__) && defined(__psp2__))
    #undef  EA_PLATFORM_PSP2
    #define EA_PLATFORM_PSP2 1
    #define EA_PLATFORM_NAME "PSVita"
    #define EA_PROCESSOR_ARM 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "PSVita on ARM"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_SONY 1

// Cafe (Nintendo Wii 2)
// We don't currently have a better way of identifying this platform other than using __ghs__ and __ppc__.
#elif defined(EA_PLATFORM_CAFE) || (defined(__ghs__) && defined(__ppc__))
    #undef  EA_PLATFORM_CAFE
    #define EA_PLATFORM_CAFE 1
    #define EA_PLATFORM_NAME "Cafe"
    #define EA_PROCESSOR_POWERPC_32 1
    #define EA_PROCESSOR_POWERPC 1
    #define EA_SYSTEM_BIG_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "Cafe on PowerPC"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_NINTENDO 1 // Identifies the GameCube/Wii/Cafe OS API family.

// Revolution (Nintendo Wii)
// There is no way to automatically detect the Wii uniquely from GameCube until
// CodeWarrior 2.4 for Wii. For earlier compiler versions you may need to manually
// define EA_PLATFORM_REVOLUTION in your project settings.
#elif defined(EA_PLATFORM_REVOLUTION) || defined(__PPCBROADWAY__)
    #undef  EA_PLATFORM_REVOLUTION
    #define EA_PLATFORM_REVOLUTION 1
    #define EA_PLATFORM_NAME "Revolution"
    #define EA_PROCESSOR_POWERPC_GEKKO 1
    #define EA_PROCESSOR_POWERPC_32 1
    #define EA_PROCESSOR_POWERPC 1
    #define EA_SYSTEM_BIG_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "Revolution on PowerPC"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_NINTENDO 1

// GameCube
// __SN__ and __PPC__ are defined by the SN compiler.
// __MWERKS__ and __PPCGEKKO__ are defined by the Metrowerks compiler.
#elif defined(EA_PLATFORM_GAMECUBE) || ((defined(__SN__) && defined(__PPC__)) || (defined(__MWERKS__) && defined(__PPCGEKKO__)))
    #undef  EA_PLATFORM_GAMECUBE
    #define EA_PLATFORM_GAMECUBE 1
    #define EA_PLATFORM_NAME "GameCube"
    #define EA_PROCESSOR_POWERPC_GEKKO 1
    #define EA_PROCESSOR_POWERPC_32 1
    #define EA_PROCESSOR_POWERPC 1
    #define EA_SYSTEM_BIG_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "GameCube on PowerPC"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_NINTENDO 1

// Nintendo DS
// __MWERKS__ are defined by the Metrowerks compiler.
#elif defined(EA_PLATFORM_DS) || (defined(__MWERKS__) && defined(SDK_ARM9))
    #undef  EA_PLATFORM_DS
    #define EA_PLATFORM_DS 1
    #define EA_PLATFORM_NAME "NDS"
    #define EA_PROCESSOR_ARM 1
    #define EA_PROCESSOR_ARM7 1
    #define EA_PROCESSOR_ARM9 1
    #define EA_SYSTEM_BIG_ENDIAN // This is configurable on the arm processors, but big-endian for this platform.
    #define EA_PLATFORM_DESCRIPTION "NDS on ARM"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_NINTENDO 1

// Nintendo CTR (Nintendo 3DS)
// __ARMCC_VERSION is defined by the RVCT For Nintendo compiler.
#elif defined(EA_PLATFORM_CTR) || (defined(__ARMCC_VERSION) && defined(__TARGET_CPU_MPCORE))
    #undef  EA_PLATFORM_CTR
    #define EA_PLATFORM_CTR 1
    #define EA_PLATFORM_NAME "CTR"
    #define EA_PROCESSOR_ARM 1
    #define EA_PROCESSOR_ARM11 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "CTR on ARM"
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_NINTENDO 1

// XBox
// _XBOX is defined by the VC++ project, not the compiler. There is no way
// to tell if the compiler is compiling for XBox unless _XBOX is #defined
// in the project files or otherwise. _M_IX86 is the VC++ way of detecting
// an x86 target, which would mean XBox and not Xenon (a.k.a. XBox2).
#elif defined(EA_PLATFORM_XBOX) || (defined(_XBOX) && defined(_M_IX86))
    #undef  EA_PLATFORM_XBOX
    #define EA_PLATFORM_XBOX 1
    #define EA_PLATFORM_NAME "XBox"
    #define EA_PROCESSOR_X86 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "XBox on X86"
    #if defined(_MSC_VER) || defined(__ICL)
       #define EA_ASM_STYLE_INTEL 1
    #endif
    #define EA_PLATFORM_CONSOLE 1

// Xenon (XBox 360)
// The Xenon compiler doesn't define anything in particular to indicate that the
// target is the Xenon platform. The Xenon SDK, however, expects that XBOX and
// _XBOX are #defined, so the project build file must make sure these are defined.
// Since the Xenon compiler in fact defines _M_PPC, we can use this information
// to infer that Xenon is the target if neither _XENON nor _XBOX2 are specifically
// defined by the project build file.
#elif defined(EA_PLATFORM_XENON) || defined(_XENON) || defined(_XBOX2) || ((defined(_XBOX) || defined(XBOX)) && defined(_M_PPC))
    #undef  EA_PLATFORM_XENON
    #define EA_PLATFORM_XENON 1
    #define EA_PLATFORM_NAME "Xenon"
    #define EA_PROCESSOR_POWERPC 1
    #define EA_PROCESSOR_POWERPC_64 1
    #define EA_SYSTEM_BIG_ENDIAN 1
    #define EA_PLATFORM_DESCRIPTION "Xenon on PowerPC"
    #if defined(_MSC_VER) || defined(__ICL)
       #define EA_ASM_STYLE_INTEL 1
    #endif
    #define EA_PLATFORM_CONSOLE 1
    #define EA_PLATFORM_MICROSOFT 1

// Larrabee                                           // This part to be removed once __LRB__ is supported by the Larrabee compiler in 2009.
#elif defined(EA_PLATFORM_LRB) || defined(__LRB__) || (defined(__EDG__) && defined(__ICC) && defined(__x86_64__))
    #undef  EA_PLATFORM_LRB
    #define EA_PLATFORM_LRB         1
    #define EA_PLATFORM_NAME        "Larrabee"
    #define EA_PLATFORM_DESCRIPTION "Larrabee on LRB1"
    #define EA_PROCESSOR_X86_64 1
    #if defined(BYTE_ORDER) && (BYTE_ORDER == 4321)
        #define EA_SYSTEM_BIG_ENDIAN 1
    #else
        #define EA_SYSTEM_LITTLE_ENDIAN 1
    #endif
    #define EA_PROCESSOR_LRB 1
    #define EA_PROCESSOR_LRB1 1       // Larrabee version 1
    #define EA_ASM_STYLE_ATT 1        // Both types of asm style
    #define EA_ASM_STYLE_INTEL 1      // are supported.
    #define EA_PLATFORM_DESKTOP 1

// Android (Google phone OS)
#elif defined(EA_PLATFORM_ANDROID) || defined(__ANDROID__)
    #undef  EA_PLATFORM_ANDROID
    #define EA_PLATFORM_ANDROID 1
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_POSIX 1
    #define EA_PLATFORM_NAME "Android"
    #define EA_ASM_STYLE_ATT 1
    #if defined(__arm__)
        #define EA_ABI_ARM_LINUX 1  // a.k.a. "ARM eabi"
        #define EA_PROCESSOR_ARM 1
        #define EA_PLATFORM_DESCRIPTION "Android on ARM"
    #elif defined(__i386__)
        #define EA_PROCESSOR_X86 1
        #define EA_PLATFORM_DESCRIPTION "Android on x86"
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #define EA_SYSTEM_LITTLE_ENDIAN 1
    #endif
    #define EA_PLATFORM_MOBILE 1

// Palm OS for Mobile (Linux variant)
#elif defined(EA_PLATFORM_PALM)
    #undef  EA_PLATFORM_PALM
    #define EA_PLATFORM_PALM 1
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_POSIX 1
    #define EA_PLATFORM_NAME "Palm"
    #define EA_POSIX_THREADS_AVAILABLE 1
    #define EA_ASM_STYLE_ATT 1
    #if defined(__arm__)
        #define EA_ABI_ARM_LINUX 1 // a.k.a. "ARM eabi"
        #define EA_PROCESSOR_ARM 1
        #define EA_PLATFORM_DESCRIPTION "Palm on ARM"
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #define EA_SYSTEM_LITTLE_ENDIAN 1
    #endif
    #define EA_PLATFORM_MOBILE 1

// Airplay
#elif defined(EA_PLATFORM_AIRPLAY) || defined(__S3E__)
    #undef  EA_PLATFORM_AIRPLAY
    #define EA_PLATFORM_AIRPLAY 1
    #define EA_PLATFORM_NAME "Airplay"
    #if defined(__arm__)
        #define EA_ABI_ARM_LINUX 1 // a.k.a. "ARM eabi"
        #define EA_PROCESSOR_ARM 1
        #define EA_PLATFORM_DESCRIPTION "Airplay on ARM"
        #define EA_ASM_STYLE_ATT 1
        #undef _MSC_VER
    #elif defined(_M_IX86)
        #define EA_PROCESSOR_X86 1
        #define EA_PLATFORM_DESCRIPTION "Airplay on x86"
        #define EA_ASM_STYLE_INTEL 1
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #if defined(HAVE_BIG_ENDIAN) || (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && (__BYTE_ORDER == __BIG_ENDIAN))
            #define EA_SYSTEM_BIG_ENDIAN 1
        #else
            #define EA_SYSTEM_LITTLE_ENDIAN 1
        #endif
    #endif
    #define EA_PLATFORM_MOBILE 1

// Samsung Bada OS for Mobile (Linux variant)
#elif defined(EA_PLATFORM_BADA)
    #undef  EA_PLATFORM_BADA
    #define EA_PLATFORM_BADA 1
  //#define EA_PLATFORM_LINUX 1     // The underlying OS is Linux, but the app mostly doesn't see this.
  //#define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_NAME "bada"
    #define EA_ASM_STYLE_ATT 1
    #if defined(__arm__)
        #define EA_ABI_ARM_LINUX 1 // a.k.a. "ARM eabi"
        #define EA_PROCESSOR_ARM 1
        #define EA_PLATFORM_DESCRIPTION "bada on ARM"
    #elif defined(__i386__)
        #define EA_PLATFORM_BADA_SIMULATOR 1
        #define EA_PROCESSOR_X86 1
        #define EA_PLATFORM_DESCRIPTION "bada simulator on x86"
    #else
        #error Unknown processor
    #endif
    #if !defined(EA_SYSTEM_BIG_ENDIAN) && !defined(EA_SYSTEM_LITTLE_ENDIAN)
        #define EA_SYSTEM_LITTLE_ENDIAN 1
    #endif
    #define EA_PLATFORM_MOBILE 1

// Freebox
// This is a Linux-based console-like computer that's used in Europe to augment TV functionality.
// It's not explicitly a gaming device but can be used as such to some degree.
#elif defined(EA_PLATFORM_FREEBOX)
    #undef  EA_PLATFORM_FREEBOX
    #define EA_PLATFORM_FREEBOX 1
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_POSIX 1
    #define EA_PLATFORM_NAME "Freebox"
    #define EA_PLATFORM_DESCRIPTION "Freebox on x86 Atom"
    #define EA_ASM_STYLE_ATT 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PROCESSOR_X86 1 // Atom processor.

// Samsung SMART TV - a Linux-based smart TV
#elif defined(EA_PLATFORM_SAMSUNG_TV)
    #undef  EA_PLATFORM_SAMSUNG_TV
    #define EA_PLATFORM_SAMSUNG_TV 1
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_POSIX 1
    #define EA_PLATFORM_NAME "SamsungTV"
    #define EA_PLATFORM_DESCRIPTION "Samsung SMART TV on ARM"
    #define EA_ASM_STYLE_ATT 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #define EA_PROCESSOR_ARM 1
    #define EA_ABI_ARM_LINUX 1 // a.k.a. "ARM eabi"
    #define EA_PROCESSOR_ARM7 1

#elif defined(__APPLE__) && __APPLE__
    #include <TargetConditionals.h>

    // Apple family of operating systems.
    #define EA_PLATFORM_APPLE
    #define EA_PLATFORM_POSIX 1

    // iPhone
    // TARGET_OS_IPHONE will be undefined on an unknown compiler, and will be defined on gcc.
    #if defined(EA_PLATFORM_IPHONE) || defined(__IPHONE__) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR)
        #undef  EA_PLATFORM_IPHONE
        #define EA_PLATFORM_IPHONE 1
        #define EA_PLATFORM_NAME "iPhone"
        #define EA_ASM_STYLE_ATT 1
        #define EA_POSIX_THREADS_AVAILABLE 1
        #if defined(__arm__)
            #define EA_ABI_ARM_APPLE 1
            #define EA_PROCESSOR_ARM 1
            #define EA_SYSTEM_LITTLE_ENDIAN 1
            #define EA_PLATFORM_DESCRIPTION "iPhone on ARM"
        #elif defined(__i386__)
            #define EA_PLATFORM_IPHONE_SIMULATOR 1
            #define EA_PROCESSOR_X86 1
            #define EA_SYSTEM_LITTLE_ENDIAN 1
            #define EA_PLATFORM_DESCRIPTION "iPhone simulator on x86"
        #else
            #error Unknown processor
        #endif
        #define EA_PLATFORM_MOBILE 1

    // Macintosh OSX
    // TARGET_OS_MAC is defined by the Metrowerks and older AppleC compilers.
    // Howerver, TARGET_OS_MAC is defined to be 1 in all cases.
    // __i386__ and __intel__ are defined by the GCC compiler.
    // __dest_os is defined by the Metrowerks compiler.
    // __MACH__ is defined by the Metrowerks and GCC compilers.
    // powerc and __powerc are defined by the Metrowerks and GCC compilers.
    #elif defined(EA_PLATFORM_OSX) || defined(__MACH__) || (defined(__MSL__) && (__dest_os == __mac_os_x))
        #undef  EA_PLATFORM_OSX
        #define EA_PLATFORM_OSX 1
        #define EA_PLATFORM_UNIX 1
        #define EA_PLATFORM_POSIX 1
      //#define EA_PLATFORM_BSD 1           We don't currently define this. OSX has some BSD history but a lot of the API is different.
        #define EA_PLATFORM_NAME "OSX"
        #if defined(__i386__) || defined(__intel__)
            #define EA_PROCESSOR_X86 1
            #define EA_SYSTEM_LITTLE_ENDIAN 1
            #define EA_PLATFORM_DESCRIPTION "OSX on x86"
        #elif defined(__x86_64) || defined(__amd64)
            #define EA_PROCESSOR_X86_64 1
            #define EA_SYSTEM_LITTLE_ENDIAN 1
            #define EA_PLATFORM_DESCRIPTION "OSX on x86-64"
        #elif defined(__arm__)
            #define EA_ABI_ARM_APPLE 1
            #define EA_PROCESSOR_ARM 1
            #define EA_SYSTEM_LITTLE_ENDIAN 1
            #define EA_PLATFORM_DESCRIPTION "OSX on ARM"
        #elif defined(__POWERPC64__) || defined(__powerpc64__)
            #define EA_PROCESSOR_POWERPC 1
            #define EA_PROCESSOR_POWERPC_64 1
            #define EA_SYSTEM_BIG_ENDIAN 1
            #define EA_PLATFORM_DESCRIPTION "OSX on PowerPC 64"
        #elif defined(__POWERPC__) || defined(__powerpc__)
            #define EA_PROCESSOR_POWERPC 1
            #define EA_PROCESSOR_POWERPC_32 1
            #define EA_SYSTEM_BIG_ENDIAN 1
            #define EA_PLATFORM_DESCRIPTION "OSX on PowerPC"
        #else
            #error Unknown processor
        #endif
        #if defined(__GNUC__)
            #define EA_ASM_STYLE_ATT 1
        #else
            #define EA_ASM_STYLE_MOTOROLA 1
        #endif
        #define EA_PLATFORM_DESKTOP 1
    #else
        #error Unknown Apple Platform
    #endif

// Linux
// __linux and __linux__ are defined by the GCC and Borland compiler.
// __i386__ and __intel__ are defined by the GCC compiler.
// __i386__ is defined by the Metrowerks compiler.
// _M_IX86 is defined by the Borland compiler.
// __sparc__ is defined by the GCC compiler.
// __powerpc__ is defined by the GCC compiler.
#elif defined(EA_PLATFORM_LINUX) || (defined(__linux) || defined(__linux__))
    #undef  EA_PLATFORM_LINUX
    #define EA_PLATFORM_LINUX 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_POSIX 1
    #define EA_PLATFORM_NAME "Linux"
    #if defined(__i386__) || defined(__intel__) || defined(_M_IX86)
        #define EA_PROCESSOR_X86 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Linux on x86"
    #elif defined(__x86_64__)
        #define EA_PROCESSOR_X86_64 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Linux on x86-64"
    #elif defined(__powerpc64__)
        #define EA_PROCESSOR_POWERPC 1
        #define EA_PROCESSOR_POWERPC_64 1
        #define EA_SYSTEM_BIG_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Linux on PowerPC 64"
    #elif defined(__powerpc__)
        #define EA_PROCESSOR_POWERPC 1
        #define EA_PROCESSOR_POWERPC_32 1
        #define EA_SYSTEM_BIG_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Linux on PowerPC"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if defined(__GNUC__)
        #define EA_ASM_STYLE_ATT 1
    #endif
    #define EA_PLATFORM_DESKTOP 1


#elif defined(EA_PLATFORM_BSD) || (defined(__BSD__) || defined(__FreeBSD__))
    #undef  EA_PLATFORM_BSD
    #define EA_PLATFORM_BSD 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_POSIX 1     // BSD's posix complaince is not identical to Linux's
    #define EA_PLATFORM_NAME "BSD Unix"
    #if defined(__i386__) || defined(__intel__)
        #define EA_PROCESSOR_X86 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "BSD on x86"
    #elif defined(__x86_64__)
        #define EA_PROCESSOR_X86_64 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "BSD on x86-64"
    #elif defined(__powerpc64__)
        #define EA_PROCESSOR_POWERPC 1
        #define EA_PROCESSOR_POWERPC_64 1
        #define EA_SYSTEM_BIG_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "BSD on PowerPC 64"
    #elif defined(__powerpc__)
        #define EA_PROCESSOR_POWERPC 1
        #define EA_PROCESSOR_POWERPC_32 1
        #define EA_SYSTEM_BIG_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "BSD on PowerPC"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if !defined(EA_PLATFORM_FREEBSD) && defined(__FreeBSD__)
        #define EA_PLATFORM_FREEBSD 1 // This is a variation of BSD.
    #endif
    #if defined(__GNUC__)
        #define EA_ASM_STYLE_ATT 1
    #endif
    #define EA_PLATFORM_DESKTOP 1


// Win CE (Windows mobile)
#elif defined(EA_PLATFORM_WINCE) || defined(_WIN32_WCE)
    #undef  EA_PLATFORM_WINCE
    #define EA_PLATFORM_WINCE 1
    #define EA_PLATFORM_NAME "WinCE"
    #define EA_ASM_STYLE_INTEL 1
    #define EA_SYSTEM_LITTLE_ENDIAN 1
    #if defined(_M_ARM) // Also there is _M_ARMT
        #define EA_ABI_ARM_WINCE 1
        #define EA_PROCESSOR_ARM 1
        #define EA_PLATFORM_DESCRIPTION "Windows CE on ARM"
    #elif defined(_M_IX86)
        #define EA_PROCESSOR_X86 1
        #define EA_PLATFORM_DESCRIPTION "Windows CE on X86"
    #else //Possibly other Windows CE variants
        #error Unknown processor
    #endif
    #define EA_PLATFORM_MOBILE 1

#elif defined(EA_PLATFORM_WINDOWS_PHONE)
    #undef EA_PLATFORM_WINDOWS_PHONE
    #define EA_PLATFORM_WINDOWS_PHONE 1
    #define EA_PLATFORM_NAME "Windows Phone"
    #if defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
        #define EA_PROCESSOR_X86_64 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Windows Phone on X86-64"
    #elif defined(_M_IX86) || defined(_X86_)
        #define EA_PROCESSOR_X86 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Windows Phone on X86"
    #elif defined(_M_ARM)
        #define EA_ABI_ARM_WINCE 1
        #define EA_PROCESSOR_ARM 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Windows Phone on ARM"
    #else //Possibly other Windows Phone variants
        #error Unknown processor
        #error Unknown endianness
    #endif
    #define EA_PLATFORM_MICROSOFT 1

    // WINAPI_FAMILY defines - mirrored from winapifamily.h
    #define EA_WINAPI_FAMILY_APP         1
    #define EA_WINAPI_FAMILY_DESKTOP_APP 2
    #define EA_WINAPI_FAMILY_PHONE_APP   3

    #if defined(WINAPI_FAMILY)
        #include <winapifamily.h>
        #if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
            #define EA_WINAPI_FAMILY EA_WINAPI_FAMILY_PHONE_APP
        #else
            #error Unsupported WINAPI_FAMILY for Windows Phone
        #endif
    #else
        #error WINAPI_FAMILY should always be defined on Windows Phone.
    #endif

    // Macro to determine if a partition is enabled.
    #define EA_WINAPI_FAMILY_PARTITION(Partition)   (Partition)

    // Enable the appropriate partitions for the current family
    #if EA_WINAPI_FAMILY == EA_WINAPI_FAMILY_PHONE_APP
    #   define EA_WINAPI_PARTITION_CORE    1
    #   define EA_WINAPI_PARTITION_PHONE   1
    #   define EA_WINAPI_PARTITION_APP     1
    #else
    #   error Unsupported WINAPI_FAMILY for Windows Phone
    #endif


// Windows
// _WIN32 is defined by the VC++, Intel and GCC compilers.
// _WIN64 is defined by the VC++, Intel and GCC compilers.
// __WIN32__ is defined by the Borland compiler.
// __INTEL__ is defined by the Metrowerks compiler.
// _M_IX86, _M_AMD64 and _M_IA64 are defined by the VC++, Intel, and Borland compilers.
// _X86_, _AMD64_, and _IA64_ are defined by the Metrowerks compiler.
// _M_ARM is defined by the VC++ compiler.
#elif (defined(EA_PLATFORM_WINDOWS) || (defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || (defined(__MWERKS__) && defined(_X86_)))) && !defined(_XBOX)
    #undef  EA_PLATFORM_WINDOWS
    #define EA_PLATFORM_WINDOWS 1
    #define EA_PLATFORM_NAME "Windows"
    #ifdef _WIN64 // VC++ defines both _WIN32 and _WIN64 when compiling for Win64.
        #define EA_PLATFORM_WIN64 1
    #else
        #define EA_PLATFORM_WIN32 1
    #endif
    #if defined(_M_AMD64) || defined(_AMD64_) || defined(__x86_64__)
        #define EA_PROCESSOR_X86_64 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Windows on X86-64"
    #elif defined(_M_IX86) || defined(_X86_)
        #define EA_PROCESSOR_X86 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Windows on X86"
    #elif defined(_M_IA64) || defined(_IA64_)
        #define EA_PROCESSOR_IA64 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Windows on IA-64"
    #elif defined(_M_ARM)
        #define EA_ABI_ARM_WINCE 1
        #define EA_PROCESSOR_ARM 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Windows on ARM"
    #else //Possibly other Windows CE variants
        #error Unknown processor
        #error Unknown endianness
    #endif
    #if defined(__GNUC__)
        #define EA_ASM_STYLE_ATT 1
    #elif defined(_MSC_VER) || defined(__BORLANDC__) || defined(__ICL)
        #define EA_ASM_STYLE_INTEL 1
    #endif
    #define EA_PLATFORM_DESKTOP 1
    #define EA_PLATFORM_MICROSOFT 1

    // WINAPI_FAMILY defines to support Windows 8 Metro Apps - mirroring winapifamily.h in the Windows 8 SDK
    #define EA_WINAPI_PARTITION_DESKTOP   0x00000001
    #define EA_WINAPI_PARTITION_APP       0x00000002
    #define EA_WINAPI_FAMILY_APP          EA_WINAPI_PARTITION_APP
    #define EA_WINAPI_FAMILY_DESKTOP_APP  (EA_WINAPI_PARTITION_DESKTOP | EA_WINAPI_PARTITION_APP)

    #if defined(WINAPI_FAMILY)
        #if defined(_MSC_VER)
            #pragma warning(push, 0)
        #endif
        #include <winapifamily.h>
        #if defined(_MSC_VER)
            #pragma warning(pop)
        #endif
        #if WINAPI_FAMILY == WINAPI_FAMILY_DESKTOP_APP
            #define EA_WINAPI_FAMILY EA_WINAPI_FAMILY_DESKTOP_APP
        #elif WINAPI_FAMILY == WINAPI_FAMILY_APP
            #define EA_WINAPI_FAMILY EA_WINAPI_FAMILY_APP
        #else
            #error Unsupported WINAPI_FAMILY
        #endif
    #else
        #define EA_WINAPI_FAMILY EA_WINAPI_FAMILY_DESKTOP_APP
    #endif

    #define EA_WINAPI_FAMILY_PARTITION(Partition)   ((EA_WINAPI_FAMILY & Partition) == Partition)

    // EA_PLATFORM_WINRT
    // This is a subset of Windows which is used for tablets and the "Metro" (restricted) Windows user interface.
    // WinRT doesn't doesn't have access to the Windows "desktop" API, but WinRT can nevertheless run on 
    // desktop computers in addition to tablets. The Windows Phone API is a subset of WinRT and is not included
    // in it due to it being only a part of the API.
    #if (defined(EA_PLATFORM_WINDOWS) && !EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP))
        #define EA_PLATFORM_WINRT 1 
    #endif

// Sun (Solaris)
// __SUNPRO_CC is defined by the Sun compiler.
// __sun is defined by the GCC compiler.
// __i386 is defined by the Sun and GCC compilers.
// __sparc is defined by the Sun and GCC compilers.
#elif defined(EA_PLATFORM_SUN) || (defined(__SUNPRO_CC) || defined(__sun))
    #undef  EA_PLATFORM_SUN
    #define EA_PLATFORM_SUN 1
    #define EA_PLATFORM_UNIX 1
    #define EA_PLATFORM_POSIX 1
    #define EA_PLATFORM_NAME "SUN"
    #if defined(__i386)
        #define EA_PROCESSOR_X86 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "SUN on x86"
    #elif defined(__sparc)
        #define EA_PROCESSOR_SPARC 1
        #define EA_SYSTEM_BIG_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "SUN on Sparc"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif
    #define EA_PLATFORM_DESKTOP 1

// Playbook (RIM Blackberry tablet)
// Runs on QNX, with GCC-based toolchain. Posix system, so far the only QNX based one
// __QNX__ is defined by the GCC compiler
#elif defined(EA_PLATFORM_PLAYBOOK) || defined(__QNX__)
    #undef  EA_PLATFORM_PLAYBOOK
    #define EA_PLATFORM_PLAYBOOK 1
    #define EA_PLATFORM_POSIX 1
    #define EA_PLATFORM_NAME "Playbook"
    #define EA_POSIX_THREADS_AVAILABLE 1

    #if defined(__X86__)
        #define EA_PROCESSOR_X86 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Playbook on x86"
        #define EA_PLATFORM_DESKTOP 1
    #elif defined(__ARM__)
        #define EA_PROCESSOR_ARM 1
        #define EA_ABI_ARM_LINUX 1  // a.k.a. "ARM eabi"
        #define EA_PROCESSOR_ARM7 1
        #define EA_SYSTEM_LITTLE_ENDIAN 1
        #define EA_PLATFORM_DESCRIPTION "Playbook on ARM"
    #else
        #error Unknown processor
        #error Unknown endianness
    #endif

    #define EA_PLATFORM_MOBILE 1
#else
    #error Unknown platform
    #error Unknown processor
    #error Unknown endianness
#endif



// EA_PLATFORM_PTR_SIZE
// Platform pointer size; same as sizeof(void*).
// This is not the same as sizeof(int), as int is usually 32 bits on
// even 64 bit platforms.
//
// _WIN64 is defined by Win64 compilers, such as VC++.
// _M_IA64 is defined by VC++ and Intel compilers for IA64 processors.
// __LP64__ is defined by HP compilers for the LP64 standard.
// _LP64 is defined by the GCC and Sun compilers for the LP64 standard.
// __ia64__ is defined by the GCC compiler for IA64 processors.
// __arch64__ is defined by the Sparc compiler for 64 bit processors.
// __mips64__ is defined by the GCC compiler for MIPS processors.
// __powerpc64__ is defined by the GCC compiler for PowerPC processors.
// __64BIT__ is defined by the AIX compiler for 64 bit processors.
// __sizeof_ptr is defined by the ARM compiler (armcc, armcpp).
//
#ifndef EA_PLATFORM_PTR_SIZE
    #if defined(__WORDSIZE) // Defined by some variations of GCC.
        #define EA_PLATFORM_PTR_SIZE ((__WORDSIZE) / 8)
    #elif defined(_WIN64) || defined(__LP64__) || defined(_LP64) || defined(_M_IA64) || defined(__ia64__) || defined(__arch64__) || defined(__mips64__) || defined(__64BIT__) || defined(__Ptr_Is_64)
        #define EA_PLATFORM_PTR_SIZE 8
    #elif defined(__CC_ARM) && (__sizeof_ptr == 8)
        #define EA_PLATFORM_PTR_SIZE 8
    #elif defined(__CC_ARM) && (__sizeof_ptr == 8)
        #define EA_PLATFORM_PTR_SIZE 8
    #else
        #define EA_PLATFORM_PTR_SIZE 4
    #endif
#endif



// EA_PLATFORM_WORD_SIZE
// This defines the size of a machine word. This will be the same as
// the size of registers on the machine but not necessarily the same
// as the size of pointers on the machine. A number of 64 bit platforms
// have 64 bit registers but 32 bit pointers.
//
#ifndef EA_PLATFORM_WORD_SIZE
   #if defined(EA_PLATFORM_XENON) || defined(EA_PLATFORM_PS3) || defined(EA_PLATFORM_PLAYSTATION2)
      #define EA_PLATFORM_WORD_SIZE 8
   #else
      #define EA_PLATFORM_WORD_SIZE EA_PLATFORM_PTR_SIZE
   #endif
#endif


// EA_MISALIGNED_SUPPORT_LEVEL
// Specifies if the processor can read and write built-in types that aren't
// naturally aligned.
//    0 - not supported. Likely causes an exception.
//    1 - supported but slow.
//    2 - supported and fast.
//
#ifndef EA_MISALIGNED_SUPPORT_LEVEL
    #if defined(EA_PROCESSOR_X64) || defined(EA_PROCESSOR_X86_64)
        #define EA_MISALIGNED_SUPPORT_LEVEL 2
    #elif defined(EA_PLATFORM_XENON)
        #define EA_MISALIGNED_SUPPORT_LEVEL 1
    #else
        #define EA_MISALIGNED_SUPPORT_LEVEL 0
    #endif
#endif

// Macro to determine if a Windows API partition is enabled. Always false on non Microsoft platforms.
#if !defined(EA_WINAPI_FAMILY_PARTITION)
    #define EA_WINAPI_FAMILY_PARTITION(Partition) (0)
#endif


#endif // INCLUDED_eaplatform_H








