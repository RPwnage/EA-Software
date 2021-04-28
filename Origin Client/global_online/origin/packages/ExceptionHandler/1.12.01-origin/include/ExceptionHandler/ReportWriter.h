///////////////////////////////////////////////////////////////////////////////
// ReportWriter.h
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Written and maintained by Paul Pedriana.
//
// Exception handling and reporting facilities.
///////////////////////////////////////////////////////////////////////////////


#ifndef EXCEPTIONHANDLER_REPORTWRITER_H
#define EXCEPTIONHANDLER_REPORTWRITER_H


#include <EABase/eabase.h>
#include <eathread/eathread.h>
#include <stdio.h>
#include <time.h>
#include <EACallstack/EACallstack.h>
#include <coreallocator/icoreallocator_interface.h>


namespace EA
{
    #if (defined(EA_PROCESSOR_POWERPC) || defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_ARM))
        namespace Dasm {
        #if defined(EA_PROCESSOR_POWERPC)
            class DisassemblerPowerPC;
        #elif defined(EA_PROCESSOR_X86)
            class DisassemblerX86;
        #elif defined(EA_PROCESSOR_ARM)
            class DisassemblerARM;
        #endif
        }
    #endif

    namespace Debug
    {
        class ReportWriter
        {
        public:
            ReportWriter();
            virtual ~ReportWriter();

            enum Option
            {
                kOptionNone,
                kOptionReportUserName,      // 0 or 1. Default is 0 in release builds. Disabled by default because when uploaded to EA from a user computer it represents personal information.
                kOptionReportComputerName,  // 0 or 1. Default is 0 in release builds, for user security reasons.
                kOptionReportProcessList,   // 0 or 1. Default is 0 in release builds, for user security reasons.
                kOptionCount
            };

            /// Allows you to set a memory allocator for use by ReportWriter. Note that by default ReportWriter inherits the 
            /// allocator set by ExceptionHandler::SetAllocator, so if you call that then you don't need to call this. 
            /// Due to the fact that the global heap may be in an unstable state during a crash (it may have crashed itelf), 
            /// allocators set here ideally should be separated from the main heap. ReportWriter avoids using the allocator
            /// to the extent possible for this reason.
            virtual EA::Allocator::ICoreAllocator* GetAllocator() const;
            virtual void SetAllocator(EA::Allocator::ICoreAllocator* pCoreAllocator);

            /// Example usage:
            ///     pReportWriter->SetOption(ReportWriter::kOptionReportUserName, 1);
            virtual int  GetOption(Option option) const;
            virtual void SetOption(Option option, int value);

            virtual void SetExceptionThreadIds(const Thread::ThreadId& threadId, const Thread::SysThreadId& exceptionSysThreadId);
            virtual void SetExceptionCallstack(const void* addressArray, size_t count);

            virtual void GetFileNameExtension(char* pExtension, size_t capacity);

            virtual bool OpenReport(const char* pReportPath = NULL);
            virtual bool CloseReport();

            virtual bool BeginReport(const char* pReportName);
            virtual bool EndReport();

            virtual bool BeginSection(const char* pSectionName);
            virtual bool EndSection(const char* pSectionName);

            virtual bool WriteText(const char* pText, bool bAppendNewline = false);
            virtual bool WriteFormatted(const char* pFormat, ...);
            virtual bool WriteKeyValue(const char* pKey, const char* pValue);
            virtual bool WriteKeyValueFormatted(const char* pKey, const char* pFormat, ...);
            virtual bool WriteBlankLine();
            virtual bool WriteFlush();

            virtual bool Write(const char* pData, size_t count = (size_t)-1);

            virtual size_t GetPosition();
            virtual void   SetPosition(size_t pos);

            // Report reading functionality. This is used for reading report data in 
            // a way that avoids allocating a memory buffer to hold the entire report, 
            // as that may be unsafe. This may be called only after CloseReport was 
            // called for a written report.
            virtual bool    ReadReportBegin();                           // Returns true if successful.
            virtual size_t  ReadReportPart(char* buffer, size_t size);   // Returns (ssize_t)-1 upon error, 0 if no more to read, else the number of bytes read. Call this repeatedly until it returns -1 or 0.
            virtual void    ReadReportEnd();                             // This must always be called if ReadReportBegin returned true.

        public:
            static const size_t kMemoryViewAlignment = 16;

