///////////////////////////////////////////////////////////////////////////////
// EACallstack.h
//
// Copyright (c) 2003-2009 Electronic Arts Inc.
// Created and maintained by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#ifndef EACALLSTACK_EACALLSTACK_H
#define EACALLSTACK_EACALLSTACK_H

#include <eathread/internal/config.h>

// The implementation of the callstack fetch code and callstack context code 
// has been moved to EAThread past version 1.17.02.
// TODO: Remove old callstack implementation code by June, 2014.
#if EATHREAD_VERSION_N > 11702

	#include <eathread/eathread_callstack.h>
	#include <EACallstack/Context.h>

	namespace EA
	{
		namespace Callstack
		{
			using ::EA::Thread::InitCallstack;
			using ::EA::Thread::ShutdownCallstack;
			using ::EA::Thread::GetCallstack;
			using ::EA::Thread::GetCallstackContext;
			using ::EA::Thread::GetCallstackContextSysThreadId;
			using ::EA::Thread::GetModuleFromAddress;
			using ::EA::Thread::GetModuleHandleFromAddress;
			using ::EA::Thread::GetInstructionPointer;
			using ::EA::Thread::SetStackBase;
			using ::EA::Thread::GetStackBase;
			using ::EA::Thread::GetStackLimit;


			#if defined(EA_PLATFORM_MICROSOFT)
				using ::EA::Thread::ThreadHandlesAreEqual;
				using ::EA::Thread::GetThreadIdFromThreadHandle;
			#endif

		} // namespace Callstack
} // namespace EA