            virtual size_t GetTimeString(char* pTimeString, size_t capacity, const tm* pTime = NULL, bool bLongFormat = false) const;
            virtual size_t GetDateString(char* pDateString, size_t capacity, const tm* pTime = NULL, bool bFourDigitYear = true) const;
            virtual size_t GetMachineName(char* pNameString, size_t capacity);
            virtual size_t GetAddressLocation(const void* pAddress, char* buffer, size_t capacity, uint32_t& section, uint32_t& offset);
            virtual size_t GetAddressLocationString(const void* pAddress, char* buffer, size_t capacity);

            virtual bool   WriteSystemInfo();
            virtual bool   WriteApplicationInfo();
            virtual bool   WriteOpenFileList();
            virtual bool   WriteThreadList();
            virtual bool   WriteProcessList();
            virtual bool   WriteDisassembly(const uint8_t* pInstructionData, size_t count, const uint8_t* pInstructionPointer);
            virtual bool   WriteMemoryView(const uint8_t* pMemory, size_t count, const uint8_t* pMark, const char* pFirstLineHeader = NULL);
            virtual bool   WriteStackMemoryView(const uint8_t* pStackPointer, size_t count);
            virtual bool   WriteModuleList();
            virtual bool   WriteRegisterValues(const void* pPlatformContext);
            virtual bool   WriteRegisterMemoryView(const void* pPlatformContext);
            virtual void   WriteAbbreviatedCallstackForThread(EA::Thread::ThreadId threadId, EA::Thread::SysThreadId sysThreadId, bool bCurrentThread);
            virtual bool   WriteCallStack(const EA::Callstack::Context* pContext, int addressRepTypeFlags, bool bAbbreviated = false);
            virtual bool   WriteCallStackFromCallstackContext(const EA::Callstack::CallstackContext* pCallstackContext, int addressRepTypeFlags, bool bAbbreviated = false);
            virtual bool   WriteCallStackFromCallstack(void* callstack[], size_t callstackCount, int addressRepTypeFlags, bool bAbbreviated = false);

            #if (defined(EA_PROCESSOR_POWERPC) || defined(EA_PROCESSOR_X86) || defined(EA_PROCESSOR_ARM))
                #if defined(EA_PROCESSOR_POWERPC)
                    typedef EA::Dasm::DisassemblerPowerPC Disassembler;
                #elif defined(EA_PROCESSOR_X86)
                    typedef EA::Dasm::DisassemblerX86 Disassembler;
                #elif defined(EA_PROCESSOR_ARM)
                    typedef EA::Dasm::DisassemblerARM Disassembler;
                #endif
                void WriteDisassembly(Disassembler & dasm, const uint8_t* pInstructionData, size_t count, const uint8_t* pInstructionPointer);
            #endif

        protected:
            static const int kWriteBufferSize   = 1024;
            static const int kScratchBufferSize =  512;

            EA::Allocator::ICoreAllocator*  mpCoreAllocator;                    /// 
            int                             mOption[kOptionCount];              /// User settable options.
            char                            mPath[256];                         /// File path to mpFile.
            FILE*                           mpFile;                             /// FILE to report
            char                            mWriteBuffer[kWriteBufferSize];     /// Buffer for file text line writing.
            char                            mScratchBuffer[kScratchBufferSize]; /// Buffer for temporary usage, building strings.
            int                             mnSectionDepth;                     /// Count of sections that are active.
            EA::Thread::ThreadId            mExceptionThreadId;                 /// 
            EA::Thread::SysThreadId         mExceptionSysThreadId;              ///
            void*                           mExceptionCallstack[48];            /// The callstack for the exception. Don't use an STL vector because that might allocate memory, which we can't do during exception handling.
            size_t                          mExceptionCallstackCount;           ///
        };


        /// AlignPointerUp
        /// 
        /// Aligns a given object up to an address of a specified power of two.
        /// Example usage:
        ///    Matrix* pM;
        ///    pM = AlignPointerUp(pM, 16); 
        /// 
        template <typename T>
        inline T* AlignPointerUp(const T* p, size_t a)
        {
            return (T*)(((uintptr_t)p + (a - 1)) & ~(a - 1));
        }


        /// AlignPointerDown
        /// 
        /// Aligns a given object down to an address of a specified power of two.
        /// Example usage:
        ///    Matrix* pM;
        ///    pM = AlignPointerDown(pM, 16); 
        /// 
        template <typename T>
        inline T* AlignPointerDown(const T* p, size_t a)
        {
            return (T*)((uintptr_t)p & ~(a - 1));
        }

    } // namespace Debug

} // namespace EA


#endif // Header include guard




 