#else


	#include <EABase/eabase.h>
	#include <EACallstack/internal/Config.h>
	#include <EACallstack/internal/EASTLCoreAllocator.h>
	#include <EACallstack/EAAddressRep.h>
	#include <stddef.h>
	#include <stdio.h>


	namespace EA
	{
		namespace Callstack
		{
			/// CallstackContext
			/// 
			/// This is forward-declared here and fully declared at the bottom of this file.
			///
			#if EATHREAD_VERSION_N <= 11702 // If before we moved CallstackContext and Context to EAThread...
				struct CallstackContext;
				struct Context;
			#endif

			/// InitCallstack
			///
			/// Allows the user to explicitly initialize the callstack mechanism.
			/// Only the first call to InitCallstack will have effect. Calls to 
			/// InitCallstack must be matched by calls to ShutdownCallstack.
			///
			EACALLSTACK_API void InitCallstack();


			/// ShutdownCallstack
			///
			/// Allows the user to explicitly shutdown the callstack mechanism.
			/// Calls to InitCallstack must be matched by calls to ShutdownCallstack.
			/// The last call to ShutdownCallstack will shutdown and free the callstack mechanism.
			///
			EACALLSTACK_API void ShutdownCallstack();


			/// GetCallstack
			///
			/// Gets the addresses of the calling instructions of a call stack.
			/// If the CallstackContext parameter is used, then that execution context is used;
			/// otherwise the current execution context is used.
			/// The return value is the number of entries written to the callstack array.
			/// The item at callstack[0] is from the function calling the GetCallstack function.
			/// For most platforms the addresses reported are the addresses of the instruction 
			/// that will next be executed upon returning from the function it is calling.
			/// The maxDepth parameter must be at least one and callstack must be able to hold
			/// at least one entry (a terminating 0 NULL entry).
			///
			EACALLSTACK_API size_t GetCallstack(void* callstack[], size_t maxDepth, const CallstackContext* pContext = NULL);




			#if defined(EA_PLATFORM_MICROSOFT)
				/// Microsoft thread handles are opaque types which are non-unique per thread.
				/// That is, two different thread handles might refer to the same thread.
				/// threadId is the same as EA::Thread::ThreadId and is a Microsoft thread HANDLE. 
				/// This is not the same as a Microsoft DWORD thread id which is the same as EA::Thread::SysThreadId.
				EACALLSTACK_API bool ThreadHandlesAreEqual(intptr_t threadId1, intptr_t threadId2);

				/// This function is the same as EA::Thread::GetSysThreadId(ThreadId id).
				/// This function converts from one type of Microsoft thread identifier to another.
				/// threadId is the same as EA::Thread::ThreadId and is a Microsoft thread HANDLE. 
				/// The return value is a Microsoft DWORD thread id which is the same as EA::Thread::SysThreadId.
				/// Upon failure, the return value will be zero.
				EACALLSTACK_API uint32_t GetThreadIdFromThreadHandle(intptr_t threadId);
			#endif


			/// GetCallstackContext
			///
			/// Gets the CallstackContext associated with the given thread.
			/// The thread must be in a non-running state.
			/// If the threadID is EA::Thread::kThreadIdInvalid, the current thread context is retrieved.
			/// However, it's of little use to get the context of the current thread, since upon return
			/// from the GetCallstackContext the data will not apply to the current thread any more;
			/// thus this information is probably useful only for diagnostic purposes.
			/// The threadId parameter is the same type as an EAThread ThreadId. It is important to 
			/// note that an EAThread ThreadId under Microsoft platforms is a thread handle and not what 
			/// Microsoft calls a thread id. This is by design as Microsoft thread ids are second class
			/// citizens and likely wouldn't exist if it not were for quirks in the Windows API evolution.
			///
			/// Note that threadId is the same as EA::Thread::ThreadId and is a Microsoft thread HANDLE. 
			/// This is not the same as a Microsoft DWORD thread id which is the same as EA::Thread::SysThreadId.
			///
			/// EACallstack has a general struct for each CPU type called Context, defined in EACallstack/Context.h. 
			/// The Context struct contains the entire CPU register context information. In order to walk a thread's 
			/// callstack, you really need only two or three of the register values from the Context. So there is a 
			/// mini struct called CallstackContext which is just those registers needed to read a thread's callstack.
			///
			EACALLSTACK_API bool GetCallstackContext(CallstackContext& context, intptr_t threadId = 0);


			/// GetCallstackContextSysThreadId
			///
			/// This is the same as GetCallstackContext, except it uses what EAThread calls SysThreadId.
			/// On Microsoft platforms a SysThreadId is a "thread id" whereas ThreadId is "thread handle."
			/// On non-Microsoft platforms a SysThreadId is defined to be the same as ThreadId and is often
			/// just an integer (Playstation3) or opaque identifier (e.g. pthread).
			/// This function exists because it may be more convenient to work with SysThreadIds in some cases.
			/// You can convert from a ThreadId (Microsoft thread handle) to a SysThreadId (Microsoft thread id)
			/// with the GetThreadIdFromThreadHandle function.
			EACALLSTACK_API bool GetCallstackContextSysThreadId(CallstackContext& context, intptr_t sysThreadId = 0);


			/// GetCallstackContext
			///
			/// Gets the CallstackContext from a full Context struct. Note that the Context struct
			/// defines the entire machine context, whereas the CallstackContext is a tiny struct
			/// with just a couple integer members and is all that's needed to describe a callstack.
			///
			EACALLSTACK_API void GetCallstackContext(CallstackContext& context, const Context* pContext = NULL);


			/// GetModuleFromAddress
			///
			/// Given an address, this function tells what module it comes from. 
			/// The primary use of this is to tell what DLL an instruction pointer comes from.
			/// Returns the required strlen of the pModuleFileName. If the return value is >= moduleNameCapacity,
			/// there wasn't enough space. pModuleFileName is written with as many characters as possible
			/// and will always be zero terminated. moduleNameCapacity must be at least one.
			///
			EACALLSTACK_API size_t GetModuleFromAddress(const void* pAddress, char* pModuleFileName, size_t moduleNameCapacity);


			/// GetModuleHandleFromAddress
			///
			/// Returns the module handle from a code address.
			/// Returns 0/NULL if no associated module could be found.
			///
			EACALLSTACK_API ModuleHandle GetModuleHandleFromAddress(const void* pAddress);


			/// EAGetInstructionPointer
			///
			/// Returns the current instruction pointer (a.k.a. program counter).
			/// This function is implemented as a macro, it acts as if its declaration 
			/// were like so:
			///     void EAGetInstructionPointer(void*& p);
			///
			/// For portability, this function should only be used as a standalone 
			/// statement on its own line.
			///
			/// Example usage:
			///    void* pInstruction;
			///    EAGetInstructionPointer(pInstruction);
			///
			#if defined(_MSC_VER) && defined(EA_PROCESSOR_X86)
				// We implement this via calling the next line of code as a function.
				// Then we continue as if we were exiting that function but with no
				// return statement. The result is that the instruction pointer will
				// be placed on the stack and we merely pop it off the stack and 
				// into a local variable.
				#define EAGetInstructionPointer(p)   \
				{                                    \
					uintptr_t eip;                   \
					__asm {                          \
						__asm call GetEIP            \
						__asm GetEIP:                \
						__asm pop eip                \
					}                                \
					p = (void*)eip;                  \
				}

				inline void GetInstructionPointer(void*& p)
					{ EAGetInstructionPointer(p); }

			#elif defined(_MSC_VER) && (defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_ARM))

				EACALLSTACK_API EA_NO_INLINE void GetInstructionPointer(void*& p);

				#define EAGetInstructionPointer(p) EA::Callstack::GetInstructionPointer(p)

			#elif defined(_MSC_VER) && defined(EA_PROCESSOR_POWERPC) && (EA_PLATFORM_PTR_SIZE == 4)

				/// GetInstructionPointer
				/// Function-based version of GetInstructionPointer.
				EACALLSTACK_API void GetInstructionPointer(void*& p);

				// The XBox 360 inline assembler doesn't seem to like labels, 
				// so we can't use a (bl NextLineLabel) statement. Also, (bl $+04)
				// doesn't work either, though on x86 you can use such a statement  
				// to jump to the next line. However, we can achieve the same thing
				// via a machine opcode emit. The 0x48000004 opcode means the same
				// thing as (bl <next instruction>), and is like calling the next
				// instruction as a function.

				#define EAGetInstructionPointer(p) \
				{                                  \
					__asm {mflr r0;}               \
					__emit(0x48000004);            \
					__asm {mflr r12;}              \
					__asm {mtlr r0;}               \
					__asm {stw r12, p;}            \
				}

			#elif defined(__ARMCC_VERSION) // ARM compiler

				// Even if there are compiler intrinsics that let you get the instruction pointer, 
				// this function can still be useful. For example, on ARM platforms this function
				// returns the address with the 'thumb bit' set if it's thumb code. We need this info sometimes.
				EACALLSTACK_API void GetInstructionPointer(void*& p);

				// The ARM compiler provides a __current_pc() instrinsic, which returns an unsigned integer type.
				#define EAGetInstructionPointer(p) { uintptr_t pc = (uintptr_t)__current_pc(); p = reinterpret_cast<void*>(pc); }

			//#elif defined(EA_COMPILER_CLANG) // Disabled until implemented. The GCC code below works under clang, though it wouldn't if compiler extensions were disabled.
			//    EACALLSTACK_API void GetInstructionPointer(void*& p);
			//
			//    // To do: implement this directly instead of via a call to GetInstructionPointer.
			//    #define EAGetInstructionPointer(p) EA::Callstack::GetInstructionPointer(p)
				
			#elif defined(__GNUC__) || defined(EA_COMPILER_CLANG) // This covers EA_PLATFORM_UNIX, EA_PLATFORM_OSX

				// Even if there are compiler intrinsics that let you get the instruction pointer, 
				// this function can still be useful. For example, on ARM platforms this function
				// returns the address with the 'thumb bit' set if it's thumb code. We need this info sometimes.
				EACALLSTACK_API void GetInstructionPointer(void*& p);

				// It turns out that GCC has an extension that allows you to take the address 
				// of a label. The code here looks a little wacky, but that's how it's done.
				// Basically, this generates a global variable called 'label' and the assignment
				// to 'p' reads that variable into p. One possible downside to this technique is
				// that it relies on registers and global memory not being corrupted, yet one of
				// reasons why we might want to be getting the instruction pointer is in dealing
				// with some sort or processor exception which may be due to memory corruption.
				// To consider: Make a version of this which calculates the value dynamically via asm.
				#define EAGetInstructionPointer(p) { p = ({ __label__ label; label: &&label; }); }
			#else
				#error
			#endif


			/// EASetStackBase / SetStackBase / GetStackBase / GetStackLimit / GetStackCurrent
			///
			/// EASetStackBase as a macro and acts as if its declaration were like so:
			///     void EASetStackBase();
			/// 
			/// EASetStackBase sets the current stack pointer as the bottom (beginning)
			/// of the stack. Depending on the platform, the "bottom" may be up or down
			/// depending on whether the stack grows upward or downward (usually it grows
			/// downward and so "bottom" actually refers to an address that is above child
			/// stack frames in memory.
			/// This function is intended to be called on application startup as early as 
			/// possiblem, and in each created thread, as early as possible. Its purpose 
			/// is to record the beginning stack pointer because the platform doesn't provide
			/// APIs to tell what it is, and we need to know it (e.g. so we don't overrun
			/// it during stack unwinds). 
			///
			/// For portability, EASetStackBase should be used only as a standalone 
			/// statement on its own line, as it may include statements that can't work otherwise.
			///
			/// Example usage:
			///    int main(int argc, char** argv) {
			///       EASetStackBase();
			///       . . .
			///    }
			///
			/// SetStackBase is a function which lets you explicitly set a stack bottom instead
			/// of doing it automatically with EASetStackBase. If you pass NULL for pStackBase
			/// then the function uses its stack location during its execution, which will be 
			/// a little less optimal than calling EASetStackBase.
			///
			/// GetStackBase returns the stack bottom set by EASetStackBase or SetStackBase.
			/// It returns NULL if no stack bottom was set or could be set.
			///
			/// GetStackLimit returns the current stack "top", which will be lower than the stack
			/// bottom in memory if the platform grows its stack downward.

			EACALLSTACK_API void  SetStackBase(void* pStackBase);
			inline          void  SetStackBase(uintptr_t pStackBase){ SetStackBase((void*)pStackBase); }
			EACALLSTACK_API void* GetStackBase();
			EACALLSTACK_API void* GetStackLimit();
			EACALLSTACK_API void* GetStackCurrent();


			#if defined(_MSC_VER) && defined(EA_PROCESSOR_X86)
				#define EASetStackBase()                  \
				{                                         \
					void* esp;                            \
					__asm { mov esp, ESP }                \
					::EA::Callstack::SetStackBase(esp);   \
				}                               

			#elif defined(_MSC_VER) && (defined(EA_PROCESSOR_X86_64) || defined(EA_PROCESSOR_ARM))
				// This implementation uses SetStackBase(NULL), which internally retrieves the stack pointer.
				#define EASetStackBase()                        \
				{                                               \
					::EA::Callstack::SetStackBase((void*)NULL); \
				}                                               \

			#elif defined(_MSC_VER) && defined(EA_PROCESSOR_POWERPC) && (EA_PLATFORM_PTR_SIZE == 4)

				#define EASetStackBase()                          \
				{                                                 \
					uint64_t gpr1;                                \
					__asm { std r1, gpr1 };                       \
					::EA::Callstack::SetStackBase((void*)gpr1);   \
				}

			#elif defined(__ARMCC_VERSION)          // ARM compiler

				#define EASetStackBase()  \
					::EA::Callstack::SetStackBase((void*)__current_sp())

			#elif defined(__GNUC__) // This covers EA_PLATFORM_UNIX, EA_PLATFORM_OSX

				#define EASetStackBase()  \
					::EA::Callstack::SetStackBase((void*)__builtin_frame_address(0));

			#else
				// This implementation uses SetStackBase(NULL), which internally retrieves the stack pointer.
				#define EASetStackBase()                        \
				{                                               \
					::EA::Callstack::SetStackBase((void*)NULL); \
				}                                               \

			#endif



			/// CallstackContext
			///
			/// Processor-specific information that's needed to walk a call stack.
			///
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

			// The following are base values required for processor-agnostic offline stack dumping. 
			// Not all implementations will fill them in, and most times only the base and pointer 
			// will be filled. Also, most of the specific contexts' will have a member with the 
			// same value as the stack pointer, i.e. mESP on the x86
			struct CallstackContextBase
			{
				uintptr_t mStackBase;       /// Used to help tell what the valid stack ranges is. 0 if not used.
				uintptr_t mStackLimit;      /// "
				uintptr_t mStackPointer;    /// [Deprecated, use the specific stack pointer instead]

				CallstackContextBase() : mStackBase(0), mStackLimit(0), mStackPointer(0) {}
			};

				struct CallstackContextPowerPC : public CallstackContextBase
				{
					uintptr_t mGPR1;        /// General purpose register 1.
					uintptr_t mIAR;         /// Instruction address pseudo-register.
					
					CallstackContextPowerPC() : mGPR1(0), mIAR(0) {}
				};

			#if defined(EA_PROCESSOR_POWERPC)
				struct CallstackContext : public CallstackContextPowerPC 
				{ 
					static const CallstackContextType kType = CALLSTACK_CONTEXT_POWERPC;
				};
			#endif

				struct CallstackContextX86 : public CallstackContextBase
				{
					uint32_t mEIP;      /// Instruction pointer.
					uint32_t mESP;      /// Stack pointer.
					uint32_t mEBP;      /// Base pointer.

					CallstackContextX86() : mEIP(0), mESP(0), mEBP(0) {}
				};

			#if defined(EA_PROCESSOR_X86)
				struct CallstackContext : public CallstackContextX86 
				{ 
					static const CallstackContextType kType = CALLSTACK_CONTEXT_X86;
				};
			#endif

				struct CallstackContextX86_64 : public CallstackContextBase
				{
					uint64_t mRIP;      /// Instruction pointer.
					uint64_t mRSP;      /// Stack pointer.
					uint64_t mRBP;      /// Base pointer.

					CallstackContextX86_64() : mRIP(0), mRSP(0), mRBP(0) {}
				};

			#if defined(EA_PROCESSOR_X86_64)
				struct CallstackContext : public CallstackContextX86_64 
				{ 
					static const CallstackContextType kType = CALLSTACK_CONTEXT_X86_64;
				};
			#endif

			struct CallstackContextARM : public CallstackContextBase
			{
				uint32_t mFP;   /// Frame pointer; register 11 for ARM instructions, register 7 for Thumb instructions.
				uint32_t mSP;   /// Stack pointer; register 13
				uint32_t mLR;   /// Link register; register 14
				uint32_t mPC;   /// Program counter; register 15
				CallstackContextARM() : mFP(0), mSP(0), mLR(0), mPC(0) {}
			};

			#if defined(EA_PROCESSOR_ARM)
				struct CallstackContext : public CallstackContextARM 
				{ 
					static const CallstackContextType kType = CALLSTACK_CONTEXT_ARM;
				};
			#endif

				struct CallstackContextMIPS : public CallstackContextBase
				{
					uintptr_t mPC;      /// Program counter.
					uintptr_t mSP;      /// Stack pointer.
					uintptr_t mFP;      /// Frame pointer.
					uintptr_t mRA;      /// Return address.

					CallstackContextMIPS() : mPC(0), mSP(0), mFP(0), mRA(0) {}
				};
		} // namespace Callstack

	} // namespace EA


	namespace EA
	{
		namespace Callstack
		{
			////////////////////////////////////////////////////////////////////////////////////////////
			// inlines:
			////////////////////////////////////////////////////////////////////////////////////////////

			///////////////////////////////////////////////////////////////////////////////
			// GetStackCurrent
			//
			EACALLSTACK_API inline void* GetStackCurrent()
			{
				// We want to return the address of buffer, but don't trigger any compiler warnings
				// that we are returning the address of memory that will be invalid upon return.
				// We also want to avoid this function from being made into a leaf funtion with
				// no stack frame, hence the call to sprintf.
				char  buffer[16];
				void* pStack = buffer;
				sprintf(buffer, "%p", &pStack);
				return pStack;
			}


			////////////////////////////////////////////////////////////////////////////////////////////
			// Deprecated:
			////////////////////////////////////////////////////////////////////////////////////////////
			inline void  SetStackBottom(void* pStackBottom)    { SetStackBase(pStackBottom); }
			inline void  SetStackBottom(uintptr_t pStackBottom){ SetStackBase(pStackBottom); }
			inline void* GetStackBottom()                      { return GetStackBase(); }
			inline void* GetStackTop()                         { return GetStackLimit(); }

			#define EASetStackBottom EASetStackBase

		}
	}

#endif

#endif // Header include guard.







